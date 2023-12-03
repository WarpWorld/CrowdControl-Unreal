#include "CrowdControl.h"
#include "CCManager.h"
#include <mutex>

class FCrowdControlRunnable : public FRunnable
{
private:
    ACCManager* Manager;
    FRunnableThread* Thread; 
    bool bRequestStop; // Standard bool for stop request
    std::mutex StopMutex; // Mutex for thread-safe access to bRequestStop

public:
    FCrowdControlRunnable(ACCManager* InManager)
        : Manager(InManager), Thread(nullptr), bRequestStop(false)
    {
    }

    virtual ~FCrowdControlRunnable() {
        EnsureCompletion();
    }
	
	void RedirectTest() {
		UE_LOG(LogTemp, Error, TEXT("HERE"));
	}

    virtual bool Init() override
    {
        return true;
    }

    virtual uint32 Run() override
    {
        if (Manager && Manager->CrowdControlFunction)
        {
			UE_LOG(LogTemp, Warning, TEXT("Running CrowdControlFunction"));
			Manager->CrowdControlFunction();
        }
        return 0;
    }

    virtual void Stop() override
    {
        std::lock_guard<std::mutex> lock(StopMutex);
        bRequestStop = true;
    }

    void FCrowdControlRunnable::EnsureCompletion() {
		if (Thread != nullptr) {
			UE_LOG(LogTemp, Warning, TEXT("About to Stop"));
			Stop();
			
			Manager->DisconnectFunction();
			Thread->WaitForCompletion();
			delete Thread;
			Thread = nullptr;

			UE_LOG(LogTemp, Warning, TEXT("Thread successfully stopped and deleted"));
		}
	}

    void StartThread()
    {
        if (Thread == nullptr)
        {
            {
                std::lock_guard<std::mutex> lock(StopMutex);
                bRequestStop = false;
            }
            Thread = FRunnableThread::Create(this, TEXT("CrowdControlThread"), 0, TPri_BelowNormal);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Thread already running!"));
        }
    }
	
	void TestRedirect() {
		UE_LOG(LogTemp, Warning, TEXT("REDIRECTED!"));
	}
};
