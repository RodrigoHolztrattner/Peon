Peon
[![Website](https://img.shields.io/website-up-down-green-red/http/shields.io.svg?label=my-website)](https://sites.google.com/view/rodrigoholztrattner)
[![Linkedin](https://img.shields.io/badge/linkedin-updated-blue.svg)](https://www.linkedin.com/in/rodrigoholztrattner/)
=====

Originally created to help me with the processing division of the game engines that I work on, it has become an almost essential module in all my projects.

Basically this system works by breaking the code into smaller pieces that can be processed in parallel to increase performance without generating too many dependencies internally. All features are (partially) described in the Peon.h file.

In essence it's a work stealing queue system designed for real time applications.

The implementation here is based on the Lock-Free-Work-Stealing idea from Stefan Reinalter, more info can be found on his website https://blog.molecular-matters.com, a huuuge thanks for this sir!

--------------------------------

# How It Works

- First, you will initialize the system with the maximum number of working threads you want to use.
- Then you will divide your workload into small jobs...
- You will start those jobs and...
- The magic will just happen!

> The library itself was made to be used in a per-thread basis, that is, only one instance per thread is allowed!
> Using this approach we get a huge thread control and alot of flexibility.
> Keep in mind that you still can create different threads from any type by your own.

# Dependencies

No dependencies, pure C++, just try to use the lastest version because I'm always trying to update my libraries with the most recent features the language can offer and I'm a little lazy to keep checking this :)

# Install

Just copy & paste every *.h* and *.cpp* from the root folder into your project. To use the library just include the **Peon.h** as it includes
every other file needed.

The project was built using the Visual Studio 2017 and should work properly just by opening its solution file.

The entire solution was made using pure C++ so if you want you can just copy-paste the files and use them in your project.

# Getting Started

Like I said, you need to specify how many threads you want to use for the system, also you need to provide the maximum buffer size.
The buffer size determines how many jobs you can create/run without needing to call the **ResetWorkFrame** method. The main idea here is to
not use an extensible array for all jobs, instead we will use a fixed one (no memory allocation = more speed) and just refresh this array
when we are close to the limit or our "frame" finished.

> Keep in mind that checking the number of jobs is you responsability and in the current implementation the system will not refresh by its own.

### Initializing the Peon System

To initialize the system we will be using the **Initialize** method, it's possible to pass a class/struct type as a template parameter for this
method, this will make each worker thread allocate a <your class/struct type here> object for itself (if you are using this custom data
remember to pass the last !optional! boolean parameter as true).

```c++
// Create our scheduler instance
Peon::Scheduler scheduler = new Peon::Scheduler();

// Initialize without custom data
if(!scheduler->Initialize(4, 4096))
{
	return false;
}

// Initialize with custom data
if(!scheduler->Initialize<MyCustomType>(4, 4096, true))
{
	return false;
}
```

To retrieve the custom data, use the **GetCustomData** method:

```c++
// Get the custom data from the current running thread (you SHOULD ensure that we are inside a job)
MyCustomType* myData = scheduler->GetCustomData();

// Get the custom data from the given thread index (from 0 to the maximum worker threads)
MyCustomType* myData = scheduler->GetCustomData(2);
```

### Creating and Starting a Job

It's possible to create an independent job or a child job, using the hierarchical way you will be creating a dependency tree where each
parent job depends on its children to finish first, then allowing it to finish too.

Creating a job is simple:

```c++
// Creating an independent job
Peon::Job* myJob = scheduler->CreateJob([]()
{
    std::cout << "Hello world!" << std::endl;
});

// Creating a child job
Peon::Job* myJob = scheduler->CreateChildJob(parentJob, []()
{
    std::cout << "Hello world!" << std::endl;
});

// Creating a child job for the current running worker thread
Peon::Job* myJob = scheduler->CreateChildJob([]()
{
    std::cout << "Hello world!" << std::endl;
});
```

Now the only remaning thing to begin the execution is the **StartJob** method:

```c++
scheduler->StartJob(myJob);
```

Now the job was configured and one of our worker threads will execute this code!

### Wait for Job

Of course we need a method to synchronize, like the *join* one that exist from almost any thread system, so this is the one we have:

```c++
scheduler->WaitForJob(myJob);
```

### Containers

The container type is supposed to be used as a parent for many children, you will probably use this when creating jobs inside a loop:

```c++
// First we will create our container
Peon::Container* myContainer = scheduler->CreateJobContainer();

/* you can start the container job here or after creating those child jobs, doesn't matter */

// Now we will create multiple jobs
for(int i=0; i<1000; i++)
{
    // Create a simple job
    Peon::Job* currentJob = scheduler->CreateChildJob(myContainer, [=]()
    {
        std::cout << "Hello world with index: " << i << std::endl;
    });

    // Also start our job (we will lost the variable reference on the next iteration)
    scheduler->StartJob(currentJob);
}

// Start the container execution
scheduler->StartJob(myContainer);

// Wait until each of those jobs finish
scheduler->WaitForJob(myContainer);
```

### Control

There are some utility methods that you can use in your application.
Those are the ones:

```c++

// Return the total number of worker threads
uint32_t totalNumberWorkers = scheduler->GetTotalWorkers();

// Return the current worker index (from 0 to the maximum number of threads)
uint32_t currentWorkerIndex = scheduler->GetCurrentWorkerIndex();

// Block the execution of any new created job that could start after this method invocation
scheduler->BlockWorkerExecution();

// Release the execution block from the last method
scheduler->ReleaseWorkerExecution();

// Return the current job object for the current context (you MUST ensure we are inside a job)
Peon::Job* currentJob = scheduler->GetCurrentJob();

// Return the current worker object for the current context (again, same rules, you must ensure we are inside a job)
Peon::Worker* currentWorker = scheduler->GetCurrentWorker();
```

> Just to clarify, the **GetCurrentWorker** method exists for debug purposes and you wont be gaining any functionality from the worker object.

### Considerations

- Remember to use the **WaitForJob** method for each container/active job before calling **ResetWorkFrame**, you must ensure all jobs have finished running before calling this.
- REMEMBER to call the **ResetWorkFrame** method when your "frame update" (games) finish or when your job count is closer to the maximum allowed.
