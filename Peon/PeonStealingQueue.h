////////////////////////////////////////////////////////////////////////////////
// Filename: PeonStealingQueue.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "PeonConfig.h"
#include "PeonJob.h"
#include <vector>
#include <cstdint>
#include <atomic>
#include <mutex>

///////////////
// NAMESPACE //
///////////////

// __InternalPeon
PeonNamespaceBegin(__InternalPeon)

/////////////
// DEFINES //
/////////////

////////////
// GLOBAL //
////////////

////////////////////////////////////////////////////////////////////////////////
// Class name: PeonStealingQueue
////////////////////////////////////////////////////////////////////////////////
class PeonStealingQueue
{
public:
	PeonStealingQueue();
	PeonStealingQueue(const PeonStealingQueue&);
	~PeonStealingQueue();

	// Initialize the work stealing queue
	bool Initialize(unsigned int _bufferSize);

	// Return a valid job from our ring buffer
	PeonJob* GetFreshJob();

    // Insert a job into this queue (must be called only by the owner thread)
	void Push(PeonJob* _job);

    // Get a job from this queue (must be called only by the owner thread)
	PeonJob* Pop();

    // Try to steal a job from this queue (can be called from any thread)
	PeonJob* Steal();

    // Reset this deque (start at the initial position)
	void Reset();

public:

	// The top and bottom deque positions
	std::atomic<long> m_Top;
	std::atomic<long> m_Bottom;

	// The job buffer size (for the deque and the ring buffer)
	long m_BufferSize;

	// The job ring buffer position
	long m_RingBufferPosition;

	// The job ring buffer
	PeonJob* m_RingBuffer;

	// The deque buffer
	PeonJob** m_DequeBuffer;

#ifdef JobWorkerDebug

	// Our debug mutex
	std::mutex m_DebugMutex;

#endif
};

// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
