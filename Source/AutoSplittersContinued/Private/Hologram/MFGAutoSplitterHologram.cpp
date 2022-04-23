// ILikeBanas

#include "Hologram/MFGAutoSplitterHologram.h"
#include "Buildables/MFGBuildableAutoSplitter.h"
#include "Buildables/FGBuildable.h"

#include "AutoSplittersLog.h"

void AMFGAutoSplitterHologram::ConfigureComponents(AFGBuildable* inBuildable) const
{

	AMFGBuildableAutoSplitter* Splitter = Cast<AMFGBuildableAutoSplitter>(inBuildable);

	if (HasAuthority() && IsUpgrade())
	{
		UE_LOG(LogAutoSplitters,Display,TEXT("Upgrading, working around broken connection handling of upstream hologram"));
		std::array<UFGFactoryConnectionComponent*,4> SnappedConnections;
		for (int i = 0 ; i < 4 ; ++i)
		{
			SnappedConnections[i] = mSnappedConnectionComponents[i];
			if (mSnappedConnectionComponents[i])
			{
				mSnappedConnectionComponents[i]->ClearConnection();
			}
			const_cast<AMFGAutoSplitterHologram*>(this)->mSnappedConnectionComponents[i] = nullptr;
		}

		// Skip AFGAttachmentSplitterHologram and AFGConveyorAttachmentHologram, those two completely
		// break in the update case
		AFGFactoryHologram::ConfigureComponents(inBuildable);

		TInlineComponentArray<UFGFactoryConnectionComponent*,4> Connections;
		Splitter->GetComponents(Connections);

		if (Connections.Num() != 4)
		{
			UE_LOG(LogAutoSplitters,Error,TEXT("Unexpected number of connections: %d"),Connections.Num());
			return;
		}

		for (int i = 0 ; i < 4 ; ++i)
		{
			if (Connections[i]->IsConnected())
			{
				UE_LOG(LogAutoSplitters,Warning,TEXT("Connection %d is connected but should not be"),i);
				Connections[i]->ClearConnection();
			}

			if (SnappedConnections[i])
			{
				Connections[i]->SetConnection(SnappedConnections[i]);
			}
		}
	}
	else
	{
		UE_LOG(LogAutoSplitters,Display,TEXT("Calling original implementation"));
		Super::ConfigureComponents(inBuildable);
	}

	if (mPreUpgradeConnections.Num() > 0)
	{
		UE_LOG(LogAutoSplitters,Display,TEXT("Reconnecting %d conveyors from removed pre-upgrade splitter"),mPreUpgradeConnections.Num());

		TInlineComponentArray<UFGFactoryConnectionComponent*,4> Connections;
		Splitter->GetComponents(Connections);

		std::array<UFGFactoryConnectionComponent*,4> Candidates = {nullptr};

		for (auto Conveyor : mPreUpgradeConnections)
		{
			auto ConveyorPos = Conveyor->GetComponentLocation();
			bool AmbiguousMatch = true;
			float MinDistance = INFINITY;
			int32 Candidate = -1;

			for (int32 i = 0 ; i < 4 ; ++i)
			{
				auto ConnectionPos = Connections[i]->GetComponentLocation();
				auto Distance = FVector::Dist(ConveyorPos,ConnectionPos);
				if (std::abs(Distance - MinDistance) < 40)
				{
					UE_LOG(LogAutoSplitters,Error,TEXT("Distance too small for unique detection"));
					AmbiguousMatch = true;
					continue;
				}
				if (Distance < MinDistance)
				{
					MinDistance = Distance;
					Candidate = i;
					AmbiguousMatch = false;
				}
			}

			if (AmbiguousMatch)
			{
				UE_LOG(LogAutoSplitters,Error,TEXT("Error: Could not find unambiguous match, skipping!"))
				continue;
			}

			if (Candidate < 0)
			{
				UE_LOG(LogAutoSplitters,Error,TEXT("Error: Could not find any candidate, skipping!"))
				continue;
			}

			if (Candidates[Candidate] != nullptr)
			{
				UE_LOG(LogAutoSplitters,Error,TEXT("Error: Best dandidate %d already assigned to different conveyor"),Candidate);
				continue;
			}

			UE_LOG(LogAutoSplitters,Display,TEXT("Candidate found : %d"))
			Candidates[Candidate] = Conveyor;
		}

		for (int32 i = 0 ; i < 4 ; ++i)
		{
			if (Candidates[i])
			{
				Candidates[i]->SetConnection(Connections[i]);
			}
		}

		Splitter->mNeedsInitialDistributionSetup = true;

	}
}
