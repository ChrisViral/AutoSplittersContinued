// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/AutoSplittersSubsystem.h"
#include "ModLoading/ModLoadingLibrary.h"
#include "AutoSplittersLog.h"

AAutoSplittersSubsystem* AAutoSplittersSubsystem::sCachedSubsystem = nullptr;

const FVersion AAutoSplittersSubsystem::New_Session = FVersion(INT32_MAX,INT32_MAX,INT32_MAX);
const FVersion AAutoSplittersSubsystem::ModVersion_Legacy = FVersion(0,0,0);

AAutoSplittersSubsystem::AAutoSplittersSubsystem()
    : mLoadedModVersion(New_Session) // marker for new session
    , mSerializationVersion(EAutoSplittersSerializationVersion::Legacy)
    , mIsNewSession(false)
{
    ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
}

AAutoSplittersSubsystem* AAutoSplittersSubsystem::FindAndGet(UObject* WorldContext, bool FailIfMissing)
{
    const auto World = WorldContext->GetWorld();
    auto SubsystemActorManager = World->GetSubsystem<USubsystemActorManager>();
    sCachedSubsystem = SubsystemActorManager->GetSubsystemActor<AAutoSplittersSubsystem>();
    if (FailIfMissing)
        check(sCachedSubsystem);
    return sCachedSubsystem;
}

void AAutoSplittersSubsystem::Init()
{
    Super::Init();

	FModInfo ModInfo;
    GEngine->GetEngineSubsystem<UModLoadingLibrary>()->GetLoadedModInfo("AutoSplitters",ModInfo);
    mRunningModVersion = ModInfo.Version;

    // preload configuration
    ReloadConfig();

    UE_LOG(LogAutoSplitters,Display,TEXT("AAutoSplittersSubsytem initialized: AutoSplitters %s"),*GetRunningModVersion().ToString());

    // figure out if this a loaded save file or a new session
    if (GetLoadedModVersion().Compare(New_Session) == 0)
    {
        if (FAutoSplittersModule::Get()->HaveLoadedSplitters())
        {
            // subsystem was not loaded from save, we need to fix this up manually
            mLoadedModVersion = ModVersion_Legacy;
            mIsNewSession = false;
        }
        else
        {
            mIsNewSession = true;
        }
    }
    else
    {
        mIsNewSession = false;
    }

    if (!IsNewSession())
    {
        UE_LOG(
            LogAutoSplitters,
            Display,
            TEXT("Savegame was created with AutoSplitters %s"),
            GetLoadedModVersion().Compare(ModVersion_Legacy) == 0 ? TEXT("legacy version < 0.3.9") : *GetLoadedModVersion().ToString()
            );
    }

    if (IsModOlderThanSaveGame())
    {
        UE_LOG(
            LogAutoSplitters,
            Warning,
            TEXT("Savegame was created with a newer mod version, there might be bugs or incompatibilities")
            );
    }

    UE_LOG(
        LogAutoSplitters,
        Display,
        TEXT("AutoSplitters serialization version: %d (%s)"),
        GetSerializationVersion(),
        *UEnum::GetValueAsString(GetSerializationVersion())
        );

    if (GetSerializationVersion() > EAutoSplittersSerializationVersion::Latest)
    {
        UE_LOG(LogAutoSplitters,Error,TEXT("Serialization version not supported by this version of the mod, all splitters will be removed during loading"));
    }

}

void AAutoSplittersSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (this == sCachedSubsystem)
        sCachedSubsystem = nullptr;
}

void AAutoSplittersSubsystem::ReloadConfig()
{
    mConfig = FAutoSplitters_ConfigStruct::GetActiveConfig();
}

void AAutoSplittersSubsystem::NotifyChat(ESeverity Severity, FString Msg) const
{
    auto ChatManager = AFGChatManager::Get(GetWorld());

    FChatMessageStruct Message;
    Message.MessageString = FString::Printf(TEXT("AutoSplitters: %s"),*Msg);
    Message.MessageType = EFGChatMessageType::CMT_SystemMessage;
    Message.ServerTimeStamp = GetWorld()->TimeSeconds;

    switch (Severity)
    {
    case ESeverity::Debug:
    case ESeverity::Info:
        Message.CachedColor = FLinearColor(0.92, 0.92, 0.92);
        break;
    case ESeverity::Notice:
        Message.CachedColor = FLinearColor(0.0, 0.667, 0);
        break;
    case ESeverity::Warning:
        Message.CachedColor = FLinearColor(0.949, 0.667, 0);
        break;
    case ESeverity::Error:
        Message.CachedColor = FLinearColor(0.9, 0, 0);
        break;
    }

    ChatManager->AddChatMessageToReceived(Message);

}

void AAutoSplittersSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion)
{
    mLoadedModVersion = mRunningModVersion;
	mSerializationVersion = EAutoSplittersSerializationVersion::Latest;
    mVersionMajor = mLoadedModVersion.Major;
    mVersionMinor = mLoadedModVersion.Minor;
    mVersionPatch = mLoadedModVersion.Patch;
}

void AAutoSplittersSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion)
{
    // if serialized properties are missing, they get zeroed out and we get our legacy version marker by copying them into mLoadedModVersion
    mLoadedModVersion = FVersion(mVersionMajor,mVersionMinor,mVersionPatch);
}

bool AAutoSplittersSubsystem::NeedTransform_Implementation()
{
    return false;
}

bool AAutoSplittersSubsystem::ShouldSave_Implementation() const
{
    return true;
}
