////////////////////////////////////////////////////////////////////////////////
// Filename: PeonJob.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "PeonConfig.h"
#include <atomic>
#include <functional>

/////////////
// DEFINES //
/////////////

// Return if a job is done
#define HasJobCompleted(job)	((job->GetTotalUnfinishedJobs()) <= 0)

///////////////
// NAMESPACE //
///////////////

// __InternalPeon
PeonNamespaceBegin(__InternalPeon)

// Classes we know
class PeonWorker;
class PeonSystem;

////////////
// GLOBAL //
////////////

////////////////////////////////////////////////////////////////////////////////
// Class name: PeonJob
////////////////////////////////////////////////////////////////////////////////
class PeonJob // alignas(32)
{
	// Friend classes
	friend PeonSystem;

public:
	PeonJob();
	PeonJob(const PeonJob&);
	~PeonJob();

	// Initialize the job
	bool Initialize();

	// Set the job function (syntax: (*MEMBER, &MEMBER::FUNCTION, DATA))
	void SetJobFunction(PeonJob* _parentJob, std::function<void()> _function);

	// Finish this job
	void Finish(PeonWorker* _peonWorker);

	// Run the job function
	void RunJobFunction();

	// Return the parent job
	PeonJob* GetParent();

	// Set the worker thread
	void SetWorkerThread(PeonWorker* _workerThread);

	// Return the worker thread
	PeonWorker* GetWorkerThread();

	// Set the parent job
	void SetParentJob(PeonJob* _job);

	// Return the number of unfinished jobs
	int32_t GetTotalUnfinishedJobs();

protected:

	// The job function and data
	std::function<void()> m_Function;

	// The parent job
	PeonJob* m_ParentJob;

	// The current worker thread
	PeonWorker* m_CurrentWorkerThread;

	// The total number of jobs that depends on this (and the job array)
	std::atomic<int32_t> m_TotalJobsThatDependsOnThis;
	PeonJob* m_JobsThatDependsOnThis[17];

public: // Arrumar public / private

	// The number of unfinished jobs
	std::atomic<int32_t> m_UnfinishedJobs;
};

// The container type
typedef PeonJob	Container;

// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
