////////////////////////////////////////////////////////////////////////////////
// Filename: PeonMemoryAllocator.cpp
////////////////////////////////////////////////////////////////////////////////
#include "PeonMemoryAllocator.h"
#include "PeonSystem.h"
#include <chrono>
#include <algorithm>

__InternalPeon::PeonMemoryAllocator::PeonMemoryAllocator()
{
	// Set the initial data
	memset(m_MemoryBlockFreeList, 0, sizeof(MemoryBlock*) * std::numeric_limits<IntegerSize>::digits);
	memset(m_TotalMemoryBlocks, 0, sizeof(IntegerSize) * std::numeric_limits<IntegerSize>::digits);
}

__InternalPeon::PeonMemoryAllocator::PeonMemoryAllocator(const __InternalPeon::PeonMemoryAllocator& other)
{
}

__InternalPeon::PeonMemoryAllocator::~PeonMemoryAllocator()
{
}

char* __InternalPeon::PeonMemoryAllocator::AllocateData(PeonWorker* _owner, IntegerSize _amount)
{
	// Determine the correct block to use
	IntegerSize blockIndexToUse = DetermineCorrectBlock(_amount);
	if (blockIndexToUse == -1)
	{
		return nullptr;
	}

	// Allocate the block
	MemoryBlock* block = AllocateBlock(_owner, _amount, blockIndexToUse);
	if (block == nullptr)
	{
		return nullptr;
	}

	return CastDataFromBlock(block);
}

void __InternalPeon::PeonMemoryAllocator::DeallocateData(char* _data)
{
	// Get the memory block from this data
	MemoryBlock* block = CastBlockFromData((char*)_data);

	// Determine the block index
	IntegerSize blockIndex = ilog2(block->totalMemory);

	// Set the block data
	block->nextBlock = m_MemoryBlockFreeList[blockIndex];

	// Set the root
	m_MemoryBlockFreeList[blockIndex] = block;
}

__InternalPeon::PeonMemoryAllocator::IntegerSize __InternalPeon::PeonMemoryAllocator::DetermineCorrectBlock(IntegerSize& _amount)
{
	// Internal block size
	IntegerSize internalBlockSize = sizeof(MemoryBlock);

	// Calc the real amount of memory we need
	_amount = pow2roundup(_amount + internalBlockSize);

	// Determine the index to use
	IntegerSize memoryIndex = ilog2(_amount);

	// Check if the block index is valid
	if (memoryIndex < 0 || memoryIndex >= std::numeric_limits<IntegerSize>::digits)
	{
		return -1;
	}

	return memoryIndex;
}

__InternalPeon::PeonMemoryAllocator::MemoryBlock* __InternalPeon::PeonMemoryAllocator::AllocateBlock(PeonWorker* _owner, IntegerSize _amount, IntegerSize _blockIndex)
{
	// Get the memory block list
	MemoryBlock* blockList = m_MemoryBlockFreeList[_blockIndex];

	// Check if this block is valid
	if (blockList != nullptr)
	{
		// Point to the next block
		m_MemoryBlockFreeList[_blockIndex] = blockList->nextBlock;

		return blockList;
	}

	// Determine the amount of blocks that should be allocated
	uint32_t totalBlocksToAllocate = std::max(MinimumBlocksAllocated, (uint32_t)(m_TotalMemoryBlocks[_blockIndex] * 1.7));

	// Block map method
	auto MapMemoryBlock = [](char* _data, uint32_t _size, uint32_t _index)
	{
		char* dataLocated = &_data[_size * _index];
		MemoryBlock* blockCasted = (MemoryBlock*)dataLocated;
		return blockCasted;
	};

	// Allocate all blocks
	char* allocatedData = new char[totalBlocksToAllocate * _amount];

	// For each memory block
	for (unsigned i = 0; i < totalBlocksToAllocate; i++)
	{
		// Cast to the current block
		MemoryBlock* currentBlock = MapMemoryBlock(allocatedData, _amount, i);
		
		// Set the block data
		currentBlock->totalMemory = _amount;
		currentBlock->workerOwner = _owner;
		currentBlock->nextBlock = (i + 1) < totalBlocksToAllocate ? MapMemoryBlock(allocatedData, _amount, i + 1) : nullptr;
	}

	// Increment the total memory blocks
	m_TotalMemoryBlocks[_blockIndex] += totalBlocksToAllocate;

	// Set the new root block
	m_MemoryBlockFreeList[_blockIndex] = MapMemoryBlock(allocatedData, _amount, 1);

	return MapMemoryBlock(allocatedData, _amount, 0);
}

__InternalPeon::PeonMemoryAllocator::MemoryBlock* __InternalPeon::PeonMemoryAllocator::CastBlockFromData(char* _data)
{
	return (MemoryBlock*)(_data - sizeof(MemoryBlock));
}

char* __InternalPeon::PeonMemoryAllocator::CastDataFromBlock(MemoryBlock* _block)
{
	return (char*)_block + sizeof(MemoryBlock);
}