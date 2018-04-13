////////////////////////////////////////////////////////////////////////////////
// Filename: PeonStealingQueue.cpp
////////////////////////////////////////////////////////////////////////////////
#include "PeonStealingQueue.h"

#include <algorithm>

///////////////
// NAMESPACE //
///////////////

__InternalPeon::PeonStealingQueue::PeonStealingQueue()
{
}

__InternalPeon::PeonStealingQueue::PeonStealingQueue(const __InternalPeon::PeonStealingQueue& other)
{
}

__InternalPeon::PeonStealingQueue::~PeonStealingQueue()
{
}

#define QueueMask(bufferSize)	(((unsigned long)bufferSize) - 1u)

bool __InternalPeon::PeonStealingQueue::Initialize(unsigned int _bufferSize)
{
#ifdef JobWorkerDebug

	// Lock our debug mutex
	std::lock_guard<std::mutex> lock(m_DebugMutex);

#endif

    // Set the size and allocate the ring buffer
    m_BufferSize = _bufferSize;
    m_RingBuffer = new PeonJob[_bufferSize];

    // Set the deque size (allocate memory for it)
    m_DequeBuffer = new PeonJob*[_bufferSize];

    // Set the initial position
    m_RingBufferPosition = 0;

    // Set the top and bottom positions
    m_Top = 0;
    m_Bottom = 0;

	return true;
}

__InternalPeon::PeonJob* __InternalPeon::PeonStealingQueue::GetFreshJob()
{
#ifdef JobWorkerDebug

	// Lock our debug mutex
	std::lock_guard<std::mutex> lock(m_DebugMutex);

#endif

    const long index = m_RingBufferPosition++;
    return &m_RingBuffer[(index-1u) & QueueMask(m_BufferSize)];
}

void __InternalPeon::PeonStealingQueue::Reset()
{
#ifdef JobWorkerDebug

	// Lock our debug mutex
	std::lock_guard<std::mutex> lock(m_DebugMutex);

#endif

	m_RingBufferPosition = 0;
}

void __InternalPeon::PeonStealingQueue::Push(PeonJob* _job)
{
#ifdef JobWorkerDebug

	// Lock our debug mutex
	std::lock_guard<std::mutex> lock(m_DebugMutex);

#endif

	long b = m_Bottom.load(std::memory_order_acquire);

    // Push the job
    m_DequeBuffer[b & QueueMask(m_BufferSize)] = _job;

    // Set the new bottom
	m_Bottom.store(b + 1l, std::memory_order_release);
}

__InternalPeon::PeonJob* __InternalPeon::PeonStealingQueue::Pop()
{
#ifdef JobWorkerDebug

	// Lock our debug mutex
	std::lock_guard<std::mutex> lock(m_DebugMutex);

#endif

	long b = m_Bottom.load(std::memory_order_acquire);
	b = b - 1;
	m_Bottom.store(b, std::memory_order_release);
	long t = m_Top.load(std::memory_order_acquire);

	if (t <= b)
	{
		// non-empty queue
		PeonJob* job = m_DequeBuffer[b & QueueMask(m_BufferSize)];
		if (t != b)
		{
			// There's still more than one item left in the queue
			return job;
		}
	
		long expectedTop = t;
		long desiredTop = t + 1;

		if (!m_Top.compare_exchange_strong(expectedTop, desiredTop,
			std::memory_order_acq_rel))
		{
			// Someone already took the last item, abort
			job = nullptr;
		}

		m_Bottom.store(t + 1, std::memory_order_release);

		return job;
	}
	else
	{
		// Deque was already empty
		m_Bottom.store(t, std::memory_order_release);
		m_Bottom = t;
		return nullptr;
	}
}

__InternalPeon::PeonJob* __InternalPeon::PeonStealingQueue::Steal()
{
#ifdef JobWorkerDebug

	// Lock our debug mutex
	std::lock_guard<std::mutex> lock(m_DebugMutex);

#endif

	long t = m_Top.load(std::memory_order_acquire);

    // ensure that top is always read before bottom.
    // loads will not be reordered with other loads on x86, so a compiler barrier is enough.
	std::atomic_thread_fence(std::memory_order_release);
	long b = m_Bottom.load(std::memory_order_acquire);

	// Deque status...
    if (t < b)
    {
        // non-empty queue
        PeonJob* job = m_DequeBuffer[t & QueueMask(m_BufferSize)];

		long expectedTop = t;
		long desiredTop = t + 1;

        // the interlocked function serves as a compiler barrier, and guarantees that the read happens before the CAS.
		if (!m_Top.compare_exchange_strong(expectedTop, desiredTop, std::memory_order_acq_rel))
        {
            // a concurrent steal or pop operation removed an element from the deque in the meantime. Unlock our overhead mutex and return a nullptr
            return nullptr;
        }

        return job;
    }
    else
    {
        // empty queue
        return nullptr;
    }
}