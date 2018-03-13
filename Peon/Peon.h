////////////////////////////////////////////////////////////////////////////////
// Filename: PeonJob.h
////////////////////////////////////////////////////////////////////////////////
#pragma once

//////////////
// INCLUDES //
//////////////
#include "PeonConfig.h"
#include "PeonSystem.h"
#include "PeonWorker.h"
#include "PeonJob.h"

/////////////
// DEFINES //
/////////////

///////////////
// NAMESPACE //
///////////////

// Peon
PeonNamespaceBegin(Peon)

////////////
// GLOBAL //
////////////

typedef __InternalPeon::PeonWorker	Worker;
typedef __InternalPeon::PeonJob		Job;
typedef __InternalPeon::Container	Container;
typedef __InternalPeon::PeonSystem	Scheduler;

// Peon
PeonNamespaceEnd(Peon)
