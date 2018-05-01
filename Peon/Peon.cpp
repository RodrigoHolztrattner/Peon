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

/////////////
// DEFINES //
/////////////

// FEEL FREE TO EDIT THOSE 2 DEFINES //

// The total number of jobs that depends on the first that are going to be created
#define TotalDependantJobs			(4)

// The total number of primary jobs we will create (this will be multiplicated by the above value)
#define TotalPrimaryJobs			(4096)

// - //
// - //
// - //

// DONT TOUCH THOSE DEFINES BELOW //

// The maximum number of jobs that we can fit inside our queue <respect the maximum unsigned int size>
#define MaximumNumberJobQueue		(TotalPrimaryJobs * TotalDependantJobs * 2)

// The maximum and minimum number of created jobs (we will create a random amount between those 2 values each "frame")
// The maximum value should not be higher then the MaximumNumberJobQueue/(TotalDependantJobs * 2) defined above
#define MaximumNumberJobs			(MaximumNumberJobQueue/(TotalDependantJobs * 4))
#define MinimumNumberJobs			((uint32_t)(MaximumNumberJobs * 0.8))

////////////////
// STRUCTURES //
////////////////

// The test instance type
struct TestInstance
{
	// Our counter
	std::atomic<uint32_t> counter;

	// The loop total
	uint32_t loopTotal;

	// Our scheduler
	Peon::Scheduler scheduler;

	// If this test instance was created in another thread
	bool usesAnotherThread;

	// The thread (if used)
	std::thread* thread;
};

/////////////
// GLOBALS //
/////////////

// Our print and general purpose mutex
std::mutex s_PrintMutex;

// Our test instance vector
std::vector<TestInstance*> m_TestInstanceVector;

// Our random seed variable
uint32_t s_RandomSeed;

/////////////
// METHODS //
/////////////

uint32_t RandomIntInRange(uint32_t _from, uint32_t _to)
{
	s_RandomSeed = (214013 * s_RandomSeed + 2531011);
	uint32_t rand = (s_RandomSeed >> 16) & 0x7FFF;
	return (rand % (_to - _from)) + _from;
}

void Loop(TestInstance* _testInstance, std::string _loopName)
{
	// Get a short variable for each test object
	Peon::Scheduler* _scheduler = &_testInstance->scheduler;
	std::atomic<uint32_t>& _counter = _testInstance->counter;
	uint32_t& _loopTotal = _testInstance->loopTotal;

	// Create a container
	Peon::Container* container = _scheduler->CreateContainer();

	// Create the jobs
	for (uint32_t i = 0; i < _loopTotal; i++)
	{
		// Create a new job
		Peon::Job* newJob = _scheduler->CreateChildJob(container, [i, _loopTotal, &_counter]()
		{
			// Increment the counter
			_counter++;
		});

		// For each dependant jobs we should create
		for (int j = 0; j < TotalDependantJobs; j++)
		{
			// Create a new job
			Peon::Job* dependantJob = _scheduler->CreateChildJob(container, [j, &_counter]()
			{
				// Increment the counter
				_counter++;
			});

			// Set the job dependency (no need to start this job)
			_scheduler->AddJobDependency(newJob, dependantJob);
		}

		// Begin the new job
		_scheduler->StartJob(newJob);
	}

	// Begin the container
	_scheduler->StartJob(container);

	// Wait for the container
	_scheduler->WaitForJob(container);

#ifdef NDEBUG

	// Check the count total
	if (_counter != (_loopTotal * (TotalDependantJobs + 1)))
	{
		std::cout << "Invalid couter found, the values are: " << _counter << " and: " << (_loopTotal * (TotalDependantJobs + 1)) << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(10));
		exit(1);
	}

#else

	// Check the count total
	assert(_counter == (_loopTotal * (TotalDependantJobs + 1)) && "Invalid values were found!");

#endif

	// New random total count
	_loopTotal = RandomIntInRange(MinimumNumberJobs, MaximumNumberJobs);

	// Lock the print mutex
	s_PrintMutex.lock();

	// Print text
	std::cout << "Loop: " << _loopName << " reached the end sucessfully! The counter was: " << _counter << " and in the next loop we will count to: " << (_loopTotal * (TotalDependantJobs + 1)) << std::endl;

	// Unlock the print mutex
	s_PrintMutex.unlock();

	// Reset the counter
	_counter = 0;
}

void PreLoop(TestInstance* _testInstance, std::string _loopName)
{
	// First update loop
	while (true)
	{
		// Perform the update
		Loop(_testInstance, _loopName);

		// Reset the work frame
		_testInstance->scheduler.ResetWorkerFrame();

		// Sleep random amount
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void CreateSchedulerAndRun(int _totalWorkerThreads, bool _createInSeparatedThread, std::string _loopName)
{
	// Create our test instance object
	TestInstance* testInstance = new TestInstance();

	// Set the initial data
	testInstance->counter		= 0;
	testInstance->loopTotal		= 0;

	// Initialize the peon scheduler
	testInstance->scheduler.Initialize(_totalWorkerThreads, MaximumNumberJobQueue);

	// Get the random initial value for our counter
	testInstance->loopTotal = RandomIntInRange(MinimumNumberJobs, MaximumNumberJobs);

	// Check if we should create a separated thread to run the loop
	if (_createInSeparatedThread)
	{
		// Create a separated thread
		testInstance->thread = new std::thread([=]() 
		{
			// Call the PreLoop() method
			PreLoop(testInstance, _loopName);
		});
	}
	else
	{
		// Call the PreLoop() method
		PreLoop(testInstance, _loopName);
	}

	// Insert it into our vector
	m_TestInstanceVector.push_back(testInstance);
}

int main()
{
	// Create the schedulers and run the loop method
	CreateSchedulerAndRun(2, true,		"A");
	CreateSchedulerAndRun(3, true,		"B");
	CreateSchedulerAndRun(4, true,		"C");
	CreateSchedulerAndRun(5, true,		"D");
	CreateSchedulerAndRun(2, true,		"E");
	CreateSchedulerAndRun(19, true,		"F");
	CreateSchedulerAndRun(2, true,		"G");
	CreateSchedulerAndRun(2, true,		"H");
	CreateSchedulerAndRun(2, true,		"I");
	CreateSchedulerAndRun(2, true,		"J");
	CreateSchedulerAndRun(3, false,		"K");

	return 0;
}