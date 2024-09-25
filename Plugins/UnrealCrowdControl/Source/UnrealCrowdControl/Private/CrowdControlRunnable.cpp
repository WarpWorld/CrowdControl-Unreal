
#include "CrowdControlRunable.h"
#include "CrowdControlSubsystem.h"


uint32 FCrowdControlRunnable::Run()
{
	{
		if (Manager && Manager->CC_CrowdControlFunction)
		{
			UE_LOG(LogCrowdControl, Log, TEXT("Running CrowdControlFunction"));
			Manager->CC_CrowdControlFunction();
		}
		return 0;
	}
}

void FCrowdControlRunnable::EnsureCompletion()
{
	if (Thread != nullptr)
	{
		UE_LOG(LogCrowdControl, Log, TEXT("About to Stop"));
		Stop();
			
		Manager->CC_DisconnectFunction();
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;

		UE_LOG(LogCrowdControl, Log, TEXT("Thread successfully stopped and deleted"));
	}
}
