//
//  scheduler.cpp
//  Processor Scheduler
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 9/13/26.
//

#include <queue>
#include "scheduler.hpp"

/**
 * credit to niranjan for the idea to only run on small cores!!
 * we will not be keeping this logic forever, just for tinkering and data collection purposes
 */

std::queue<ProcessId_t> readyQ;
const int NUM_CORES = 4;
const int OFFSET = 4; // running on the latter 4 cores (the smaller ones) 
ProcessId_t running[8] = {InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), 
                          InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId()};

/* HELPERS */

int getFreeCore() {
    for(int i = OFFSET; i < NUM_CORES+OFFSET; i++) {
        if(running[i] == InvalidProcessId()) {
            return i;
        }
    }
    return -1;
}

int getCoreFromPid(ProcessId_t pid) {
    for(int i = OFFSET; i < NUM_CORES+OFFSET; i++) {
        if(running[i] == pid) {
            return i;
        }
    }
    return -1;
}

/* LOGIC AND SUCH */

void CreateProcess(ProcessId_t pid) {
    // A new process has been created. Update the scheduler's data structures and decisions accordingly.
    SimOutput("CreateProcess(" + std::to_string(pid) + ")", 4);

    // is there a core free?
    int free_core = getFreeCore();
    if(free_core != -1) { // yes
        running[free_core] = pid;
        LoadContext(running[free_core], free_core);
        RunCore(free_core);
    }
    else {  // no
        readyQ.push(pid);
    }
    // this is probably not the best way to do this but it's what i got rn
    // basically turning off the big cores and setting the one's we're using to
    // P3 just cause that gives us the best numbers in practice...
    if(readyQ.size() == 1) {
        SetCState(0, CState_t(C6));
        SetCState(1, CState_t(C6));
        SetCState(2, CState_t(C6));
        SetCState(3, CState_t(C6));
        SetPState(4, PState_t(P3));
        SetPState(5, PState_t(P3));
        SetPState(6, PState_t(P3));
        SetPState(7, PState_t(P3));
    }
}

void ExitProcess(ProcessId_t pid) {
    // Process finished running. Update the scheduler's data structures and decisions accordingly.
    // sanity check (what core is this process runnin on? if -1 then it's not running on a core)
    int free_core = getCoreFromPid(pid);
    if(getCoreFromPid(pid) == -1) {
        ThrowException("A process that was not running is calling exit!!!");
    }
    if(!readyQ.empty()){
        // replace what just finished with something new
        running[free_core] = readyQ.front();
        readyQ.pop();
        LoadContext(running[free_core], free_core);
        RunCore(free_core);
    }
    else {
        running[free_core] = InvalidProcessId();   // Nothing is running right now
    }
}

void TimerInterrupt(Time_t now) {
    // You received a timer interrupt. This is where you want to execute scheduling decisions

    if(readyQ.empty())              // no processes are waiting
        return;

    // sanity check, all cores should be working i feel
    if(getFreeCore() != -1) {
        std::cout << "readyQ is not empty but core " << getFreeCore() << "is Free..." << std::endl;
    }
    
    // we have waiting processes, so lets get them sometime on the cores, concurrency...
        // the stats say this doesn't help too much, i just extended what mootaz did

    // for each core, replace what is currently running (as long as the q has processes to run)
    for(int i = OFFSET; i < NUM_CORES+OFFSET && !readyQ.empty(); i++) {
        // all cores should be busy so we can count on there being a process to push off
        SaveContext(running[i], i); // prepare
        readyQ.push(running[i]); // kick current process off
        running[i] = readyQ.front(); // get a new process to run
        readyQ.pop(); // take the new process to run off the readyQ
        LoadContext(running[i], i); // prepare
        RunCore(i); // run the new process
        if(readyQ.empty()) {
            std::cout << "the q has less items than we have cores, empty on core " << i << std::endl;
        }
    }    
}

void CStateTransitionComplete(CPUId_t core_id){
    // we aren't changing cstates throughout the simulator so we don't care about this rn
}

void SimulationComplete(Time_t now) {
    // Add any bookkeeping or statistics that you would want to collect. Program terminates after this function returns.
    // energy delay product = time times energy, let's us understand if the tradeoff of energy and time is "too much"
    std::cout << "EDP = " << (now / 1000.0) * (GetTotalEnergyConsumed()/3600000000.0) << " s*kWh" << std::endl;
    std::cout << "Run stopped at " << FormatTime(now) << " after consuming " << GetTotalEnergyConsumed()/3600000000.0 << " kWh" << std::endl;
}
