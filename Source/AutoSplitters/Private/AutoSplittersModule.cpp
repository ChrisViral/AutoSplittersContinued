#include "AutoSplittersModule.h"

#include "Patching/NativeHookManager.h"

#include "UI/FGPopupWidget.h"
#include "FGWorldSettings.h"
#include "FGPlayerController.h"
#include "FGBlueprintFunctionLibrary.h"
#include "FGBuildableSubsystem.h"
#include "Buildables/MFGBuildableAutoSplitter.h"
#include "Hologram/MFGAutoSplitterHologram.h"
#include "AutoSplittersLog.h"
#include "Engine/RendererSettings.h"
#include "Subsystem/AutoSplittersSubsystem.h"
#include "Registry/ModContentRegistry.h"
#include "Resources/FGBuildingDescriptor.h"
#include "ModLoading/PluginModuleLoader.h"

// #pragma optimize( "", off )

void FAutoSplittersModule::OnSplitterLoadedFromSaveGame(AMFGBuildableAutoSplitter* Splitter)
{
	// just record that we have loaded a splitter, needed e.g. for detection of legacy save game loading
	++mLoadedSplitterCount;
}

void FAutoSplittersModule::ScheduleDismantle(AMFGBuildableAutoSplitter* Splitter)
{
	if (mLoadedSplitterCount == 0)
	{
		UE_LOG(LogAutoSplitters,Fatal,TEXT("ScheduleDismantle() may only be called while loaded splitters are pending"));
	}
	mDoomedSplitters.Add(Splitter);
}

void FAutoSplittersModule::ReplacePreComponentFixSplitters(UWorld* World, AAutoSplittersSubsystem* AutoSplittersSubsystem)
{
	const auto& Config = AutoSplittersSubsystem->mConfig;

	UE_LOG(LogAutoSplitters, Display, TEXT("Found %d pre-upgrade AutoSplitters while loading savegame"),
	       mPreComponentFixSplitters.Num());

	if (Config.Upgrade.RemoveAllConveyors)
	{
		UE_LOG(LogAutoSplitters, Display,
		       TEXT("User has chosen nuclear upgrade option of removing all conveyors attached to Auto Splitters"));
	}

	auto ModContentRegistry = AModContentRegistry::Get(World);
	auto BuildableSubSystem = AFGBuildableSubsystem::Get(World);

	UFGRecipe* AutoSplitterRecipe = nullptr;
	for (auto& RecipeInfo : ModContentRegistry->GetRegisteredRecipes())
	{
		if (RecipeInfo.OwnedByModReference != FName("AutoSplitters"))
			continue;
		auto Recipe = Cast<UFGRecipe>(RecipeInfo.RegisteredObject->GetDefaultObject());
		if (Recipe->mProduct.Num() != 1)
			continue;
		auto BuildingDescriptor = Recipe->mProduct[0].ItemClass->GetDefaultObject<UFGBuildingDescriptor>();
		if (!BuildingDescriptor)
			continue;

		UE_LOG(LogAutoSplitters, Display, TEXT("Found building descriptor: %s"),
		       *BuildingDescriptor->GetClass()->GetName());

		if (UFGBuildingDescriptor::GetBuildableClass(BuildingDescriptor->GetClass())->IsChildOf(
			AMFGBuildableAutoSplitter::StaticClass()))
		{
			UE_LOG(LogAutoSplitters, Display, TEXT("Found AutoSplitter recipe to use for rebuilt splitters"));
			AutoSplitterRecipe = Recipe;
			break;
		}
	}

	if (!AutoSplitterRecipe)
	{
		UE_LOG(LogAutoSplitters, Fatal,
		       TEXT("Error: Could not find AutoSplitter recipe, unable to upgrade old Autosplitters"));
	}

	TSet<AFGBuildableConveyorBase*> Conveyors;
	for (auto& [Splitter,PreUpgradeComponents,ConveyorConnections] : mPreComponentFixSplitters)
	{
		UE_LOG(LogAutoSplitters, Display, TEXT("Replacing AutoSplitter %s"), *Splitter->GetName());

		auto Location = Splitter->GetActorLocation();
		auto Transform = Splitter->GetTransform();
		IFGDismantleInterface::Execute_Dismantle(Splitter);

		for (auto Component : PreUpgradeComponents)
		{
			Component->DestroyComponent();
		}

		UE_LOG(LogAutoSplitters, Display, TEXT("Creating and setting up hologram"),);

		auto Hologram = Cast<AMFGAutoSplitterHologram>(
			AFGHologram::SpawnHologramFromRecipe(
				AutoSplitterRecipe->GetClass(),
				World->GetFirstPlayerController(),
				Location
			)
		);

		Hologram->SetActorTransform(Transform);
		if (Config.Upgrade.RemoveAllConveyors)
		{
			for (auto Connection : ConveyorConnections)
			{
				auto Conveyor = Cast<AFGBuildableConveyorBase>(Connection->GetOuterBuildable());
				if (!Conveyor)
				{
					UE_LOG(LogAutoSplitters, Warning,
					       TEXT("Found something connected to a splitter that is not a conveyor: %s"),
					       *Connection->GetOuterBuildable()->GetClass()->GetName())
					break;
				}
				Conveyors.Add(Conveyor);
			}
		}
		else
		{
			Hologram->mPreUpgradeConnections = ConveyorConnections;
		}

		UE_LOG(LogAutoSplitters, Display, TEXT("Spawning Splitter through hologram"));
		TArray<AActor*> Children;
		auto Actor = Hologram->Construct(Children, BuildableSubSystem->GetNewNetConstructionID());
		UE_LOG(LogAutoSplitters, Display, TEXT("Destroying Hologram"));
		Hologram->Destroy();
	}

	if (Config.Upgrade.RemoveAllConveyors)
	{
		UE_LOG(LogAutoSplitters, Display, TEXT("Dismantling %d attached conveyors"), Conveyors.Num());
		for (auto Conveyor : Conveyors)
		{
			IFGDismantleInterface::Execute_Dismantle(Conveyor);
		}
	}

	if (Config.Upgrade.ShowWarningMessage)
	{
		FString Str;

		if (Config.Upgrade.RemoveAllConveyors)
		{
			Str = FString::Printf(TEXT(
				"Your savegame contained %d Auto Splitters created with versions of the mod older than 0.3.0, "
				"which connect to the attached conveyors in a wrong way. The mod has replaced these Auto Splitters with new ones, but because "
				"you have selected the mod configuration option \"Remove Conveyors\", all conveyors attached to Auto Splitters have been dismantled. "
				"\n\nAll replaced splitters have been reset to fully automatic mode."
				"\n\nA total of %d conveyors have been removed."), mPreComponentFixSplitters.Num(), Conveyors.Num());
		}
		else
		{
			Str = FString::Printf(TEXT(
				"Your savegame contained %d Auto Splitters created with versions of the mod older than 0.3.0, "
				"which connect to the attached conveyors in a wrong way. The mod has replaced these Auto Splitters with new ones. "
				"\n\nUnfortunately, it is not possible to carry over any manual settings for those splitters, and they are now all in fully automatic mode"),
			                      mPreComponentFixSplitters.Num());
		}

		AFGPlayerController* LocalController = UFGBlueprintFunctionLibrary::GetLocalPlayerController(World);

		FPopupClosed CloseDelegate;

		UFGBlueprintFunctionLibrary::AddPopupWithCloseDelegate(
			LocalController,
			FText::FromString("Savegame upgraded from legacy version < 0.3.0"),
			FText::FromString(Str),
			CloseDelegate
		);
	}
}

