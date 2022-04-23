#include "AutoSplittersGameInstanceModule.h"

#include "Registry/RemoteCallObjectRegistry.h"
#include "AutoSplittersLog.h"
#include "AutoSplittersRCO.h"

void UAutoSplittersGameInstanceModule::DispatchLifecycleEvent(const ELifecyclePhase Phase)
{
    Super::DispatchLifecycleEvent(Phase);
    switch(Phase)
    {
        case ELifecyclePhase::CONSTRUCTION:
        {
            UE_LOG(LogAutoSplitters,Display,TEXT("Registering AutoSplittersRCO object with RemoteCallObjectRegistry"));
            URemoteCallObjectRegistry* RCORegistry = GetGameInstance()->GetSubsystem<URemoteCallObjectRegistry>();
            RCORegistry->RegisterRemoteCallObject(UAutoSplittersRCO::StaticClass());
            break;
        }

        default:
            break;
    }
}
