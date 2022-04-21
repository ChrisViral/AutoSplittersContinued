#pragma once

#include "CoreMinimal.h"

#include "FGPlayerController.h"
#include "FGRemoteCallObject.h"

#include "AutoSplittersRCO.generated.h"

class AMFGBuildableAutoSplitter;

/**
 *
 */
UCLASS(NotBlueprintable)
class AUTOSPLITTERS_API UAutoSplittersRCO : public UFGRemoteCallObject
{
    GENERATED_BODY()

public:

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    static UAutoSplittersRCO* Get(UWorld* World)
    {
        return Cast<UAutoSplittersRCO>(
            Cast<AFGPlayerController>(World->GetFirstPlayerController())->
                GetRemoteCallObjectOfClass(UAutoSplittersRCO::StaticClass())
            );
    }

    UFUNCTION(Server,Unreliable)
    void EnableReplication(AMFGBuildableAutoSplitter* Splitter, float Duration) const;

    UFUNCTION(Server,Reliable)
    void SetTargetRateAutomatic(AMFGBuildableAutoSplitter* Splitter, bool Automatic) const;

    UFUNCTION(Server,Reliable)
    void SetTargetInputRate(AMFGBuildableAutoSplitter* Splitter, float Rate) const;

    UFUNCTION(Server,Reliable)
    void SetOutputRate(AMFGBuildableAutoSplitter* Splitter, int32 Output, float Rate) const;

    UFUNCTION(Server,Reliable)
    void SetOutputAutomatic(AMFGBuildableAutoSplitter* Splitter, int32 Output, bool Automatic) const;

    UFUNCTION(Server,Reliable)
    void BalanceNetwork(AMFGBuildableAutoSplitter* Splitter, bool RootOnly) const;

private:

    UPROPERTY(Replicated)
    int32 Dummy;
};
