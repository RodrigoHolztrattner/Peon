////////////////////////////////////////////////////////////////////////////////
// Filename: PeonMap.cpp
////////////////////////////////////////////////////////////////////////////////
#include "PeonMap.h"
#include "PeonSystem.h"
#include <chrono>
#include <algorithm>
#include <cassert>

__InternalPeon::PeonMap::PeonMap()
{
	// Set the initial data
	m_PendencyList[0] = nullptr;
	m_PendencyList[1] = nullptr;
	m_ObjectVersion = 0;
	m_TotalConcurrentAccesses = 0;
	m_CurrentIndex = 0;
	m_ItemCount = 0;
	m_CollisionCount = 0;
	m_ListItemCount = 0;
	m_LastUpdatedPendencyItem = &m_PendencyList[m_CurrentIndex];
}

__InternalPeon::PeonMap::PeonMap(const __InternalPeon::PeonMap& other)
{
}

__InternalPeon::PeonMap::~PeonMap()
{
}

bool __InternalPeon::PeonMap::Insert(Key _key, ObjectType _object)
{
	// Increment the access controll
	m_TotalConcurrentAccesses++;

	// Try to update and swap
	UpdateAndSwap();

	// Check if an object with the given key already exists on the internal map
	if (m_InternalMap[m_CurrentIndex].find(_key) != m_InternalMap[m_CurrentIndex].end())
	{
		// Decrement the access controll
		m_TotalConcurrentAccesses--;

		return false;
	}

	// Try to insert the insertion item
	return TryInsertItem(_key, _object, ItemOperation::Insert, false, true);
}

bool __InternalPeon::PeonMap::Remove(Key _key)
{
	// Increment the access controll
	m_TotalConcurrentAccesses++;

	// Try to update and swap
	UpdateAndSwap();

	// This variable will control if we found the object valid
	bool have = false;

	// Check if an object with the given key exists on the internal map
	if (m_InternalMap[m_CurrentIndex].find(_key) != m_InternalMap[m_CurrentIndex].end())
	{
		// Set that we found the object on the internal map
		have = true;
	}

	// Try to insert the deletion item
	return TryInsertItem(_key, ObjectType(), ItemOperation::Delete, have, false);
}

uint32_t __InternalPeon::PeonMap::Size()
{
	// Increment the access controll
	m_TotalConcurrentAccesses++;

	// Try to update and swap
	UpdateAndSwap();

	// Save the item count
	uint32_t itemCount = m_ItemCount;

	// Decrement the access controll
	m_TotalConcurrentAccesses--;

	return itemCount;
}

bool __InternalPeon::PeonMap::TryInsertItem(Key _key, ObjectType _object, ItemOperation _itemOperation, bool _returnInitialCondition, bool _returnCondition)
{
	// The last valid exchange object we will use to shorter our path to the list end
	std::atomic<ListItem*>* lastValidExchangeObject = &m_PendencyList[m_CurrentIndex];

	// The local have control variable
	bool localHave = _returnInitialCondition;

	// Until we delete/insert the item or determine that it doesn't/does exist
	while (true)
	{
		// The current exchange object (initialize it with the root)
		std::atomic<ListItem*>* currentExchangeObject = lastValidExchangeObject;

		// For each item on the pendency list
		ListItem* currentItem = *currentExchangeObject;
		while (currentItem != nullptr)
		{
			// Compare the keys
			if (currentItem->key == _key)
			{
				// Check the operation
				if (currentItem->operation == ItemOperation::Insert && !localHave)
				{
					// Set that we have the item on the list
					localHave = true;
				}
				// Check for deletion
				else if (currentItem->operation == ItemOperation::Delete && localHave)
				{
					// Set that we don't have the item on the list anymore
					localHave = false;
				}
			}

			// Check if we can set a new last valid exchange object
			if (currentItem->next != nullptr)
			{
				// Set it
				lastValidExchangeObject = &currentItem->next;
			}

			// Set the new exchange object
			currentExchangeObject = &currentItem->next;

			// Set the new current item
			currentItem = currentItem->next;
		}

		// Check if we have/dont't have the item on the list
		if (localHave == _returnCondition)
		{
			// Decrement the access controll
			m_TotalConcurrentAccesses--;

			// We don't have the item anymore, return false
			return false;
		}
		
		// Create a new list item
		ListItem* newItem = PeonAllocator<ListItem>::allocate(1);

		// Set the new item data
		newItem->key = _key;
		newItem->object = _object;
		newItem->operation = _itemOperation;
		newItem->next = nullptr;

		// We expect the next item from the current exchange object is a nullptr
		ListItem* expectexNext = nullptr;

		// Compare and swap
		if (std::atomic_compare_exchange_strong(currentExchangeObject, &expectexNext, newItem))
		{
			// Increment the object version
			m_ObjectVersion++;

			// We sucessfully inserted the new item, return true
			_itemOperation == ItemOperation::Insert ? m_ItemCount++ : m_ItemCount--;
			m_ListItemCount++;

			// Decrement the access controll
			m_TotalConcurrentAccesses--;

			return true;
		}

		// We've failed the race to insert the new item, try again on the next iteration, deallocate the memory used by the new item
		PeonAllocator<ListItem>::deallocate(newItem, 1);

		// Increment the number of collisions
		m_CollisionCount++;
	}

	// Decrement the access controll
	m_TotalConcurrentAccesses--;

	// Unreachable statement
	return false;
}

