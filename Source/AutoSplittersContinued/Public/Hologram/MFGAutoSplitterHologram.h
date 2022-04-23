// ILikeBanas

#pragma once

#include <array>

#include "CoreMinimal.h"
#include "Hologram/FGAttachmentSplitterHologram.h"
#include "AutoSplittersContinuedModule.h"

#include "MFGAutoSplitterHologram.generated.h"

/**
 *
 */
UCLASS()
class AUTOSPLITTERSCONTINUED_API AMFGAutoSplitterHologram : public AFGAttachmentSplitterHologram
{
	GENERATED_BODY()

	friend class FAutoSplittersContinuedModule;

	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObjectsInContainer
	TArray<UFGFactoryConnectionComponent*> mPreUpgradeConnections;

protected:

	virtual void ConfigureComponents( class AFGBuildable* inBuildable ) const override;

};
