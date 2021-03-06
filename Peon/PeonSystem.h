////////////////////////////////////////////////////////////////////////////////
// Filename: PeonSystem.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "PeonConfig.h"
#include <atomic>
#include <vector>
#include <cstdlib>
#include <new>
#include "PeonJob.h"
#include "PeonWorker.h"

/////////////
// DEFINES //
/////////////

////////////
// GLOBAL //
////////////

///////////////
// NAMESPACE //
///////////////

// __InternalPeon
PeonNamespaceBegin(__InternalPeon)

///////////////
// ALLOCATOR //
///////////////

// The allocator type
template <class T>
struct PeonAllocator
{
	typedef T value_type;
	PeonAllocator() = default;

	// The allocate method
	template <class U> constexpr PeonAllocator(const PeonAllocator<U>&) noexcept {}
	static [[nodiscard]] T* allocate(std::size_t n)
	{
		// Get the current worker thread in execution
		PeonWorker* currentWorker = PeonWorker::GetCurrentLocalThreadWorker();

		// Get the worker allocator
		auto& allocator = currentWorker->GetMemoryAllocator();

		// Allocate the data
		if (auto p = (T*)(allocator.AllocateData(currentWorker, sizeof(T) * n))) return p;

		throw std::bad_alloc();
	}

	// The deallocate method
	static void deallocate(T* p, std::size_t) noexcept
	{
		// Get the current worker thread in execution
		PeonWorker* currentWorker = PeonWorker::GetCurrentLocalThreadWorker();

		// Get the worker allocator
		auto& allocator = currentWorker->GetMemoryAllocator();

		// Deallocate the data
		allocator.DeallocateData((char*)(p));
	}

	// The deleter method
	void operator()(T* b)
	{
		// Get the current worker thread in execution
		PeonWorker* currentWorker = PeonWorker::GetCurrentLocalThreadWorker();

		// Get the worker allocator
		auto& allocator = currentWorker->GetMemoryAllocator();

		// Deallocate the data
		allocator.DeallocateData((char*)(b));
	}
};
template <class T, class U>
bool operator==(const PeonAllocator<T>&, const PeonAllocator<U>&) { return true; }
template <class T, class U>
bool operator!=(const PeonAllocator<T>&, const PeonAllocator<U>&) { return false; }

////////////////////////////////////////////////////////////////////////////////
// Class name: PeonSystem
////////////////////////////////////////////////////////////////////////////////
class PeonSystem
{
public:
	PeonSystem();
	PeonSystem(const PeonSystem&);
	~PeonSystem();

	inline unsigned int pow2roundup(unsigned int x)
    {
        if (x < 0)
            return 0;
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        return x+1;
    }

	// Initialize the job system
	template <class ThreadUserDatType = unsigned char>
	bool Initialize(unsigned int _numberWorkerThreads, unsigned int _jobBufferSize, bool _useUserData = false)
	{
		// Block all threads
		BlockThreadsStatus(true);

		// Make sure we are working with a buffer size power of 2
		_jobBufferSize = pow2roundup(_jobBufferSize);

		// Save the number of worker threads
		m_TotalWokerThreads = _numberWorkerThreads;

		// Alocate space for all threads
		m_JobWorkers = new PeonWorker[_numberWorkerThreads];

		// Set the queue size for each worker thread (WE CANT DO THIS AND INITIALIZE AT THE SAME TIME!)
		for (unsigned int i = 0; i < _numberWorkerThreads; i++)
		{
			m_JobWorkers[i].SetQueueSize(_jobBufferSize);
		}

		// Create the thread user data
		for (unsigned int i = 0; i < _numberWorkerThreads && _useUserData; i++)
		{
			// Create the thread user data
			ThreadUserDatType* threadUserData = new ThreadUserDatType();
			m_ThredUserData.push_back(threadUserData);
		}

		// Initialize all threads
		for (unsigned int i = 0; i < _numberWorkerThreads; i++)
		{
			m_JobWorkers[i].Initialize(this, i, i == 0); // Check for the first thread (the main one)
		}

		// Unblock all threads
		BlockThreadsStatus(false);

		return true;
	}

	// Return the thread user data by thread index
	template <class ThreadUserDatType>
	ThreadUserDatType* GetUserData(unsigned int _threadIndex)
	{
		return (ThreadUserDatType*)m_ThredUserData[_threadIndex];
	}

	// Return the thread user data by thread index
	template <class ThreadUserDatType>
	ThreadUserDatType* GetUserData()
	{
		return GetUserData(__InternalPeon::PeonWorker::GetCurrentLocalThreadIdentifier());
	}

	// Return the total worker threads
	unsigned int GetTotalWorkers();

	// Return the job worker array
	PeonWorker* GetJobWorkers();

	// Reset the actual worker frame
	void ResetWorkerFrame();

	// Job container creation helper
	void JobContainerHelper(void* _data) {}

	//////////////////////////////////
	// CONSIDERED STATIC BUT MEMBER //
	//////////////////////////////////

	// Create a job
	PeonJob* CreateJob(std::function<void()> _function);

	// Create a job as child for the current job in execution
	PeonJob* CreateChildJob(std::function<void()> _function);

	// Create a job as child for the given parent job
	PeonJob* CreateChildJob(PeonJob* _parentJob, std::function<void()> _function);

	// Create a container
	Container* CreateContainer();

	// Create a child container for the current job in execution
	Container* CreateChildContainer();

	// Create a child container for the given parent job
	Container* CreateChildContainer(PeonJob* _parentJob);

	// Run a job
	void StartJob(PeonJob* _job);

	// Wait for a job to continue
	void WaitForJob(PeonJob* _job);

	// Add a job dependency (remember to NOT start this job manually)
	void AddJobDependency(PeonJob* _thisFirst, PeonJob* _thenThis);

	///////////////////////
	// STATIC BUT MEMBER //
	///////////////////////

	// Return the current thread executing this code
	PeonWorker* GetCurrentPeon();

	// Return the default worker thread
	__InternalPeon::PeonWorker* GetDefaultWorkerThread();

	// Block, release and return the current worker execution status (allow/disallow worker threads to pick new jobs)
	void BlockWorkerExecution() { BlockThreadsStatus(true); }
	void ReleaseWorkerExecution() { BlockThreadsStatus(false); }
	bool WorkerExecutionStatus() { return ThreadsBlocked(); }

	// Return the current job for the actual context
	PeonJob* GetCurrentJob();

	// Return the current worker for the actual context
	PeonWorker* GetCurrentWorker();

	// Return the current worker index for the actual context
	int GetCurrentWorkerIndex();

protected:

	// Should not be used externally, set and check the thread block status
	void BlockThreadsStatus(bool _status);
	bool ThreadsBlocked();

private:

	// The total of worker threads
	unsigned int m_TotalWokerThreads;

	// The worker thread array
	__InternalPeon::PeonWorker* m_JobWorkers;

	// The thread user data
	std::vector<void*> m_ThredUserData;
};


// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
