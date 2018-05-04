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
	m_PendencyList = nullptr;
	m_ItemCount = 0;
	m_CollisionCount = 0;
	m_ListItemCount = 0;
	m_ZeroElementsCount = 0;
}

__InternalPeon::PeonMap::PeonMap(const __InternalPeon::PeonMap& other)
{
}

__InternalPeon::PeonMap::~PeonMap()
{
}

bool __InternalPeon::PeonMap::Insert(Key _key, ObjectType _object)
{
	// Check if an object with the given key already exists on the internal map
	if (m_InternalMap.find(_key) != m_InternalMap.end())
	{
		return false;
	}

	// Try to insert the insertion item
	return TryInsertItem(_key, _object, ItemOperation::Insert, false, true);
}

bool __InternalPeon::PeonMap::Remove(Key _key)
{
	// This variable will control if we found the object valid
	bool have = false;

	// Check if an object with the given key exists on the internal map
	if (m_InternalMap.find(_key) != m_InternalMap.end())
	{
		// Set that we found the object on the internal map
		have = true;
	}

	// Try to insert the deletion item
	return TryInsertItem(_key, ObjectType(), ItemOperation::Delete, have, false);
}

uint32_t __InternalPeon::PeonMap::Size()
{
	return m_ItemCount;
}

bool __InternalPeon::PeonMap::TryInsertItem(Key _key, ObjectType _object, ItemOperation _itemOperation, bool _returnInitialCondition, bool _returnCondition)
{
	// The last valid exchange object we will use to shorter our path to the list end
	std::atomic<ListItem*>* lastValidExchangeObject = &m_PendencyList;

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
			// We sucessfully inserted the new item, return true
			_itemOperation == ItemOperation::Insert ? m_ItemCount++ : m_ItemCount--;
			m_ListItemCount++;

			// Check if we should increment the zero element count
			if (m_ItemCount == 0)
			{
				m_ZeroElementsCount++;
			}

			return true;
		}

		// We've failed the race to insert the new item, try again on the next iteration, deallocate the memory used by the new item
		PeonAllocator<ListItem>::deallocate(newItem, 1);

		// Increment the number of collisions
		m_CollisionCount++;
	}

	// Unreachable statement
	return false;
}

void __InternalPeon::PeonMap::PrintMap(bool _onlyCollisions)
{
	// Check if we should ignore the values
	if (!_onlyCollisions)
	{
		// For each item inside the internal map
		for (auto it = m_InternalMap.begin(); it != m_InternalMap.end(); ++it)
		{
			// Get the item
			auto listItem = it->second;

			// Print the key/val
			std::cout << "- key: " << it->first << " value: " << listItem->object << std::endl;
		}

		// For each item on the pendency list
		ListItem* currentItem = m_PendencyList;
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
	std::cout << "Number of times we had zero elements: " << m_ZeroElementsCount << std::endl;
}