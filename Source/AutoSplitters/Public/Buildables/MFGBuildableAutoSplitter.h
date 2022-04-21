// ILikeBanas

#pragma once

#include <array>
#include <tuple>

#include "FGPlayerController.h"
#include "FGFactoryConnectionComponent.h"
#include "Buildables/FGBuildableAttachmentSplitter.h"
#include "Buildables/FGBuildableConveyorBase.h"

#include "AutoSplittersModule.h"
#include "AutoSplittersRCO.h"
#include "AutoSplittersLog.h"
#include "util/BitField.h"

#include "MFGBuildableAutoSplitter.generated.h"

UENUM(BlueprintType, Meta = (BitFlags))
enum class EOutputState : uint8
{
    Automatic UMETA(DisplayName = "Automatic"),
    Connected UMETA(DisplayName = "Connected"),
    AutoSplitter UMETA(DisplayName = "AutoSplitter"),
};

template <>
struct is_enum_bitfield<EOutputState> : std::true_type {};


enum class EAutoSplitterPersistentFlags : uint32
{
    // first eight bits reserved for version
    ManualInputRate        =  8,
    NeedsConnectionsFixup  =  9,
    NeedsDistributionSetup = 10,
};

template<>
struct is_enum_bitfield<EAutoSplitterPersistentFlags> : std::true_type{};

enum class EAutoSplitterTransientFlags : uint32
{
    // first eight bits reserved for error code

    // replication is currently turned on
    IsReplicationEnabled          =  8,

    // splitter was loaded from save game and needs to be processed accordingly in BeginPlay()
    NeedsLoadedSplitterProcessing =  9,

    // splitter was deemed incompatible after load process, remove in Invoke_BeginPlay() hook
    DismantleAfterLoading         = 10,

};

template<>
struct is_enum_bitfield<EAutoSplitterTransientFlags> : std::true_type{};



USTRUCT(BlueprintType)
struct AUTOSPLITTERS_API FMFGBuildableAutoSplitterReplicatedProperties
{
    GENERATED_BODY()

    static constexpr int32 NUM_OUTPUTS = 3;

    UPROPERTY(Transient)
    uint32 TransientState;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    int32 OutputStates[NUM_OUTPUTS];

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    uint32 PersistentState;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    int32 TargetInputRate;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    int32 OutputRates[NUM_OUTPUTS];

    UPROPERTY(Transient, BlueprintReadOnly)
    int32 LeftInCycle;

    UPROPERTY(Transient, BlueprintReadOnly)
    int32 CycleLength;

    UPROPERTY(Transient, BlueprintReadOnly)
    int32 CachedInventoryItemCount;

    UPROPERTY(Transient, BlueprintReadOnly)
    float ItemRate;

    FMFGBuildableAutoSplitterReplicatedProperties();

};


class AMFGBuildableAutoSplitter;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAMFGBuildableAutoSplitterOnStateChanged,AMFGBuildableAutoSplitter*,AutoSplitter);

/**
 *
 */
UCLASS()
class AUTOSPLITTERS_API AMFGBuildableAutoSplitter : public AFGBuildableAttachmentSplitter
{
    GENERATED_BODY()

    friend class FAutoSplittersModule;
    friend class AMFGAutoSplitterHologram;
    friend class UAutoSplittersRCO;
    friend class AMFGReplicationDetailActor_BuildableAutoSplitter;

public:

    // shorter names to avoid crazy amounts of typing
    using EPersistent = EAutoSplitterPersistentFlags;
    using ETransient  = EAutoSplitterTransientFlags;

    static constexpr uint32 VERSION = 1;

    static constexpr int32 MAX_INVENTORY_SIZE = 10;
    static constexpr float EXPONENTIAL_AVERAGE_WEIGHT = 0.5f;
    static constexpr int32 NUM_OUTPUTS = 3;
    static constexpr float BLOCK_DETECTION_THRESHOLD = 0.5f;

    static constexpr int32 FRACTIONAL_RATE_DIGITS = 3;
    static constexpr int32 FRACTIONAL_RATE_MULTIPLIER = Pow_Constexpr(10,FRACTIONAL_RATE_DIGITS);
    static constexpr float INV_FRACTIONAL_RATE_MULTIPLIER = 1.0f / FRACTIONAL_RATE_MULTIPLIER;

