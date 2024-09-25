#pragma once


#include <mutex>

#include "CrowdControlLogChannels.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"


class UCrowdControlSubsystem;

class FCrowdControlRunnable : public FRunnable
{
private:
    UCrowdControlSubsystem* Manager;
    FRunnableThread* Thread; 
    bool bRequestStop; // Standard bool for stop request
    std::mutex StopMutex; // Mutex for thread-safe access to bRequestStop

public:
    FCrowdControlRunnable(UCrowdControlSubsystem* InManager)
        : Manager(InManager), Thread(nullptr), bRequestStop(false)
    {
    }

    virtual ~FCrowdControlRunnable() override {
        EnsureCompletion();
    }
	
	void RedirectTest() {
		UE_LOG(LogCrowdControl, Error, TEXT("HERE"));
	}

    virtual bool Init() override
    {
        return true;
    }

    virtual uint32 Run() override;

    virtual void Stop() override
    {
        std::lock_guard<std::mutex> lock(StopMutex);
        bRequestStop = true;
    }

    void EnsureCompletion();

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
            UE_LOG(LogCrowdControl, Warning, TEXT("Thread already running!"));
        }
    }
	
	void TestRedirect() {
		UE_LOG(LogCrowdControl, Warning, TEXT("REDIRECTED!"));
	}
};