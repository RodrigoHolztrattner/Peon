////////////////////////////////////////////////////////////////////////////////
// Filename: PeonMemoryAllocator.cpp
////////////////////////////////////////////////////////////////////////////////
#include "PeonMemoryAllocator.h"
#include "PeonSystem.h"
#include <chrono>
#include <algorithm>
#include <cassert>

__InternalPeon::PeonMemoryAllocator::PeonMemoryAllocator(PeonWorker* _owner) : m_Owner(_owner)
{
	// Set the initial data
	memset(m_MemoryBlockFreeList, 0, sizeof(MemoryBlock*) * std::numeric_limits<IntegerSize>::digits);
	memset(m_TotalMemoryBlocks, 0, sizeof(IntegerSize) * std::numeric_limits<IntegerSize>::digits);
	m_DeallocationChain = nullptr;

#ifdef _DEBUG 
	memset(m_TotalUsedMemoryBlocks, 0, sizeof(IntegerSize) * std::numeric_limits<IntegerSize>::digits);
#endif
}

__InternalPeon::PeonMemoryAllocator::PeonMemoryAllocator(const __InternalPeon::PeonMemoryAllocator& other)
{
}

__InternalPeon::PeonMemoryAllocator::~PeonMemoryAllocator()
{
	// Call the validate method (check for any leaks)
	Validate(true);
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

	// Check if this block can be deallocated by this allocator (compare the owners)
	if (block->workerOwner != m_Owner)
	{
		// Push this block to a future deallocation
		PushDeallocationBlock(block);
	}
	else
	{
		// Deallocate this block
		DeallocateBlock(block);
	}
}

void __InternalPeon::PeonMemoryAllocator::DeallocateBlock(MemoryBlock* _block)
{
	// Determine the block index
	IntegerSize blockIndex = ilog2(_block->totalMemory);

	// Set the block data
	_block->nextBlock = m_MemoryBlockFreeList[blockIndex];

	// Set the root
	m_MemoryBlockFreeList[blockIndex] = _block;

#ifdef _DEBUG 
	// Decrement the total used blocks
	m_TotalUsedMemoryBlocks[blockIndex]--;
#endif
}

void __InternalPeon::PeonMemoryAllocator::ReleaseDeallocationChain()
{
	int count = 0;
	
	// Until we each the list end
	while (m_DeallocationChain != nullptr)
	{
		// Get the block owner
		auto* blockOwner = m_DeallocationChain->workerOwner;

		// Get the block itself
		auto* block = m_DeallocationChain;

		// Set the new root
		m_DeallocationChain = block->nextBlock;

		// Deallocate the block
		blockOwner->GetMemoryAllocator().DeallocateBlock(block);

		count++;
	}

	std::cout << this << " -> Total deallocation blocks on chain: " << count << std::endl;

	// Validate this allocator
	Validate();
}

void __InternalPeon::PeonMemoryAllocator::PushDeallocationBlock(MemoryBlock* _block)
{
	// Insert this block into the list
	_block->nextBlock = m_DeallocationChain;
	m_DeallocationChain = _block;
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

	#ifdef _DEBUG 
		// Increment the total used blocks
		m_TotalUsedMemoryBlocks[_blockIndex]++;
	#endif

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

#ifdef _DEBUG 
	// Increment the total used blocks
	m_TotalUsedMemoryBlocks[_blockIndex]++;
#endif

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

void __InternalPeon::PeonMemoryAllocator::Validate(bool _destructorCheck)
{
	// For each possible block size
	for (int i = 0; i < std::numeric_limits<IntegerSize>::digits; i++)
	{
		// For each block inside this size lane
		MemoryBlock* currentBlock = m_MemoryBlockFreeList[i];
		IntegerSize count = 0;
		while (currentBlock != nullptr)
		{
			// Go to the next block and increment the count
			currentBlock = currentBlock->nextBlock;
			count++;
		}

#ifdef _DEBUG 

		// If this validate was called from the destructor
		if (_destructorCheck)
		{
			// The total blocks in use must be zero
			assert(m_TotalUsedMemoryBlocks[i] == 0 && "Peon: Not all memory blocks were deallocated!");

			// Check the count
			assert(m_TotalMemoryBlocks[i] == count && "Peon: The memory block count doesn't match the list count!");
		}
		else
		{
			// Check the count
			assert(m_TotalMemoryBlocks[i] - m_TotalUsedMemoryBlocks[i] == count && "Peon: The memory block count doesn't match the list count!");
		}
#else
		// Check the count
		assert(m_TotalMemoryBlocks[i] == count);
#endif
	}
}