    static constexpr int32 FRACTIONAL_SHARE_DIGITS = 5;
    static constexpr int64 FRACTIONAL_SHARE_MULTIPLIER = Pow_Constexpr(10,FRACTIONAL_SHARE_DIGITS);
    static constexpr float INV_FRACTIONAL_SHARE_MULTIPLIER = 1.0f / FRACTIONAL_SHARE_MULTIPLIER;

    static constexpr float UPGRADE_POSITION_REQUIRED_DELTA = 100.0f;

public:

    AMFGBuildableAutoSplitter();
    virtual void GetLifetimeReplicatedProps( TArray< FLifetimeProperty >& OutLifetimeProps ) const override;

    virtual void BeginPlay() override;
    virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;

    virtual UClass* GetReplicationDetailActorClass() const override;

protected:

    virtual void Factory_Tick(float dt) override;
    virtual bool Factory_GrabOutput_Implementation( UFGFactoryConnectionComponent* connection, FInventoryItem& out_item, float& out_OffsetBeyond, TSubclassOf< UFGItemDescriptor > type ) override;
    virtual void FillDistributionTable(float dt) override;

    UAutoSplittersRCO* RCO() const
    {
        return UAutoSplittersRCO::Get(GetWorld());
    }

    void Server_EnableReplication(float Duration);

    bool Server_SetTargetRateAutomatic(bool Automatic);

    bool Server_SetTargetInputRate(float Rate);

    bool Server_SetOutputRate(int32 Output, float Rate);

    bool Server_SetOutputAutomatic(int32 Output, bool Automatic);

    void Server_ReplicationEnabledTimeout();

    UFUNCTION()
    void OnRep_Replicated()
    {
        OnStateChangedEvent.Broadcast(this);
    }

private:

    void SetupDistribution(bool LoadingSave = false);
    void PrepareCycle(bool AllowCycleExtension, bool Reset = false);

    bool IsOutputBlocked(int32 Output) const
    {
        return mBlockedFor[Output] > BLOCK_DETECTION_THRESHOLD;
    }

protected:

    UPROPERTY(SaveGame,ReplicatedUsing=OnRep_Replicated, BlueprintReadOnly, Meta = (NoAutoJson))
    FMFGBuildableAutoSplitterReplicatedProperties mReplicated;

    UPROPERTY(Transient)
    uint32 mTransientState_DEPRECATED;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    TArray<int32> mOutputStates_DEPRECATED;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    TArray<int32> mRemainingItems_DEPRECATED;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    uint32 mPersistentState_DEPRECATED;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    int32 mTargetInputRate_DEPRECATED;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    TArray<int32> mIntegralOutputRates_DEPRECATED;

    UPROPERTY(Transient)
    int32 mLeftInCycle_DEPRECATED;

    UPROPERTY(SaveGame, Meta = (NoAutoJson))
    int32 mLeftInCycleForOutputs[NUM_OUTPUTS];

    UPROPERTY(Transient, BlueprintReadWrite)
    bool mDebug;

    UPROPERTY(Transient)
    int32 mCycleLength_DEPRECATED;

    UPROPERTY(Transient)
    int32 mCachedInventoryItemCount_DEPRECATED;

    UPROPERTY(Transient)
    float mItemRate_DEPRECATED;

    UPROPERTY(BlueprintAssignable)
    FAMFGBuildableAutoSplitterOnStateChanged OnStateChangedEvent;

private:

    std::array<int32,NUM_OUTPUTS> mItemsPerCycle;
    std::array<float,NUM_OUTPUTS> mBlockedFor;
    std::array<int32,NUM_OUTPUTS> mAssignedItems;
    std::array<int32,NUM_OUTPUTS> mGrabbedItems;
    std::array<float,NUM_OUTPUTS> mPriorityStepSize;
    std::array<int32,MAX_INVENTORY_SIZE> mAssignedOutputs;
    std::array<int32,NUM_OUTPUTS> mNextInventorySlot;
    std::array<int32,NUM_OUTPUTS> mInventorySlotEnd;

    bool mBalancingRequired;
    bool mNeedsInitialDistributionSetup;
    float mCycleTime;
    int32 mReallyGrabbed;

    FTimerHandle mReplicationTimer;

public:

    UFUNCTION(BlueprintPure)
    static int32 GetFractionalRateDigits()
    {
        return FRACTIONAL_RATE_DIGITS;
    }

    UFUNCTION(BlueprintPure)
    bool IsReplicationEnabled() const
    {
        return IsSplitterFlagSet(ETransient::IsReplicationEnabled);
    }

