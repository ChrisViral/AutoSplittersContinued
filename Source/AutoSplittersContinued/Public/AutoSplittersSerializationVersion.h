#pragma once

#include "CoreMinimal.h"
#include "AutoSplittersSerializationVersion.generated.h"

UENUM(BlueprintType)
enum class EAutoSplittersSerializationVersion : uint8
{

    // initial version - no tracking at global level
    Legacy = 0,

    // first version, for 0.3.x compatibility
    FixedPrecisionArithmetic,

    // moved replicated properties to nested struct
    NestedReplicationStruct,

    // keep at the bottom of the list
    VersionPlusOne,
    Latest = VersionPlusOne - 1

};
