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
	PeonMemoryAllocator();
	PeonMemoryAllocator(const PeonMemoryAllocator&);
	~PeonMemoryAllocator();

//////////////////
// MAIN METHODS //
public: //////////

	// Allocate x amount of data
	char* AllocateData(PeonWorker* _owner, IntegerSize _amount);

	// Deallocate the input block
	void DeallocateData(char* _data);

private:

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

///////////////
// VARIABLES //
private: //////

	// The memory block free list
	MemoryBlock* m_MemoryBlockFreeList[std::numeric_limits<IntegerSize>::digits];

	// The total number of blocks
	IntegerSize m_TotalMemoryBlocks[std::numeric_limits<IntegerSize>::digits];
};

// __InternalPeon
PeonNamespaceEnd(__InternalPeon)
