////////////////////////////////////////////////////////////////////////////////
// Filename: PeonSystem.cpp
////////////////////////////////////////////////////////////////////////////////

#include "PeonSystem.h"
#include "PeonWorker.h"

__InternalPeon::PeonSystem::PeonSystem()
{
	// Set the initial data
	__InternalPeon::PeonSystem::m_JobWorkers = nullptr;
}

__InternalPeon::PeonSystem::PeonSystem(const __InternalPeon::PeonSystem& other)
{
}

__InternalPeon::PeonSystem::~PeonSystem()
{
}

__InternalPeon::PeonJob* __InternalPeon::PeonSystem::CreateJob(std::function<void()> _function)
{
    // Get the default worker thread
    PeonWorker* workerThread = PeonSystem::GetCurrentPeon();

    // Get a fresh job
    PeonJob* freshJob = workerThread->GetFreshJob();

    // Initialize the job
    freshJob->Initialize();

    // Set the job worker thread
    freshJob->SetWorkerThread(workerThread);

    // Set the job function
    freshJob->SetJobFunction(nullptr, _function);

    // Return the new job
    return freshJob;
}

__InternalPeon::PeonJob* __InternalPeon::PeonSystem::CreateChildJob(PeonJob* _parentJob, std::function<void()> _function)
{
    // Não preciso me preocupar com o parent job sendo deletado ou liberando waits caso exista concorrencia pois seguimos da seguinte lógica, apenas o job atual pode criar jobs
    // filhos, e pode apenas adicionar elas para si mesmo, logo se estamos aqui quer dizer que o parent job ainda tem no minimo um trabalho restante (que é adicionar esse job)
    // e por consequencia ele não será deletado ou liberará algum wait.

    // Atomic increment the number of unfinished jobs of our parent
    _parentJob->m_UnfinishedJobs++;

    // Get the worker thread from the parent
    PeonWorker* workerThread = PeonSystem::GetCurrentPeon();

    // Get a fresh job
    PeonJob* freshJob = workerThread->GetFreshJob();

    // Initialize the job
    freshJob->Initialize();

    // Set the job function
    freshJob->SetJobFunction(_parentJob, _function);

    // Set the job parent
    freshJob->SetParentJob(_parentJob);

    // Return the new job
    return freshJob;
}

__InternalPeon::PeonJob* __InternalPeon::PeonSystem::CreateChildJob(std::function<void()> _function)
{
	return CreateChildJob(PeonWorker::GetCurrentJob(), _function);
}

__InternalPeon::Container* __InternalPeon::PeonSystem::CreateContainer()
{
	return CreateJob([=] { JobContainerHelper(nullptr); });
}

void __InternalPeon::PeonSystem::StartJob(__InternalPeon::PeonJob* _job)
{
	// Get the worker thread for this job
	__InternalPeon::PeonWorker* workerThread = _job->GetWorkerThread();

	// Insert the job into the worker thread queue
	workerThread->GetWorkerQueue()->Push(_job);
}

void __InternalPeon::PeonSystem::WaitForJob(__InternalPeon::PeonJob* _job)
{
	// Get the job worker thread
	__InternalPeon::PeonWorker* workerThread = _job->GetWorkerThread();

	// wait until the job has completed. in the meantime, work on any other job.
	while (!HasJobCompleted(_job))
	{
		// Try to preempt another job (or just yield)
		workerThread->ExecuteThread(nullptr);
	}
}

__InternalPeon::PeonWorker* __InternalPeon::PeonSystem::GetCurrentPeon()
{
	int currentThreadIdentifier = __InternalPeon::PeonWorker::GetCurrentLocalThreadIdentifier();
	return &PeonSystem::m_JobWorkers[currentThreadIdentifier];
}

__InternalPeon::PeonWorker* __InternalPeon::PeonSystem::GetDefaultWorkerThread()
{
	return &m_JobWorkers[0];
}

__InternalPeon::PeonJob* __InternalPeon::PeonSystem::GetCurrentJob()
{
	return __InternalPeon::PeonWorker::GetCurrentJob();
}

__InternalPeon::PeonWorker* __InternalPeon::PeonSystem::GetCurrentWorker()
{
	return __InternalPeon::PeonSystem::GetCurrentPeon();
}

int __InternalPeon::PeonSystem::GetCurrentWorkerIndex()
{
	return __InternalPeon::PeonWorker::GetCurrentLocalThreadIdentifier();
}

unsigned int __InternalPeon::PeonSystem::GetTotalWorkers()
{
	return m_TotalWokerThreads;
}

__InternalPeon::PeonWorker* __InternalPeon::PeonSystem::GetJobWorkers()
{
	return m_JobWorkers;
}

void __InternalPeon::PeonSystem::ResetWorkerFrame()
{
	// For each worker
	for (unsigned int i = 0; i < m_TotalWokerThreads; i++)
	{
		// Reset the free list
		m_JobWorkers[i].ResetFreeList();
	}
}

bool threadsBlocked = false;

void __InternalPeon::PeonSystem::BlockThreadsStatus(bool _status)
{
	threadsBlocked = _status;
}

bool __InternalPeon::PeonSystem::ThreadsBlocked()
{
	return threadsBlocked;
}

