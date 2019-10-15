# HW1 - System call

## Work division

+ `撰寫報告` B10615013 李聿鎧
+ `研究系統` B10615024 李韋宗
+ `撰寫程式` B10615043 何嘉峻
---
## Source Code
Github: https://github.com/chiachun2491/NachOS.git

---
## Report

### 1. 參考PrintInt()的執行架構，建立Sleep()執行架構
+ **`/code/userprog/exception.cc`**
```cpp
case SC_Sleep:
	val=kernel->machine->ReadRegister(4);
	cout << "Sleep time:" <<val << "ms" << endl;
	kernel->alarm->WaitUntil(val);
	return;
```

+ **`/code/userprog/syscall.h`**
```cpp
#define SC_Sleep 12

void Sleep(int number);
```

+ **`/code/test/start.s`**
```assembly
Sleep:
	addiu   $2,$0,SC_Sleep
	syscall
	j       $31
	.end	Sleep
```

### 2. 撰寫`kernel->alarm->WaitUntil(time)`
+ `/code/threads/alarm.h`
```cpp
#include "thread.h"
#include <list>

class SleepList
{
public:
  SleepList();
  void push_back(Thread *t, int wakeupTime);
  bool checkWoken();
  bool empty();

private:
  class SleepThread
  {
  public:
    SleepThread(Thread *t, int wakeupTime);
    Thread *sleepingThread;
    int wakeupTime;
  };

  int currentTime;
  std::list<SleepThread> threadList;
};
```

+ `/code/threads/alarm.cc`
```cpp
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
```


### Screenshot [Link (gif)](https://i.imgur.com/Xv1tEZf.gif)
![screenshot](https://i.imgur.com/Xv1tEZf.gif)


