// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystem/ModSubsystem.h"
#include "Buildables/MFGBuildableAutoSplitter.h"
#include "AutoSplitters_ConfigStruct.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Util/SemVersion.h"
#include "AutoSplittersSerializationVersion.h"

#include "AutoSplittersSubsystem.generated.h"

UENUM()
enum class EAAutoSplittersSubsystemSeverity : uint8
{
    Debug = 1,
    Info  = 2,
    Notice = 3,
    Warning = 4,
    Error = 5,
};

/**
 *
 */
UCLASS(BlueprintType,NotBlueprintable)
class AUTOSPLITTERS_API AAutoSplittersSubsystem : public AModSubsystem, public IFGSaveInterface
{
    GENERATED_BODY()

    friend class FAutoSplittersModule;

    static AAutoSplittersSubsystem* sCachedSubsystem;

public:

    using ESeverity = EAAutoSplittersSubsystemSeverity;

    static const FVersion New_Session;
    static const FVersion ModVersion_Legacy;

protected:

    UPROPERTY(SaveGame,BlueprintReadOnly)
    TEnumAsByte<EAutoSplittersSerializationVersion> mSerializationVersion;

    // FVersion cannot be serialized, so we store the data into three separate properties in the save file
    UPROPERTY(Transient,BlueprintReadOnly)
    FVersion mLoadedModVersion;

    UPROPERTY(Transient,BlueprintReadOnly)
    FVersion mRunningModVersion;

    UPROPERTY(Transient,BlueprintReadOnly)
    FAutoSplitters_ConfigStruct mConfig;

private:

    UPROPERTY(SaveGame)
    int64 mVersionMajor;

    UPROPERTY(SaveGame)
    int64 mVersionMinor;

    UPROPERTY(SaveGame)
    int64 mVersionPatch;

    bool mIsNewSession;

    static AAutoSplittersSubsystem* FindAndGet(UObject* WorldContext,bool FailIfMissing);

protected:

    virtual void Init() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

    AAutoSplittersSubsystem();

    static AAutoSplittersSubsystem* Get(UObject* WorldContext,bool FailIfMissing = true)
    {
        if (sCachedSubsystem)
            return sCachedSubsystem;
        return FindAndGet(WorldContext,FailIfMissing);
    }

    FVersion GetLoadedModVersion() const
    {
        return mLoadedModVersion;
    }

    FVersion GetRunningModVersion() const
    {
        return mRunningModVersion;
    }

    EAutoSplittersSerializationVersion GetSerializationVersion() const
    {
        return mSerializationVersion.GetValue();
    }

    const FAutoSplitters_ConfigStruct& GetConfig() const
    {

        return mConfig;
    }

    bool IsModOlderThanSaveGame() const
    {
        return GetLoadedModVersion().Compare(New_Session) != 0 && GetLoadedModVersion().Compare(GetRunningModVersion()) > 0;
    }

    bool IsNewSession() const
    {
        return mIsNewSession;
    }

    void ReloadConfig();

    void NotifyChat(ESeverity Severity,FString Msg) const;

    virtual void PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
    //virtual void PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion) override;
    //virtual void PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
    virtual void PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion) override;
    //virtual void GatherDependencies_Implementation( TArray< UObject* >& out_dependentObjects) override;
    virtual bool NeedTransform_Implementation() override;
    virtual bool ShouldSave_Implementation() const override;

};
