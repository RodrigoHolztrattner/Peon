////////////////////////////////////////////////////////////////////////////////
// Filename: PeonJob.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "Peon.h"

/////////////
// DEFINES //
/////////////

///////////////
// NAMESPACE //
///////////////

#include <thread>
#include <chrono>
#include <atomic>
#include <assert.h>
#include <iostream>
#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

//
//
//

#include "PeonMap.h"

// The global scheduler
auto peonSystem = Peon::Scheduler();

double MapInsertionTestThreaded(uint32_t _numberThreads, uint32_t _totalObjects)
{
	// Initialize the peon system
	peonSystem.Initialize(_numberThreads, _totalObjects * _totalObjects + 2); // Total jobs + containers

	// Initialize the map
	auto map = __InternalPeon::PeonMap();

	// Begin time
	clock_t begin = clock();

	// Create the insertion container
	auto insertionContainer = peonSystem.CreateContainer();

	// Create jobs for each insertion
	for (int i = 0; i < _totalObjects; i++)
	{
		auto newJob = peonSystem.CreateChildJob(insertionContainer, [=, &map]()
		{
			for (int j = 0; j < _totalObjects; j++)
			{
				// Insert this element
				map.Insert(j, j);
			}
		});

		peonSystem.StartJob(newJob);
	}

	// Start the insetion container and wait for it
	peonSystem.StartJob(insertionContainer);
	peonSystem.WaitForJob(insertionContainer);

	// Create the deletion container
	auto deletionContainer = peonSystem.CreateContainer();

	// Create jobs for each deletion
	for (int i = 0; i < _totalObjects; i++)
	{
		auto newJob = peonSystem.CreateChildJob(deletionContainer, [=, &map]()
		{
			for (int j = 0; j < _totalObjects; j++)
			{
				// Remove this element
				map.Remove(j);
			}
		});

		peonSystem.StartJob(newJob);
	}

	// Start the deletion container and wait for it
	peonSystem.StartJob(deletionContainer);
	peonSystem.WaitForJob(deletionContainer);

	// End time
	clock_t end = clock();

	// Assertion (verify the size)
	if (map.Size() != 0)
	{
		std::cout << "Error! Size is: " << map.Size() << std::endl;
	}

	// Print the map info
	map.PrintMap(true);

	return double(end - begin) / CLOCKS_PER_SEC;
}

double MapInsertionTestSTD(uint32_t _numberThreads, uint32_t _totalObjects)
{
	// Create the data we will need
	std::thread* threads = new std::thread[_numberThreads];
	std::map<uint32_t, uint32_t> stdMap;
	std::mutex mutex;

	// Time std
	clock_t beginSTD = clock();

	// Spawn the workers
	for (int i = 0; i < _numberThreads; i++)
	{
		uint32_t slice = _totalObjects / _numberThreads;

		// Spawn the map update method
		threads[i] = std::thread([=, &stdMap, &mutex]()
		{
			// _numberThreads * slice = _totalObjects
			for (int k = 0; k < slice; k++)
			{
				// i * k * j <= _totalObjects * _totalObjects
				for (int j = 0; j < _totalObjects; j++)
				{
					std::lock_guard<std::mutex> lock(mutex);

					// Insert the element
					stdMap.insert(std::pair<uint32_t, uint32_t>(j, j));
				}
			}
		});
	}

	// Wait for all threads
	for (int i = 0; i < _numberThreads; i++)
	{
		threads[i].join();
	}

	// Spawn the workers
	for (int i = 0; i < _numberThreads; i++)
	{
		uint32_t slice = _totalObjects / _numberThreads;

		// Spawn the map update method
		threads[i] = std::thread([=, &stdMap, &mutex]()
		{
			for (int k = 0; k < slice; k++)
			{
				for (int j = 0; j < _totalObjects; j++)
				{
					std::lock_guard<std::mutex> lock(mutex);

					// Remove the element
					stdMap.erase(j);
				}
			}
		});
	}
	
	// Wait for all threads
	for (int i = 0; i < _numberThreads; i++)
	{
		threads[i].join();
	}

	// Clear
	delete[] threads;

	// Validate
	assert(stdMap.size() == 0);

	// Calculate the time
	clock_t endSTD = clock();
	return double(endSTD - beginSTD) / CLOCKS_PER_SEC;
}

int main()
{
	const uint32_t NumberThreads = 32;
	const uint32_t TotalObjects = 128;
	
	double threadedTest = MapInsertionTestThreaded(NumberThreads, TotalObjects);
	double defaultTest = MapInsertionTestSTD(NumberThreads, TotalObjects);
	
	std::cout << "Time elapsed using threads: " << threadedTest << std::endl;
	std::cout << "Time elapsed using std: " << defaultTest << std::endl;

	return 0;
}