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
#include "debug.h"

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom) :  current(0) {
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

void Alarm::WaitUntil(int val){
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    Thread* t = kernel->currentThread;
    DEBUG(dbgThread, "Thread " << t->getName() << " waits until " << val << "(ms)");

    sleeping_threads.push_back(thread_clk(t, current + val));
    t->Sleep(false);
    kernel->interrupt->SetLevel(oldLevel);
}

void Alarm::CallBack() {
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();

    ++current;
    bool woken = false;

    for(unsigned i = 0; i < sleeping_threads.size(); ){
        thread_clk it = sleeping_threads[i];

        if(it.second < current){
            woken = true;
            DEBUG(dbgThread, "Thread "<<kernel->currentThread->getName() << " is Called back");
            kernel->scheduler->ReadyToRun(it.first);
            sleeping_threads.erase(sleeping_threads.begin() + i);
        } else ++i;
    }
    
    if (status == IdleMode && !woken && sleeping_threads.empty()) {	// is it time to quit?
        if (!interrupt->AnyFutureInterrupts()) {
	        timer->Disable();	// turn off the timer
	    }
    } else {
        // cout << "=== interrupt->YieldOnReturn ===" << endl;			// there's someone to preempt
	    //interrupt->YieldOnReturn();
        ;
    }
}

