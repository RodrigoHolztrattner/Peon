////////////////////////////////////////////////////////////////////////////////
// Filename: PeonJob.cpp
////////////////////////////////////////////////////////////////////////////////
#include "PeonJob.h"
#include "PeonWorker.h"
#include "PeonSystem.h"

///////////////
// NAMESPACE //
///////////////

__InternalPeon::PeonJob::PeonJob()
{
}

__InternalPeon::PeonJob::PeonJob(const PeonJob& other)
{
}

__InternalPeon::PeonJob::~PeonJob()
{
}

bool __InternalPeon::PeonJob::Initialize()
{
    // Set the initial data
    m_CurrentWorkerThread = nullptr;
    m_ParentJob = nullptr;
    m_UnfinishedJobs = 1;
	m_TotalJobsThatDependsOnThis = 0;

    return true;
}

void __InternalPeon::PeonJob::SetJobFunction(PeonJob* _parentJob, std::function<void()> _function)
{
    // Set the function and the data
    m_Function = _function;

    // Set the current worker thread
    m_ParentJob = _parentJob;
}

void __InternalPeon::PeonJob::Finish(PeonWorker* _peonWorker)
{
	// Decrement the number of unfinished jobs
	m_UnfinishedJobs--;
	const int32_t unfinishedJobs = m_UnfinishedJobs;

	// Check if there are no jobs remaining
	if (unfinishedJobs == 0)
	{
		// If we dont have any remaining jobs, we can decrement the number of jobs from our parent
		// (if we have one).
		if (m_ParentJob != nullptr)
		{
			// Call the finish job for our parent
			m_ParentJob->Finish(_peonWorker);
		}
		
		// Run follow-up jobs
		for (int32_t i = 0; i < m_TotalJobsThatDependsOnThis; ++i)
		{
			// Insert them on the queue
			_peonWorker->GetWorkerQueue()->Push(m_JobsThatDependsOnThis[i]);
		}
	}
}

void __InternalPeon::PeonJob::RunJobFunction()
{
    m_Function();
}

__InternalPeon::PeonJob* __InternalPeon::PeonJob::GetParent()
{
    return m_ParentJob;
}

void __InternalPeon::PeonJob::SetWorkerThread(PeonWorker* _workerThread)
{
    m_CurrentWorkerThread = _workerThread;
}

__InternalPeon::PeonWorker* __InternalPeon::PeonJob::GetWorkerThread()
{
    // Set the current job
    PeonJob* currentJob = this;

    // Check if this job is the root one
    while (currentJob->GetParent() != nullptr)
    {
        // Iterate until we find the root job
        currentJob = currentJob->GetParent();
    }

    return currentJob->m_CurrentWorkerThread;
}

void __InternalPeon::PeonJob::SetParentJob(PeonJob* _job)
{
    m_ParentJob = _job;
}

int32_t __InternalPeon::PeonJob::GetTotalUnfinishedJobs()
{
    return m_UnfinishedJobs;
}
