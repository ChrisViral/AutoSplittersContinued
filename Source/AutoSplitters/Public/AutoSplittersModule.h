#pragma once

// define to 1 to get more debug output to console when the debug flag is set in the splitter UI
#define AUTO_SPLITTERS_DEBUG 1

#include <FGFactoryConnectionComponent.h>

#include "Buildables/MFGBuildableAutoSplitter.h"
#include "Modules/ModuleManager.h"

class FAutoSplittersModule : public IModuleInterface
{
	friend class AMFGBuildableAutoSplitter;
	friend class AAutoSplittersSubsystem;

	TArray<
		std::tuple<
			AMFGBuildableAutoSplitter*,
			TInlineComponentArray<UFGFactoryConnectionComponent*, 2>,
			TInlineComponentArray<UFGFactoryConnectionComponent*, 4>
		>
	> mPreComponentFixSplitters;

	int32 mLoadedSplitterCount;

	TArray<AMFGBuildableAutoSplitter*> mDoomedSplitters;

	void OnSplitterLoadedFromSaveGame(AMFGBuildableAutoSplitter* Splitter);
	void ScheduleDismantle(AMFGBuildableAutoSplitter* Splitter);

	bool HaveLoadedSplitters() const
	{
		return mLoadedSplitterCount > 0;
	}

	void ReplacePreComponentFixSplitters(UWorld* World, AAutoSplittersSubsystem* AutoSplittersSubsystem);

public:

	static const bool IsAlphaVersion = true;

	virtual void StartupModule() override;

	static const FName ModReference;

	static FAutoSplittersModule* Get()
	{
		return FModuleManager::GetModulePtr<FAutoSplittersModule>(ModReference);
	}
};
