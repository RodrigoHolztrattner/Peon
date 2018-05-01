////////////////////////////////////////////////////////////////////////////////
// Filename: PeonMemoryAllocator.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "PeonConfig.h"

#include <iostream>
#include <chrono>
#include <thread>

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

// We know the job system
class PeonSystem;
class PeonWorker;

////////////////////////////////////////////////////////////////////////////////
// Class name: PeonMemoryAllocator
////////////////////////////////////////////////////////////////////////////////
class PeonMemoryAllocator
{
private:

	// All friend classes
	friend PeonSystem;
	friend PeonWorker;

	// The integer size used
	using IntegerSize = uint32_t;

	// The minimum number of blocks the should be allocated
	static const uint32_t MinimumBlocksAllocated = 10;

	// Next power of 2 rounded up
	inline IntegerSize pow2roundup(IntegerSize x)
	{
		if (x < 0)
			return 0;
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return x + 1;
	}

	// The memory block type
	struct MemoryBlock
	{
		// The peon worker owner
		PeonWorker* workerOwner;

		// The memory block size
		uint32_t totalMemory;

		// The next memory block
		MemoryBlock* nextBlock;

		// Padding
		uint32_t padding;
	};

public:
	PeonMemoryAllocator(PeonWorker* _owner);
	PeonMemoryAllocator(const PeonMemoryAllocator&);
	~PeonMemoryAllocator();

//////////////////
// MAIN METHODS //
public: //////////

	// Allocate x amount of data
	char* AllocateData(PeonWorker* _owner, IntegerSize _amount);

	// Deallocate the input block
	void DeallocateData(char* _data);

protected:

	// Deallocate a block
	void DeallocateBlock(MemoryBlock* _block);

	// Release our deallocation chain and make each block be deallocated by the correct owner
	void ReleaseDeallocationChain();

private:

	// Push a deallocation block (that aren't ours and must be deallocated by the System)
	void PushDeallocationBlock(MemoryBlock* _block);

	// Determine the correct block index that should be used for the amount of data needed (also adjust he input memory to the correct size)
	IntegerSize DetermineCorrectBlock(IntegerSize& _amount);

	// Allocate a block
	MemoryBlock* AllocateBlock(PeonWorker* _owner, IntegerSize _amount, IntegerSize _blockIndex);

	// Cast data to block
	MemoryBlock* CastBlockFromData(char* _data);

	// Cast block to data
	char* CastDataFromBlock(MemoryBlock* _block);

	// Integer log2
	IntegerSize ilog2(IntegerSize _value)
	{
		int targetlevel = 0;
		while (_value >>= 1) ++targetlevel;
		return targetlevel;
	}

	// Validate this allocator (check for any leaks, only use this when all used memory was properly deallocated)
	void Validate(bool _destructorCheck = false);

///////////////
// VARIABLES //
private: //////

	// The owner worker
	PeonWorker* m_Owner;

	// The memory block free list
	MemoryBlock* m_MemoryBlockFreeList[std::numeric_limits<IntegerSize>::digits];

	// The total number of blocks
	IntegerSize m_TotalMemoryBlocks[std::numeric_limits<IntegerSize>::digits];

#ifdef _DEBUG 
	// The total number of used blocks
	IntegerSize m_TotalUsedMemoryBlocks[std::numeric_limits<IntegerSize>::digits];
#endif

	// The deallocation chain (those blocks aren't from the owner Worker, wi will retain those until the System tell us to
	// deallocate them using the correct Worker
	MemoryBlock* m_DeallocationChain;
};

// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
