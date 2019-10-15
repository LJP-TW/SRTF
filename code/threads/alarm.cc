// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
}

//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//	Also, to keep from looping forever, we check if there's
//	nothing on the ready list, and there are no other pending
//	interrupts.  In this case, we can safely halt.
//----------------------------------------------------------------------

void 
Alarm::CallBack() 
{
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();

    bool woken = sleepList.checkWoken();
    
    if (status == IdleMode && !woken && sleepList.empty()) {	// is it time to quit?
        if (!interrupt->AnyFutureInterrupts()) {
	    timer->Disable();	// turn off the timer
	}
    } else {			// there's someone to preempt
	interrupt->YieldOnReturn();
    }
}

//----------------------------------------------------------------------
// Alarm::WaitUntil
//  when execution call Sleep(x), we need to add this thread to 
//  sleepList, have to ensure process won’t be preempted when it runs 
//  in “WaitUntil” function, so need to turn off the interrupt during
//  the function until push to sleepList. 
//----------------------------------------------------------------------

void
Alarm::WaitUntil(int x)
{
    // Backup IntStatus and turn off interrupt OneTick()
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);

    // Add thread to sleepList
    Thread* t = kernel->currentThread;
    cout << "Alarm::WaitUntil go sleep" << endl;
    sleepList.push_back(t, x);

    // Recover the interrupt Status
    kernel->interrupt->SetLevel(oldLevel);
}

// Constructor
SleepList::SleepList()
{
    this->currentTime = 0;
}

void
SleepList::push_back(Thread *t, int wakeupTime)
{
    // Check 
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    threadList.push_back(SleepThread(t, currentTime + wakeupTime));
    t->Sleep(false); // Avoid thread to be destroy
}

//----------------------------------------------------------------------
// SleepList::checkWoken
//  every Alarm::CallBack run, we also need to check every thread 
//  in the sleepList is time to wake up or not.
//  If some thread is time to wake up, add the thread to scheduler
//  ReadyToRun, then erase from the sleepList, then return true.
//  Else if no thread need to wake up return false.
//----------------------------------------------------------------------

bool
SleepList::checkWoken()
{
    bool flag = false;  // record thread woke or not

    currentTime++;

    for(std::list<SleepThread>::iterator it = threadList.begin(); it != threadList.end();)
    {
        if(currentTime >= it->wakeupTime)
        {
            flag = true; // somethread is time to run
            cout << "SleepList::checkWoken Thread woken" << endl;
            kernel->scheduler->ReadyToRun(it->sleepingThread);
            it = threadList.erase(it);
        }
        else
        {
            it++;
        }
    }
    return flag;
}

bool
SleepList::empty()
{
    return threadList.size() == 0;
}

// Constructor
SleepList::SleepThread::SleepThread(Thread *t, int wakeupTime)
{
    this->sleepingThread = t;
    this->wakeupTime = wakeupTime;
}
