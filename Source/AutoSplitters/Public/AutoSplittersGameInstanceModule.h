// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Module/GameInstanceModule.h"

#include "AutoSplittersGameInstanceModule.generated.h"

/**
 *
 */
UCLASS(BlueprintType)
class AUTOSPLITTERS_API UAutoSplittersGameInstanceModule : public UGameInstanceModule
{
    GENERATED_BODY()

public:

    virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;
};