    UFUNCTION(BlueprintCallable)
    void EnableReplication(float Duration)
    {
        if (HasAuthority())
            Server_EnableReplication(Duration);
        else
        {
            UE_LOG(LogAutoSplitters,Display,TEXT("Forwarding AMFGBuildableAutoSplitter::EnableReplication() to RCO"));
            RCO()->EnableReplication(this,Duration);
        }
    }

    UFUNCTION(BlueprintCallable,BlueprintPure)
    bool IsTargetRateAutomatic() const
    {
        return !IsSplitterFlagSet(EPersistent::ManualInputRate);
    }

    UFUNCTION(BlueprintCallable)
    void SetTargetRateAutomatic(bool Automatic)
    {
        if (HasAuthority())
            Server_SetTargetRateAutomatic(Automatic);
        else
        {
            UE_LOG(LogAutoSplitters,Display,TEXT("Forwarding AMFGBuildableAutoSplitter::SetTargetRateAutomatic() to RCO"));
            RCO()->SetTargetRateAutomatic(this,Automatic);
        }
    }


    UFUNCTION(BlueprintPure)
    float GetTargetInputRate() const;

    UFUNCTION(BlueprintCallable)
    void SetTargetInputRate(float Rate)
    {
        if (HasAuthority())
            Server_SetTargetInputRate(Rate);
        else
        {
            UE_LOG(LogAutoSplitters,Display,TEXT("Forwarding AMFGBuildableAutoSplitter::SetTargetInputRate() to RCO"));
            RCO()->SetTargetInputRate(this,Rate);
        }
    }

    UFUNCTION(BlueprintPure)
    float GetOutputRate(int32 Output) const;

    UFUNCTION(BlueprintCallable)
    void SetOutputRate(int32 Output, float Rate)
    {
        if (HasAuthority())
            Server_SetOutputRate(Output,Rate);
        else
        {
            UE_LOG(LogAutoSplitters,Display,TEXT("Forwarding AMFGBuildableAutoSplitter::SetOutputRate() to RCO"));
            RCO()->SetOutputRate(this,Output,Rate);
        }
    }

    UFUNCTION(BlueprintCallable)
    void SetOutputAutomatic(int32 Output, bool Automatic)
    {
        if (HasAuthority())
            Server_SetOutputAutomatic(Output,Automatic);
        else
        {
            UE_LOG(LogAutoSplitters,Display,TEXT("Forwarding AMFGBuildableAutoSplitter::OutputAutomatic() to RCO"));
            RCO()->SetOutputAutomatic(this,Output,Automatic);
        }
    }

    UFUNCTION(BlueprintPure)
    bool IsOutputAutomatic(int32 Output) const
    {
        if (Output < 0 || Output > NUM_OUTPUTS)
            return false;

        return IsSet(mReplicated.OutputStates[Output],EOutputState::Automatic);
    }

    UFUNCTION(BlueprintPure)
    bool IsOutputAutoSplitter(int32 Output) const
    {
        if (Output < 0 || Output > NUM_OUTPUTS)
            return false;

        return IsSet(mReplicated.OutputStates[Output],EOutputState::AutoSplitter);
    }

    UFUNCTION(BlueprintPure)
    bool IsOutputConnected(int32 Output) const
    {
        if (Output < 0 || Output > NUM_OUTPUTS)
            return false;

        return IsSet(mReplicated.OutputStates[Output],EOutputState::Connected);
    }

    UFUNCTION(BlueprintCallable)
    void BalanceNetwork(bool RootOnly = true)
    {
        if (HasAuthority())
            Server_BalanceNetwork(this,RootOnly);
        else
        {
            UE_LOG(LogAutoSplitters,Display,TEXT("Forwarding AMFGBuildableAutoSplitter::BalanceNetwork() to RCO"));
            RCO()->BalanceNetwork(this,RootOnly);
        }
    }

    uint32 GetSplitterVersion() const
    {
        return mReplicated.PersistentState & 0xFFu;
    }

    UFUNCTION(BlueprintPure)
    int32 GetInventorySize() const
    {
        return mReplicated.CachedInventoryItemCount;
    }

    UFUNCTION(BlueprintPure)
    float GetItemRate() const
    {
        return mReplicated.ItemRate;
    }

    UFUNCTION(BluePrintPure)
    static bool IsDebugSupported()
    {
#if AUTO_SPLITTERS_DEBUG
        return true;
#else
        return false;
#endif
    }