void FAutoSplittersModule::StartupModule()
{

#if UE_BUILD_SHIPPING

	auto UpgradeHook = [](auto& Call, UObject* self, AActor* newActor)
	{

		if (!newActor->HasAuthority())
			return;

		UE_LOG(LogAutoSplitters,Display,TEXT("Entered hook for IFGDismantleInterface::Execute_Upgrade()"));

		AMFGBuildableAutoSplitter* Target = Cast<AMFGBuildableAutoSplitter>(newActor);
		if (!Target)
		{
			UE_LOG(LogAutoSplitters,Display,TEXT("Target is not an AMFGBuildableAutoSplitter, bailing out"));
			return;
		}
		AFGBuildableAttachmentSplitter* Source = Cast<AFGBuildableAttachmentSplitter>(self);
		if (!Source)
		{
			UE_LOG(LogAutoSplitters,Display,TEXT("self is not an AFGBuildableAttachmentSplitter, bailing out"));
			return;
		}

		UE_LOG(LogAutoSplitters,Display,TEXT("Cancelling original call"));
		Call.Cancel();
	};

	SUBSCRIBE_METHOD(IFGDismantleInterface::Execute_Upgrade,UpgradeHook);


	auto NotifyBeginPlayHook = [&](AFGWorldSettings* WorldSettings)
	{

		if (!WorldSettings->HasAuthority())
		{
			UE_LOG(LogAutoSplitters,Display,TEXT("Not running on server, skipping"));
			return;
		}

		auto World = WorldSettings->GetWorld();

		if (FPluginModuleLoader::IsMainMenuWorld(World) || !FPluginModuleLoader::ShouldLoadModulesForWorld(World))
		{
			UE_LOG(LogAutoSplitters,Display,TEXT("Ignoring main menu world"));
			return;
		}

		auto AutoSplittersSubsystem = AAutoSplittersSubsystem::Get(WorldSettings);

		if (AutoSplittersSubsystem->IsNewSession())
		{
			UE_LOG(LogAutoSplitters,Display,TEXT("Newly created game session detected, no compability issues expected"));
		}

		if (mPreComponentFixSplitters.Num() > 0)
		{
			ReplacePreComponentFixSplitters(World,AutoSplittersSubsystem);
		}

		if (mDoomedSplitters.Num() > 0)
		{
			UE_LOG(
				LogAutoSplitters,
				Error,
				TEXT("Removing %d splitters with an incompatible serialization version"),
				mDoomedSplitters.Num()
				);

			for (auto Splitter : mDoomedSplitters)
			{
				IFGDismantleInterface::Execute_Dismantle(Splitter);
			}

			AutoSplittersSubsystem->NotifyChat(
				EAAutoSplittersSubsystemSeverity::Error,
				FString::Printf(
					TEXT("Savegame created with version %s that uses unknown serialization version %d, removed %d incompatible Auto Splitters"),
					*AutoSplittersSubsystem->GetLoadedModVersion().ToString(),
					AutoSplittersSubsystem->GetSerializationVersion(),
					mDoomedSplitters.Num()
					)
				);
		}
		else if (AutoSplittersSubsystem->IsModOlderThanSaveGame())
		{
			AutoSplittersSubsystem->NotifyChat(
				EAAutoSplittersSubsystemSeverity::Warning,
				FString::Printf(
					TEXT("Running %s, but the savegame was created with %s"),
					*AutoSplittersSubsystem->GetRunningModVersion().ToString(),
					*AutoSplittersSubsystem->GetLoadedModVersion().ToString()
					)
				);
		}
		else if (AutoSplittersSubsystem->GetSerializationVersion() < EAutoSplittersSerializationVersion::Latest)
		{
			AutoSplittersSubsystem->NotifyChat(
				EAAutoSplittersSubsystemSeverity::Notice,
				FString::Printf(
					TEXT("Now running %s, downgrade to previous version %s will not be possible"),
					*AutoSplittersSubsystem->GetRunningModVersion().ToString(),
					*AutoSplittersSubsystem->GetLoadedModVersion().ToString()
					)
				);
		}
		else if (AutoSplittersSubsystem->GetLoadedModVersion().Compare(AutoSplittersSubsystem->GetRunningModVersion()) < 0)
		{
			AutoSplittersSubsystem->NotifyChat(
				EAAutoSplittersSubsystemSeverity::Info,
				FString::Printf(
					TEXT("Upgraded from %s to %s"),
					*AutoSplittersSubsystem->GetLoadedModVersion().ToString(),
					*AutoSplittersSubsystem->GetRunningModVersion().ToString()
					)
				);
		}

		if (IsAlphaVersion)
		{
			if (AutoSplittersSubsystem->GetConfig().Preferences.ShowAlphaWarning)
			{
				AFGPlayerController* LocalController = UFGBlueprintFunctionLibrary::GetLocalPlayerController(World);

				FPopupClosed CloseDelegate;

				UFGBlueprintFunctionLibrary::AddPopupWithCloseDelegate(
					LocalController,
					FText::FromString("AutoSplitters Alpha Version"),
					FText::FromString("You are running an alpha version of AutoSplitters. There WILL be bugs, so keep you old save games around! "\
						"You can really help making the mod more stable by reporting bugs on the Modding Discord."\
						"\n\nIf you don't wnat to see this message anymore, you can disable it in the mod settings."),
					CloseDelegate
				);

			}
			else
			{
				AutoSplittersSubsystem->NotifyChat(
					EAAutoSplittersSubsystemSeverity::Warning,
					FString::Printf(
						TEXT("ALPHA version %s. Please report bugs on the modding Discord."),
						*AutoSplittersSubsystem->GetRunningModVersion().ToString()
						)
					);
			}
		}

		mLoadedSplitterCount = 0;
		mPreComponentFixSplitters.Empty();
		mDoomedSplitters.Empty();
	};


	void* SampleInstance = GetMutableDefault<AFGWorldSettings>();

	SUBSCRIBE_METHOD_VIRTUAL_AFTER(AFGWorldSettings::NotifyBeginPlay,SampleInstance,NotifyBeginPlayHook);

#endif // UE_BUILD_SHIPPING

}

const FName FAutoSplittersModule::ModReference("AutoSplitters");

// #pragma optimize( "", on )

IMPLEMENT_GAME_MODULE(FAutoSplittersModule,AutoSplitters);
