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
    cout << "Alarm Working!" <<endl;
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
    ++tick;
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    
    kernel->currentThread->setPriority(kernel->currentThread->getPriority() - 1);
    if (status == IdleMode) {	// is it time to quit?
        if (!interrupt->AnyFutureInterrupts()&& awakeList.size()==0) {
	    timer->Disable();	// turn off the timer
	}
    } else {			// there's someone to preempt
       SchedulerType Mode = kernel->scheduler->getSchedulerType();
	   if( Mode == RR || Mode == Priority || Mode == SRTF) {
		  interrupt->YieldOnReturn();
	   }
    }


   unsigned int  resumedThreads = 0;
    for(vector<SleepObj*>::iterator i=awakeList.begin(),j = awakeList.end(); i != j ;i++){
    	if((*i)->sleepTo > tick)break;
    	else {
    		kernel->scheduler->ReadyToRun((*i)->thread);
    		delete *i;
    		++resumedThreads;
    		cout << "Thread Awake:"<< tick << "!" <<endl;
    	}
    }
    //cleaning up
    if(resumedThreads){
    	awakeList.erase(awakeList.begin(),awakeList.begin() + resumedThreads);
    }
}
//In miliseconds
void Alarm::WaitUntil(int x)
{
	IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff);
	Thread * idleThread = kernel->currentThread;
	unsigned int waitTo = tick + (int)((float)x/0.25);
	int i = 0;
	for(int j = awakeList.size();i < j && awakeList[i]->sleepTo < waitTo ; ++i);
	awakeList.insert(awakeList.begin()+i,new SleepObj(waitTo,idleThread));
	idleThread->Sleep(false);
	kernel->interrupt->SetLevel(oldlevel);
}
