
#include "CrowdControlRunable.h"
#include "CrowdControlSubsystem.h"


uint32 FCrowdControlRunnable::Run()
{
	{
		if (Manager && Manager->CC_CrowdControlFunction)
		{
			UE_LOG(LogCrowdControl, Log, TEXT("Running CrowdControlFunction"));
			if(Manager->CC_CrowdControlFunction() < 0)
			{
				UE_LOG(LogCrowdControl, Error, TEXT("Running CrowdControlFunction failed!"));
			}
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
