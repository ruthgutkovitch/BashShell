#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"


using namespace std;

void ctrlZHandler(int sig_num) {
    std::cout<<"smash: got ctrl-Z" <<std::endl;
    SmallShell& smash = SmallShell::getInstance();
    if(smash.getFgPid() ==-1){
        return;
    }
    if(smash.getFgJob()==-1){
        Command* command = new ExternalCommand(smash.getFgCommand()->getCmdLine(),
                                               smash.getFgCommand()->getArgs(),false,
                                               smash.getFgCommand()->getCmdLineUnmodified(), smash.getFgPid());
        smash.getJobListPtr()->addJob(command, true); // this command will get new job_id
        delete command; // CHANGE 2
    }
    else{
        Command* command = new ExternalCommand(smash.getFgCommand()->getCmdLine(),
                                               smash.getFgCommand()->getArgs(),false,
                                               smash.getFgCommand()->getCmdLineUnmodified(), smash.getFgPid());
        smash.getJobListPtr()->addFgJob(command,smash.getFgJob());//exist command
        delete command; // CHANGE 3
    }
    if(kill(smash.getFgPid(), SIGSTOP) == 0){
        std::cout << "smash: process " << smash.getFgPid() << " was stopped"<<std::endl;
    } else{
        perror("smash error: killpg failed");
    }
    smash.removeFgMode();
    return;
}

void ctrlCHandler(int sig_num) {
    std::cout<<"smash: got ctrl-C" <<std::endl;
    SmallShell& smash = SmallShell::getInstance();
    if(smash.getFgPid() ==-1){
        return;
    }
    pid_t pid = smash.getFgPid();
    if(killpg(pid, SIGKILL) == 0){ // why killpg?
        std::cout << "smash: process " << pid << " was killed"<<std::endl;
    } else{
        perror("smash error: killpg failed");
    }
    smash.removeFgMode();
}

void alarmHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    std::cout<<"smash: got an alarm"<<std::endl;
    smash.removeExpired();
}