    UFUNCTION(BluePrintCallable)
    bool HasCurrentData() // do not mark this const, as it will turn the function pure in the blueprint
    {
        return HasAuthority() || IsReplicationEnabled();
    }

    UFUNCTION(BlueprintPure)
    int32 GetError() const
    {
        return mReplicated.TransientState & 0xFFu;
    }

    struct FNetworkNode
    {
        AMFGBuildableAutoSplitter* Splitter;
        FNetworkNode* Input;
        int32 MaxInputRate;
        std::array<FNetworkNode*,NUM_OUTPUTS> Outputs;
        std::array<int64,NUM_OUTPUTS> PotentialShares;
        std::array<int32,NUM_OUTPUTS> MaxOutputRates;
        int32 FixedDemand;
        int64 Shares;
        int32 AllocatedInputRate;
        std::array<int32,NUM_OUTPUTS> AllocatedOutputRates;
        bool ConnectionStateChanged;

        explicit FNetworkNode(AMFGBuildableAutoSplitter* Splitter, FNetworkNode* Input = nullptr)
            : Splitter(Splitter)
            , Input(Input)
            , MaxInputRate(0)
            , Outputs({nullptr})
            , PotentialShares({0})
            , MaxOutputRates({0})
            , FixedDemand(0)
            , Shares(0)
            , AllocatedInputRate(0)
            , AllocatedOutputRates({0})
            , ConnectionStateChanged(false)
        {}
    };

private:

    void SetError(uint8 Error)
    {
        mReplicated.TransientState = (mReplicated.TransientState & ~0xFFu) | static_cast<uint32>(Error);
    }

    void ClearError()
    {
        mReplicated.TransientState &= ~0xFFu;;
    }

    void FixupConnections();
    void SetupInitialDistributionState();

    static std::tuple<bool,int32> Server_BalanceNetwork(AMFGBuildableAutoSplitter* ForSplitter, bool RootOnly = false);

    static std::tuple<AMFGBuildableAutoSplitter*, int32, bool>
    FindAutoSplitterAndMaxBeltRate(UFGFactoryConnectionComponent* Connection, bool Forward);

    static std::tuple<AFGBuildableFactory*, int32, bool>
    FindFactoryAndMaxBeltRate(UFGFactoryConnectionComponent* Connection, bool Forward);

    static bool DiscoverHierarchy(
        TArray<TArray<FNetworkNode>>& Nodes,
        AMFGBuildableAutoSplitter* Splitter,
        const int32 Level,
        FNetworkNode* InputNode,
        const int32 ChildInParent,
        AMFGBuildableAutoSplitter* Root, bool ExtractPotentialShares
    );

    void SetSplitterVersion(uint32 Version);

    FORCEINLINE bool IsSplitterFlagSet(EPersistent Flag) const
    {
        return IsSet(mReplicated.PersistentState,Flag);
    }

    FORCEINLINE void SetSplitterFlag(EPersistent Flag, bool Value)
    {
        mReplicated.PersistentState = SetFlag(mReplicated.PersistentState,Flag,Value);
    }

    FORCEINLINE void SetSplitterFlag(EPersistent Flag)
    {
        mReplicated.PersistentState = SetFlag(mReplicated.PersistentState,Flag);
    }

    FORCEINLINE void ClearSplitterFlag(EPersistent Flag)
    {
        mReplicated.PersistentState = ClearFlag(mReplicated.PersistentState,Flag);
    }

    FORCEINLINE void ToggleSplitterFlag(EPersistent Flag)
    {
        mReplicated.PersistentState = ToggleFlag(mReplicated.PersistentState,Flag);
    }

    FORCEINLINE bool IsSplitterFlagSet(ETransient Flag) const
    {
        return IsSet(mReplicated.TransientState,Flag);
    }

    FORCEINLINE void SetSplitterFlag(ETransient Flag, bool Value)
    {
        mReplicated.TransientState = SetFlag(mReplicated.TransientState,Flag,Value);
    }

    FORCEINLINE void SetSplitterFlag(ETransient Flag)
    {
        mReplicated.TransientState = SetFlag(mReplicated.TransientState,Flag);
    }

    FORCEINLINE void ClearSplitterFlag(ETransient Flag)
    {
        mReplicated.TransientState = ClearFlag(mReplicated.TransientState,Flag);
    }

    FORCEINLINE void ToggleSplitterFlag(ETransient Flag)
    {
        mReplicated.TransientState = ToggleFlag(mReplicated.TransientState,Flag);
    }

};
