#pragma once

#include "CoreMinimal.h"
#include "Module/GameInstanceModule.h"
#include "AutoSplittersGameInstanceModule.generated.h"


UCLASS(BlueprintType)
class AUTOSPLITTERSCONTINUED_API UAutoSplittersGameInstanceModule final : public UGameInstanceModule
{
    GENERATED_BODY()

public:
    virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
