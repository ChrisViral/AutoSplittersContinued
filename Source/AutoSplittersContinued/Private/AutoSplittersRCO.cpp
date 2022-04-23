#include "AutoSplittersRCO.h"

#include "AutoSplittersLog.h"
#include "Buildables/MFGBuildableAutoSplitter.h"

void UAutoSplittersRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAutoSplittersRCO,Dummy);
}

void UAutoSplittersRCO::SetOutputAutomatic_Implementation(AMFGBuildableAutoSplitter* Splitter, const int32 Output, const bool Automatic) const
{
    UE_LOG(LogAutoSplitters, Display, TEXT("Running client RPC: AMFGBuildableAutoSplitter::SetOutputAutomatic()"));
    Splitter->Server_SetOutputAutomatic(Output,Automatic);
}

void UAutoSplittersRCO::EnableReplication_Implementation(AMFGBuildableAutoSplitter* Splitter, const float Duration) const
{
    UE_LOG(LogAutoSplitters, Display, TEXT("Running client RPC: AMFGBuildableAutoSplitter::EnableReplication()"));
    Splitter->Server_EnableReplication(Duration);
}

void UAutoSplittersRCO::SetTargetInputRate_Implementation(AMFGBuildableAutoSplitter* Splitter, const float Rate) const
{
    UE_LOG(LogAutoSplitters, Display, TEXT("Running client RPC: AMFGBuildableAutoSplitter::SetTargetInputRate()"));
    Splitter->Server_SetTargetInputRate(Rate);
}

void UAutoSplittersRCO::SetTargetRateAutomatic_Implementation(AMFGBuildableAutoSplitter* Splitter, const bool Automatic) const
{
    UE_LOG(LogAutoSplitters, Display, TEXT("Running client RPC: AMFGBuildableAutoSplitter::SetTargetRateAutomatic()"));
    Splitter->Server_SetTargetRateAutomatic(Automatic);
}

void UAutoSplittersRCO::SetOutputRate_Implementation(AMFGBuildableAutoSplitter* Splitter, const int32 Output, const float Rate) const
{
    UE_LOG(LogAutoSplitters, Display, TEXT("Running client RPC: AMFGBuildableAutoSplitter::SetOutputRate()"));
    Splitter->Server_SetOutputRate(Output,Rate);
}

void UAutoSplittersRCO::BalanceNetwork_Implementation(AMFGBuildableAutoSplitter* Splitter, const bool RootOnly) const
{
    UE_LOG(LogAutoSplitters, Display, TEXT("Running client RPC: AMFGBuildableAutoSplitter::BalanceNetwork()"));
    Splitter->Server_BalanceNetwork(Splitter,RootOnly);
}
