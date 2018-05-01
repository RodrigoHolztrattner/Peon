////////////////////////////////////////////////////////////////////////////////
// Filename: PeonWorker.cpp
////////////////////////////////////////////////////////////////////////////////
#include "PeonWorker.h"
#include "PeonSystem.h"
#include <chrono>

__InternalPeon::PeonWorker::PeonWorker() : m_MemoryAllocator(this)
{
}

__InternalPeon::PeonWorker::PeonWorker(const __InternalPeon::PeonWorker& other) : m_MemoryAllocator(this)
{
}

__InternalPeon::PeonWorker::~PeonWorker()
{
}

#ifdef JobWorkerDebug
static unsigned int JobWorkerId = 0;
#endif

// The thread local identifier and current job
thread_local int							CurrentLocalThreadIdentifier;
thread_local __InternalPeon::PeonJob*		CurrentThreadJob;
thread_local __InternalPeon::PeonWorker*	CurrentWorker = nullptr;

int __InternalPeon::PeonWorker::GetCurrentLocalThreadIdentifier()
{
	return CurrentLocalThreadIdentifier;
}

__InternalPeon::PeonWorker*__InternalPeon::PeonWorker::GetCurrentLocalThreadWorker()
{
	return CurrentWorker;
}

void __InternalPeon::PeonWorker::SetQueueSize(unsigned int _jobBufferSize)
{
	// Initialize our concurrent queue
    m_WorkQueue.Initialize(_jobBufferSize);
}

bool __InternalPeon::PeonWorker::Initialize(__InternalPeon::PeonSystem* _ownerSystem, int _threadId, bool _mainThread)
{
	// Set the thread id and owner system
	m_ThreadId = _threadId;
	m_OwnerSystem = _ownerSystem;

#ifdef JobWorkerDebug
	std::cout << "Thread with id: " << m_ThreadId << " created" << std::endl;
#endif

	// Check if this worker thread is the main one
	if (!_mainThread)
	{
		// Create the new thread
		new std::thread(&PeonWorker::ExecuteThreadAux, this);
	}
	else
	{
		// Set the current local thread identifier
		CurrentLocalThreadIdentifier = m_ThreadId;

		// Set the current worker
		CurrentWorker = this;
	}



	return true;
}

void __InternalPeon::PeonWorker::ExecuteThreadAux()
{
	// Set the global per thread id
	CurrentLocalThreadIdentifier = m_ThreadId;
	CurrentWorker = this;

	// Run the execute function
	while (true)
	{
		ExecuteThread(nullptr);
	}
}

unsigned int __InternalPeon::PeonWorker::FastRandomUnsignedInteger()
{
	m_Seed = (214013 * m_Seed + 2531011);
	return (m_Seed >> 16) & 0x7FFF;
}

bool __InternalPeon::PeonWorker::GetJob(__InternalPeon::PeonJob** _job)
{
	// Primeira coisa, vamos ver se conseguimos pegar algum work do nosso queue interno
	*_job = m_WorkQueue.Pop();
	if (*_job != nullptr)
	{
		// Conseguimos! Retornamos ele agora
		return true;
	}

	// Primeiramente pegamos um index aleatório de alguma thread e a array de threads
	unsigned int randomIndex = FastRandomUnsignedInteger() % m_OwnerSystem->GetTotalWorkers();
	__InternalPeon::PeonWorker* workers = m_OwnerSystem->GetJobWorkers();

	// Pegamos então o queue desta thread aleatória
	PeonStealingQueue* stolenQueue = workers[randomIndex].GetWorkerQueue();

	// Verificamos se não estamos roubando de nós mesmos
	if (stolenQueue == &m_WorkQueue)
	{
		return false;
	}

	// Roubamos então um work desta thread
	*_job = stolenQueue->Steal();
	if (*_job == nullptr)
	{
		// Não foi possível roubar um work desta thread, melhor parar por aqui!
		return false;
	}

	// Conseguimos roubar um work!
	return true;
}

__InternalPeon::PeonJob* __InternalPeon::PeonWorker::GetFreshJob()
{
    return m_WorkQueue.GetFreshJob();
}

__InternalPeon::PeonStealingQueue* __InternalPeon::PeonWorker::GetWorkerQueue()
{
    return &m_WorkQueue;
}

void __InternalPeon::PeonWorker::Yield()
{
    std::this_thread::yield();
	// std::this_thread::sleep_for(std::chrono::microseconds(1));
}

int __InternalPeon::PeonWorker::GetThreadId()
{
    return m_ThreadId;
}

void __InternalPeon::PeonWorker::ResetFreeList()
{
	m_WorkQueue.Reset();
}

void __InternalPeon::PeonWorker::RefreshMemoryAllocator()
{
	m_MemoryAllocator.ReleaseDeallocationChain();
}

__InternalPeon::PeonMemoryAllocator& __InternalPeon::PeonWorker::GetMemoryAllocator()
{
	return m_MemoryAllocator;
}

void __InternalPeon::PeonWorker::ExecuteThread(void* _arg)
{
	if (m_OwnerSystem->WorkerExecutionStatus())
	{
		Yield();
		return;
	}

	// Try to get a job
	__InternalPeon::PeonJob* job = nullptr;
	bool result = GetJob(&job);
	if (result)
	{

// If debug mode is on
#ifdef JobWorkerDebug

		// Print the function message
		printf("Thread with id %d will run a function", m_ThreadId);

#endif

		// Set the current job for this thread
		CurrentThreadJob = job;

		// Run the selected job
		job->RunJobFunction();

		// Finish the job
		job->Finish(this);
	}
	else
    {
        // Set an empty current job
        CurrentThreadJob = nullptr;

        // Give our time slice away
        Yield();
    }
}

__InternalPeon::PeonJob* __InternalPeon::PeonWorker::GetCurrentJob()
{
	return CurrentThreadJob;
}