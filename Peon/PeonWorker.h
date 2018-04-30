////////////////////////////////////////////////////////////////////////////////
// Filename: PeonWorker.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include <thread>

#include <iostream>
#include <chrono>
#include <thread>

#include "PeonJob.h"
#include "PeonStealingQueue.h"
#include "PeonMemoryAllocator.h"

/////////////
// DEFINES //
/////////////

// The debug flag
// #define JobWorkerDebug

////////////
// GLOBAL //
////////////

///////////////
// NAMESPACE //
///////////////

// __InternalPeon
PeonNamespaceBegin(__InternalPeon)

// We know the job system
class PeonSystem;

////////////////////////////////////////////////////////////////////////////////
// Class name: PeonWorker
////////////////////////////////////////////////////////////////////////////////
class PeonWorker
{
public:
	PeonWorker();
	PeonWorker(const PeonWorker&);
	~PeonWorker();

public:

	// Get the current local thread identifier
	static int GetCurrentLocalThreadIdentifier();

	// Get the current worker
	static PeonWorker* GetCurrentLocalThreadWorker();

public:

	// Set the queue size
	void SetQueueSize(unsigned int _jobBufferSize);

	// Initialize this worker thread
	bool Initialize(PeonSystem* _ownerSystem, int _threadId, bool _mainThread = false);

	// Execute this thread
	void ExecuteThread(void* _arg);

	// Try to get a job from the current worker thread, or try to steal one from the others
	bool GetJob(PeonJob** _job);

    // Return this PeonWorker work queue
	PeonStealingQueue* GetWorkerQueue();

	// Yield the current time slice
	void Yield();

	// Return the thread id
	int GetThreadId();

	// Return a fresh (usable) job
	PeonJob* GetFreshJob();

	// Reset the free list
	void ResetFreeList();

	// Return a reference to our memory allocator
	PeonMemoryAllocator& GetMemoryAllocator();

public:

    // The aux execute thread
	void ExecuteThreadAux();

    // Return the job this worker ir working now
	static PeonJob* GetCurrentJob();

private:

	// A fast random uint generator
	unsigned int FastRandomUnsignedInteger();

///////////////
// VARIABLES //
private: //////

	// The owner (job system)
	PeonSystem* m_OwnerSystem;

	// Array of jobs
	PeonStealingQueue m_WorkQueue;

	// The memory allocator for this worker
	PeonMemoryAllocator m_MemoryAllocator;

	// The internal thread id
	unsigned int m_ThreadId;

	// A seed for our fast random unsigned integer generator
	unsigned int m_Seed;
};

// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