void __InternalPeon::PeonMap::UpdateAndSwap()
{
	// Try to lock our swap mutex
	if(!m_SwapMutex.try_lock())
	{
		// Lock and unlock the swap control mutex
		m_SwapControlMutex.lock();
		int a = m_ObjectVersion;
		a++;
		m_SwapControlMutex.unlock();
		return;
	}

	// Save the current object version
	uint32_t currentObjectVersion = m_ObjectVersion;

	// Determine the other index
	uint32_t otherIndex = (m_CurrentIndex + 1) % 2;

	// Until we reach the pendency list end
	ListItem* currentItem = *m_LastUpdatedPendencyItem;
	while (currentItem != nullptr)
	{
		// Check the operation type for this item
		if (currentItem->operation == ItemOperation::Insert)
		{
			// Insert this item into the other internal map
			m_InternalMap[otherIndex].insert(std::pair<Key, ListItem*>(currentItem->key, currentItem));
		}
		// Perform a deletion operation
		else
		{
			// Get the element
			ListItem* element = m_InternalMap[otherIndex][currentItem->key];

			// Remove the element with the given key
			m_InternalMap[otherIndex].erase(currentItem->key);
		}

		// Check if we can set a new last updated object
		if (currentItem->next != nullptr)
		{
			// Set it
			m_LastUpdatedPendencyItem = &currentItem->next;

			// Update the current object version
			currentObjectVersion = m_ObjectVersion;
		}

		// Go to the next item
		currentItem = currentItem->next;
	}

	// Check if we have a decent difference between the maps and if they have a minimum size
	int totalFirstMap = m_InternalMap[0].size();
	int totalSecondMap = m_InternalMap[1].size();
	int compareValue = 6;
	if (std::abs(totalFirstMap - totalSecondMap) <= compareValue || (m_InternalMap[0].size() <= compareValue && m_InternalMap[1].size() <= compareValue))
	{
		// Unlock the swap mutex
		m_SwapMutex.unlock();

		//
		return;
	}

	// Ok, now its time to check if we can swap the current index, lock the swap control mutex
	m_SwapControlMutex.lock();

	// If we are the only thread inside this object and the old and current versions are the same
	bool swapped = false;
	if (m_TotalConcurrentAccesses == 1 && currentObjectVersion == m_ObjectVersion)
	{
		// Update the current index
		m_CurrentIndex = (m_CurrentIndex + 1) % 2;

		// Update the other index
		otherIndex = (m_CurrentIndex + 1) % 2;

		// Zero the list item count
		m_ListItemCount = 0;

		// Set that we swapped
		swapped = true;
	}

	// Unlock the swap control mutex
	m_SwapControlMutex.unlock();

	// If we swapped
	if (swapped)
	{
		// Remove each object from the other pendency list
		ListItem* currentItem = m_PendencyList[otherIndex];
		while (currentItem != nullptr)
		{
			// Get the element
			ListItem* element = currentItem;

			// Set the next item
			currentItem = currentItem->next;

			// Deallocate this element
			PeonAllocator<ListItem>::deallocate(element, 1);
		}

		// Set the nullptr for the pendency list
		m_PendencyList[otherIndex] = nullptr;

		// Clear the other internal map
		m_InternalMap[otherIndex].clear();

		// Copy the current map to the old one
		m_InternalMap[otherIndex].insert(m_InternalMap[m_CurrentIndex].begin(), m_InternalMap[m_CurrentIndex].end());

		// TODO: Add desription
		m_LastUpdatedPendencyItem = &m_PendencyList[m_CurrentIndex];
	}

	// Unlock the swap mutex
	m_SwapMutex.unlock();
}

void __InternalPeon::PeonMap::PrintMap(bool _onlyCollisions)
{
	// Check if we should ignore the values
	if (!_onlyCollisions)
	{
		// For each item inside the internal map
		for (auto it = m_InternalMap[m_CurrentIndex].begin(); it != m_InternalMap[m_CurrentIndex].end(); ++it)
		{
			// Get the item
			auto listItem = it->second;

			// Print the key/val
			std::cout << "- key: " << it->first << " value: " << listItem->object << std::endl;
		}

		// For each item on the pendency list
		ListItem* currentItem = m_PendencyList[m_CurrentIndex];
		while (currentItem != nullptr)
		{
			// Print the key/val
			if (currentItem->operation == ItemOperation::Delete)
			{
				std::cout << "- key: " << currentItem->key << " value: " << "null" << std::endl;
			}
			else
			{
				std::cout << "- key: " << currentItem->key << " value: " << currentItem->object << std::endl;
			}

			// Go to the next element
			currentItem = currentItem->next;
		}
	}

	// Print the remaining control data
	std::cout << "Remaining itens: " << m_ItemCount << std::endl;
	std::cout << "Collision count: " << m_CollisionCount << std::endl;
	std::cout << "List item count: " << m_ListItemCount << std::endl;
}