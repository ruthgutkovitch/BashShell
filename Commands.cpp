#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h>
#include <vector>
#include <time.h>

#include "signals.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

std::vector<std::string> readCommand(const char* cmd_line){
    std::vector<std::string> command_vector;
    for(int i = 0; i < MAX_CHARACTERS; i++){
        while(cmd_line[i] == ' '){ // remove whitespace
            i++;
        }
        std::string cur_word;
        while(cmd_line[i] != ' ' && cmd_line[i] != '\0'){
            cur_word.push_back(cmd_line[i]);
            i++;
        }
        if(cur_word!=""){
            command_vector.push_back(cur_word);
        }
        if(cmd_line[i] == '\0'){ // check if we reached end of line
            break;
        }
    }
    return command_vector;
}

//checks whether a string is a number
bool stringIsNumber(std::string& str){
    if(str.length() == 1){
        return isdigit(str[0]);
    }
    return (!str.empty() && str.find_first_not_of("0123456789", 1) == string::npos) &&
           (str[0] == '-' || str[0] == '+' || isdigit(str[0]));
}

char* copyFromConst(const char* original){
    if(original == nullptr){
        return nullptr;
    }
    char* res = new char[strlen(original) + 1];
    strcpy(res, original);
    return res;
}

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

//class command
Command::Command(const char* cmd_line, std::vector<std::string> args, pid_t pid, const char* cmd_line_unmodified):
        cmd_line(copyFromConst(cmd_line)),
        args(args), pid(pid),
        cmd_line_unmodified(copyFromConst(cmd_line_unmodified))
{
}

pid_t Command::getPid(){
    return pid;
}

const char* Command::getCmdLine(){
    return cmd_line;
}

const char* Command::getCmdLineUnmodified(){
    return cmd_line_unmodified;
}


//class BuiltInCommands
BuiltInCommand::BuiltInCommand(const char* cmd_line, std::vector<std::string> args):
        Command(cmd_line, args) {}



//class External Commands
ExternalCommand::ExternalCommand(const char* cmd_line, std::vector<std::string> args, bool ampersand, const char* cmd_line_unmodified, pid_t pid):
        Command(cmd_line, args, pid, cmd_line_unmodified), ampersand(ampersand){}


void ExternalCommand::execute(){
    char* argv[4]; // bash -c are 2 arguments, 1 argument is the string input and null
    argv[0] = (char*)"/bin/bash";
    argv[1] = (char*)"-c";
    char* str = new char[std::string(cmd_line).size() + 1];
    strcpy(str, cmd_line);
    argv[2] = str;
    argv[3] = NULL;;
    pid_t cur_pid = fork();
    SmallShell& smash = SmallShell::getInstance();
    if(cur_pid == 0) {
        setpgrp();

        execv(argv[0],argv);
        perror("smash error: execv failed");
        delete [] str;
        exit(0);
    }
    else{
        this->setPid(cur_pid);
        if(ampersand == false) { // run in foreground
            smash.setFgMode(this,cur_pid,-1);
            if((waitpid(cur_pid, nullptr, WUNTRACED))==-1){
                perror("smash error: waitpid failed");
                delete [] str;
                return;
            }
            smash.removeFgMode();
        }
        else{ // run in background
            Command* cmd_to_add = new ExternalCommand(getCmdLine(), args, true,
                                                      getCmdLineUnmodified(), cur_pid);
            if(cmd_to_add == nullptr){
                //error
            }
            smash.getJobListPtr()->addJob(cmd_to_add, false); // add job to joblist
            delete cmd_to_add; // CHANGE 4
            smash.getJobListPtr()->removeFinishedJobs();
        }
    }
    delete [] str;
}



//class ChangePrompClass==chprompt
ChangePromptCommand::ChangePromptCommand(const char* cmd_line, std::vector<std::string> args):
        BuiltInCommand(cmd_line, args)
{
}

void ChangePromptCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    if(args.size() == 1){ // no arguments
        smash.changeName("smash");
    }
    else{
        smash.changeName(args[1]);
    }
}


//class ShowPidCommand==showpid
ShowPidCommand::ShowPidCommand(const char *cmd_line,std::vector<std::string> args):BuiltInCommand(cmd_line,args) {
}

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash pid is " << smash.getShellPid() << std::endl;
}

//class GetCurrDirCommand==pwd
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line, std::vector<std::string> args):
        BuiltInCommand(cmd_line, args){
}

void GetCurrDirCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    std::cout << smash.getCurDirectory() << std::endl;
}

//class ChangeDirCommand==cd
ChangeDirCommand::ChangeDirCommand(const char *cmd_line,std::vector<std::string> args):
        BuiltInCommand(cmd_line,args){
}

void ChangeDirCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    if (args.size() > 2){
        std::cerr << "smash error: cd: too many arguments" << std::endl;
        return;
    }
    else if(args.size() == 2){
        std::string directory = args[1];
        if (args[1] == "-") {//cd -
            std::string prev_directory = smash.getLastDirectory();
            if(prev_directory == ""){ // we dont have prev directory
                std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
                return;
            }
            smash.changeCurDirectory(prev_directory);
            return;
        }
        else if(args[1] == ".."){
            std::string cur_directory = smash.getCurDirectory();
            directory = cur_directory.substr(0,cur_directory.find_last_of("/"));
            if(directory != ""){
                smash.changeCurDirectory(directory);
            }else{
                smash.changeCurDirectory("/");
            }
            return;
        }
        else{
            smash.changeCurDirectory(directory);
            return;
        }
    }
}

//class JobsCommand
JobsCommand::JobsCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs):
        BuiltInCommand(cmd_line, args), jobs_ptr(jobs){
}


void JobsCommand::execute(){
    jobs_ptr->removeFinishedJobs();
    jobs_ptr->printJobsList();
}

//class killCommand
KillCommand::KillCommand(const char *cmd_line, std::vector <std::string> args, JobsList *jobs):
        BuiltInCommand(cmd_line,args),jobs_ptr(jobs) {}

void KillCommand::execute() {
    int signal = 0;
    int job_id = 0;
    if (args.size() == 3){
        std::string sig_num = args[1].substr(1, args[1].size() - 1);
        if(args[1][0] == '-' && stringIsNumber(sig_num) && // check if valid argument( -xxxx)
           !(sig_num[0] == '-' || sig_num[0] == '+') && stringIsNumber(args[2]))
        {
            signal = std::stoi(sig_num);
            job_id = std::stoi(args[2]);
            pid_t job_pid = jobs_ptr -> getPidByJobId(job_id);
            if(job_pid == 0) {
                std::cerr << "smash error: kill: job-id " << job_id << " does not exist" << std::endl;
                return;
            }
            if(kill(job_pid, signal) == -1) {
                perror("smash error: kill failed");
                return;
            }
            std::cout<<"signal number "<< signal <<" was sent to pid "<< job_pid <<std::endl;
        }
        else{
            std::cerr<<"smash error: kill: invalid arguments"<<std::endl;
        }
    }
    else{
        std::cerr<<"smash error: kill: invalid arguments"<<std::endl;
    }

}


//class foreground==fg
ForegroundCommand::ForegroundCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs):
        BuiltInCommand(cmd_line, args), jobs_ptr(jobs) {
}

void ForegroundCommand::execute(){
    int job_id = 0;
    JobsList::JobEntry* job = nullptr;

    if(args.size() == 1){ // no arguments
        int* last_job_id = nullptr;
        job = jobs_ptr->getLastJob(last_job_id);
        if(job == nullptr){
            std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            return;
        }
        job_id = job->getJobId();
    }
    else if(args.size() == 2 && stringIsNumber(args[1])) { // has single argument
        // check if the argument is a positive or negative number
        job_id = std::stoi(args[1]); // converts string to int
        job = jobs_ptr->getJobById(job_id);
        if (job == nullptr) {
            //no such job
            std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << std::endl;
            return;
        }
    }
    else{ // more then single agrument - invalid number of arguments
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }
    pid_t job_pid = jobs_ptr->getPidByJobId(job_id);
    SmallShell& smash = SmallShell::getInstance();
    smash.setFgMode(job->getCommand(),job_pid,job_id);

    jobs_ptr->printJobById(job_id);
    jobs_ptr->removeJobById(job_id); // we remove the job and then in kill go to jobs_ptr again.
    if(kill(job_pid, SIGCONT) == -1){ // CHANGE 6
        perror("smash error: kill failed");
        return;
    }
    if(waitpid(job_pid, nullptr, WUNTRACED) == -1){
        perror("smash error: waitpid failed");
        return;
    }
    smash.removeFgMode();
}

//class backgroundcommand==bg
BackgroundCommand::BackgroundCommand(const char *cmd_line, std::vector <std::string> args, JobsList *jobs):
        BuiltInCommand(cmd_line,args),jobs_ptr(jobs){}

void BackgroundCommand::execute() {
    if(args.size() == 2 && stringIsNumber(args[1])){// bg <number>
        int job_id = std::stoi(args[1]);
        JobsList::JobEntry * ptr = jobs_ptr->getJobById(job_id);
        if ( ptr!= nullptr){
            if(ptr->checkIsStopped()){
                std::cout << ptr->getCommand()->getCmdLineUnmodified() << " : " << ptr->getCommand()->getPid() << std::endl;
                if (kill(jobs_ptr->getPidByJobId(job_id), SIGCONT) == -1){
                    perror("smash error: kill failed");
                    return;
                }
                else{
                    ptr->changeIsStopped(false);
                }
            }
            else{
                std::cerr<<"smash error: bg: job-id "<<job_id<<" is already running in the background"<<std::endl;
                return;
            }
        }
        else{
            std::cerr<<"smash error: bg: job-id "<<job_id<<" does not exist"<<std::endl;
            return;
        }
    }
    else if(args.size()==1){//bg
        int* last_stopped_job_id=nullptr;//-1
        JobsList::JobEntry* ptr = jobs_ptr->getLastStoppedJob(last_stopped_job_id);
        if (ptr!= nullptr){
            std::cout << ptr->getCommand()->getCmdLineUnmodified() << " : " << ptr->getCommand()->getPid() << std::endl;
            if (kill(jobs_ptr->getPidByJobId(ptr->getJobId()), SIGCONT) == -1){
                perror("smash error: kill failed");
                return;
            }
            else{
                ptr->changeIsStopped(false);
            }

        }
        else{
            std::cerr<<"smash error: bg: there is no stopped jobs to resume"<<std::endl;
            return;
        }
    }
    else{
        std::cerr<<"smash error: bg: invalid arguments"<<std::endl;
        return;
    }
}

//class quit
QuitCommand::QuitCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs):
        BuiltInCommand(cmd_line, args), jobs_ptr(jobs){
}

void QuitCommand::execute(){
    if(args.size() > 1 && args[1] == "kill"){ // if the first argument is kill
        std::cout << "smash: sending SIGKILL signal to " << jobs_ptr->getNumOfJobs() << " jobs:" << std::endl;
        for(std::list<JobsList::JobEntry>::iterator it = jobs_ptr->jobs_list.begin();
            it != jobs_ptr->jobs_list.end(); ++it){
            std::cout << it->getCommand()->getPid() << ": " << std::string(it->getCommand()->getCmdLineUnmodified())
                      << std::endl;
            pid_t job_pid = jobs_ptr->getPidByJobId(it->getJobId());
            if(kill(job_pid, SIGKILL) == -1){
                perror("smash error: kill failed");
                return;
            }
        }
    }
    exit(0);
}



//class head
HeadCommand::HeadCommand(const char *cmd_line,std::vector<std::string> args) :
        BuiltInCommand(cmd_line,args){
}

void HeadCommand::execute() {
    size_t numOfLines;
    std::string path;
    if (args.size()==2){
        numOfLines = N;
        path = args[1];
    }
    else if(args.size()==3 && stringIsNumber(args[1])){
        numOfLines = std::stoi(args[1])*(-1);
        path = args[2];
    }
    else{
        std::cerr<<"smash error: head: not enough arguments"<<std::endl;
        return ;
    }
    int file=open(path.c_str(),O_RDONLY);
    if(file==-1){
        perror("smash error: open failed");
        return;
    }
    char ch;
    size_t lines = 0;
    size_t readRes = 0;
    while((readRes=read(file,&ch,1))!=0){//head -1000000 tmp.txt
        if(readRes<0){
            perror("smash error: read failed");
            return;
        }
        size_t res = write(STDOUT_FILENO,&ch,1);
        if(res<0){
            perror("smash error: write failed");
            return;
        }
        if(ch=='\n'){
            lines ++;
        }
        if(lines==numOfLines){
            break;
        }
    }
    if(close(file)<0){
        perror("smash error: close failed");
        return;
    }
}

//class redirection
RedirectionCommand::RedirectionCommand(const char* cmd_line, std::vector<std::string> args): Command(cmd_line,args) {
}

void RedirectionCommand::execute(){
    std::string cmd_str = _trim(std::string(cmd_line)); // modified
    int fd = -1;
    int position = cmd_str.find('>');
    if(cmd_str[position + 1] == '>') { // means the redirection is of the form >>
        fd = open(_trim(cmd_str.substr(position + 2, cmd_str.length())).c_str(), O_CREAT | O_RDWR | O_APPEND, 0666);

    }
    else{ // means the redirection is of the form >
        fd = open(_trim(cmd_str.substr(position + 1, cmd_str.length())).c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
    }
    if(fd == -1){
        perror("smash error: open failed");
        return;
    }
    int out_fd = dup(1); // copy standard output
    if(out_fd == -1){
        perror("smash error: dup failed");
        return;
    }
    if(dup2(fd, 1) == -1){ // overwrite standard output with file
        perror("smash error: dup2 failed");
        return;
    }
    SmallShell& smash = SmallShell::getInstance();
    smash.executeCommand(cmd_str.substr(0, position).c_str());
    close(fd);
    if(dup2(out_fd, 1) == -1){
        perror("smash error: dup2 failed");
        return;
    }
}

//class pipe
PipeCommand::PipeCommand(const char *cmd_line,std::vector<std::string> args):
        Command(cmd_line,args){}

void PipeCommand::execute() {
    std::string cmd_str = _trim(std::string(cmd_line));
    int filedes[2];
    SmallShell& smash = SmallShell::getInstance();
    if(pipe(filedes)==-1){
        perror("smash error: pipe failed");
        return;
    }
    int position = cmd_str.find('|');
    if(cmd_str[position + 1] == '&') { // means the pipe is of the form |&
        pid_t pid1 = fork();
        pid_t pid2 = -1;
        if (pid1 == 0) { //command1
            if (close(filedes[0]) == -1) {
                perror("smash error: close failed");
                return;
            }
            if (close(2) == -1) {
                perror("smash error: close failed");
                return;
            }
            if (dup2(filedes[1], 2) == -1) { // overwrite standard output with pipe
                perror("smash error: dup2 failed");
                return;
            }
            smash.executeCommand(cmd_str.substr(0, position - 1).c_str());
            exit(0);
        } else { //father code
            if ((pid2 = fork()) == 0) { //command 2
                if (close(filedes[1]) == -1) {
                    perror("smash error: close failed");
                    return;
                }
                if (close(0) == -1) {
                    perror("smash error: close failed");
                    return;
                }
                if (dup2(filedes[0], 0) == -1) { // overwrite standard input with pipe
                    perror("smash error: dup2 failed");
                    return;
                }
                smash.executeCommand(cmd_str.substr(position + 2).c_str());
                exit(0);
            }
            else{
                if(close(filedes[0]) == -1){
                    perror("smash error: close failed");
                    return;
                }
                if(close(filedes[1]) == -1) {
                    perror("smash error: close failed");
                    return;
                }
                if(waitpid(pid1, nullptr,WUNTRACED)==-1){
                    perror("smash error: waitpid failed");
                    return;
                }
                if(waitpid(pid2, nullptr,WUNTRACED)==-1){
                    perror("smash error: waitpid failed");
                    return;
                }

            }
        }
    }
    else{// means the pipe is of the form |
        pid_t pid1 =fork();
        pid_t pid2;
        if(pid1==0) { //command1
            setpgrp();
            if(close(filedes[0])==-1){
                perror("smash error: close failed");
                return;
            }
            if(close(1)==-1){
                perror("smash error: close failed");
                return;
            }
            if(dup2(filedes[1], 1) == -1){ // overwrite standard output with pipe
                perror("smash error: dup2 failed");
                return;
            }
            smash.executeCommand(cmd_str.substr(0,position-1).c_str());
            exit(0);
        }
        else{ //father code
            if((pid2=fork())==0){ //command 2
                setpgrp();
                if(close(filedes[1])==-1){
                    perror("smash error: close failed");
                    return;
                }
                if(close(0)==-1){
                    perror("smash error: close failed");
                    return;
                }
                if(dup2(filedes[0], 0) == -1) { // overwrite standard input with pipe
                    perror("smash error: dup2 failed");
                    return;
                }
                smash.executeCommand(cmd_str.substr(position+1).c_str());
                exit(0);
            }
            else{
                if(close(filedes[0]) == -1){
                    perror("smash error: close failed");
                    return;
                }
                if(close(filedes[1]) == -1) {
                    perror("smash error: close failed");
                    return;
                }
                if(waitpid(pid1, nullptr,WUNTRACED)==-1){
                    perror("smash error: waitpid failed");
                    return;
                }
                if(waitpid(pid2, nullptr,WUNTRACED)==-1){
                    perror("smash error: waitpid failed");
                    return;
                }
            }
        }
    }
}


//class TimeOutEntry
Command* TimeOutList::TimeOutEntry::getCommand() {
    return command;
}

time_t TimeOutList::TimeOutEntry::getEndTime() const{
    return end_time;
}

void TimeOutList::TimeOutEntry::setAmpersand(bool value) {
    dynamic_cast<ExternalCommand*>(command)->setAmpersand(value);
}

//class TimeOutList
void TimeOutList::addCommand(Command *command, time_t end_time) {
    TimeOutList::TimeOutEntry curCommand = TimeOutEntry(command, end_time);
    if(timedCommands.size() == 0){
        timedCommands.push_back(curCommand);
        alarm(difftime(end_time, time(nullptr)));
        return;
    }
    std::list<TimeOutEntry>::iterator it = timedCommands.begin();
    for(; it != timedCommands.end(); ++it){ // calculate valid remaining duration before comparison
        if(it->getEndTime() > end_time){
            break;
        }
    }
    if(it == timedCommands.begin()){// set this as alarm
        timedCommands.insert(it, curCommand);
        alarm(difftime(end_time, time(nullptr)));
        return;
    }
    timedCommands.insert(it, curCommand);
}

void TimeOutList::removeExpiredCommands() {
    std::list<TimeOutEntry>::iterator ptr = timedCommands.begin();
    while(ptr!=timedCommands.end()){
        if(difftime(ptr->getEndTime(), time(nullptr)) <= 0){
            if(kill(ptr->getCommand()->getPid(),0)==0){
                std::cout<<"smash: "<<ptr->getCommand()->getCmdLineUnmodified()<<" timed out!"<<std::endl;
                if(kill(ptr->getCommand()->getPid(),SIGKILL)!=0) {
                    perror("smash error: kill failed");
                }
            }
            ptr = timedCommands.erase(ptr);
        }
        else{
            ++ptr;
        }

    }
    std::list<TimeOutEntry>::iterator it = timedCommands.begin();
    alarm(difftime(it->getEndTime(), time(nullptr)));

}

//class TimeOutCommand
TimeOutCommand::TimeOutCommand(const char* cmd_line, std::vector<std::string> args,bool ampersand,
                               const char* cmd_line_unmodified, JobsList* jobs,TimeOutList* timed):
        ExternalCommand(cmd_line,args,ampersand,cmd_line_unmodified),
        jobs(jobs),timed(timed){}


void TimeOutCommand::execute() {
    if(args.size()<=2){
        std::cout<<"smash error: timeout: invalid arguments"<<std::endl;
        return;
    }
    if(!stringIsNumber(args[1])){
        std::cout<<"smash error: timeout: invalid arguments"<<std::endl;
        return ;
    }
    int duration = std::stoi(args[1]);
    if (duration<=0){
        std::cout<<"smash error: timeout: invalid arguments"<<std::endl;
        return;
    }
    SmallShell& smash = SmallShell::getInstance();
    std::string new_command;//remove timeout and duration from command
    std::vector<std::string> new_args = readCommand(this->getCmdLineUnmodified());//change
    for(unsigned int i=2;i!=new_args.size();i++){
        std::string temp = new_args[i];
        for(unsigned int j = 0; j!=temp.length();j++){
            new_command.push_back(temp[j]);
        }
        if(i!=new_args.size()-1){
            new_command.push_back(' ');
        }
    }
    char* argv[4]; // bash -c are 2 arguments, 1 argument is the string input and null
    argv[0] = (char*)"/bin/bash";
    argv[1] = (char*)"-c";
    char* str = new char[std::string(new_command).size() + 1];
    strcpy(str, new_command.c_str());
    argv[2] = str;
    argv[3] = NULL;;
    pid_t pid1 =fork();
    if(pid1==0) {
        setpgrp();
        execv(argv[0],argv);
        perror("smash error: execv failed");
        delete [] str;
        exit(0);
    }
    else{
        bool bg = _isBackgroundComamnd(new_command.c_str());
        Command* to_add = new ExternalCommand(getCmdLine(), args,bg,getCmdLineUnmodified(),pid1);
        timed->addCommand(to_add,time(nullptr) + duration);
        if(!bg){
            smash.setFgMode(this,pid1,-1);
            if(kill(pid1,0)==0 ){
                if((waitpid(pid1, nullptr, WUNTRACED))==-1) {
                    perror("smash error: waitpid failed");
                    delete[] str;
                    return;
                }
            }
            smash.removeFgMode();
        }
        else{
            Command* to_add1 = new ExternalCommand(getCmdLine(), args,bg,getCmdLineUnmodified());
            smash.getJobListPtr()->addJob(to_add1,false);
            smash.getJobListPtr()->removeFinishedJobs();
            delete to_add1;
        }
        delete [] str;
        delete to_add;
    }
}

//class JobEntry
Command* JobsList::JobEntry::getCommand(){
    return command;
}

time_t JobsList::JobEntry::getTime(){
    return time;
}

int JobsList::JobEntry::getJobId(){
    return job_id;
}

bool JobsList::JobEntry::checkIsStopped(){
    return isStopped;
}

void JobsList::JobEntry::changeIsStopped(bool status){
    isStopped = status;
}

//class JobsList

// find the maximal job_id in the list and creates a new job with job_id = max_job_id + 1
void JobsList::addJob(Command* cmd, bool isStopped){
    this->removeFinishedJobs();
    int max_job_id = 0;
    getLastJob(&max_job_id);
    int cur_job_id = max_job_id + 1;
    JobsList::JobEntry cur_job = JobEntry(cmd, time(nullptr), cur_job_id, isStopped);
    jobs_list.push_back(cur_job);
}

void JobsList::printJobsList(){
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        JobsList::JobEntry job = *it;
        std::cout << "[" << it->getJobId() << "] " << std::string(it->getCommand()->getCmdLineUnmodified())
                  << " : " << it->getCommand()->getPid() << " " <<
                  difftime( time(nullptr),it->getTime()) << " secs";
        if(it->checkIsStopped()){
            std::cout << " (stopped)";
        }
        std::cout << std::endl;
    }
}

void JobsList::killAllJobs(){
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        if(kill(it->getCommand()->getPid(), SIGKILL) == -1){
            perror("smash error: kill failed");
            return;
        }
    }
}

void JobsList::removeFinishedJobs() {
    for (std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end();) {
        pid_t pid1 = it->getCommand()->getPid();
        pid_t pid2 = -1;
        if ((pid2 = waitpid(pid1, nullptr, WNOHANG)) > 0) {
            it = jobs_list.erase(it);
        }
        else{
            ++it;
        }
    }
}

JobsList::JobEntry * JobsList::getJobById(int jobId){
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        if(it->getJobId() == jobId){
            return &*it;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId){
    std::list<JobEntry>::iterator it = jobs_list.begin();
    while(it != jobs_list.end()){
        if(it->getJobId() == jobId){
            it = jobs_list.erase(it); //remove item
            return;
        }
        else{
            ++it;
        }
    }
}

// finds last job and puts into lastJobId its job_id. last job has highest job_id!
JobsList::JobEntry * JobsList::getLastJob(int* lastJobId){
    int max_job_id = 0;
    JobEntry* last_job = nullptr;
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        if(it->getJobId() > max_job_id){
            max_job_id = it->getJobId();
            last_job = &*it;
        }
    }
    if(lastJobId != nullptr){
        *lastJobId = max_job_id;
    }
    return last_job;
}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId){
    int max_job_id = 0;
    JobEntry* last_job = nullptr;
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        if(it->checkIsStopped() == true && it->getJobId() > max_job_id){
            max_job_id = it->getJobId();
            last_job = &*it;
        }
    }
    if(jobId != nullptr){
        *jobId = max_job_id;
    }
    return last_job;
}

void JobsList::printJobById(int jobId){
    JobsList::JobEntry* job_to_print_ptr = getJobById(jobId);
    if(job_to_print_ptr != nullptr) {
        std::cout << std::string(job_to_print_ptr->getCommand()->getCmdLineUnmodified()) << " : " <<
                  job_to_print_ptr->getCommand()->getPid() << std::endl;
    }
}

pid_t JobsList::getPidByJobId(int jobId) {
    for (std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it) {
        if (it->getJobId() == jobId) {
            return it->getCommand()->getPid();
        }
    }
    return 0;
}

int JobsList::getNumOfJobs(){
    int counter = 0;
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        counter++;
    }
    return counter;
}

void JobsList::addFgJob(Command *cmd, int job_id) {
    this->removeFinishedJobs();

    std::list<JobEntry>::iterator it;
    for(it=jobs_list.begin(); it != jobs_list.end();++it){
        if(it->getJobId()>job_id){
            break;
        }
    }
    jobs_list.insert(it,JobEntry(cmd, time(nullptr),job_id, true));

}

//smallShell

SmallShell::SmallShell(){
    name = "smash";
    smash_pid = getpid();
    fg_pid = -1;
    fg_job = -1;
    fg_cmd = nullptr;

}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    Command * cmd = nullptr;
    std::vector<std::string> args = readCommand(cmd_line);
    if(args.size() == 0 || args[0] == ""){
        //std::cout<<"Invalid command" << std::endl; // added it here
        return nullptr;
    }
    std::string cmd_name = args[0];
    char* cmd_line_no_sign = copyFromConst(cmd_line);
    _removeBackgroundSign(cmd_line_no_sign);
    args = readCommand(cmd_line_no_sign);
    if(args.size() == 0){
        delete [] cmd_line_no_sign;
        return nullptr;
    }
    SmallShell& smash = SmallShell::getInstance();
    std::string check_second_ampersand = std::string(cmd_line_no_sign);

    std::string cmd_line_str = std::string(cmd_line);
    if(cmd_line_str.find('>') != std::string::npos){
        cmd = new RedirectionCommand(cmd_line_no_sign, args);
        delete [] cmd_line_no_sign;
        return cmd;
    }
    else if(cmd_line_str.find('|')!=std::string::npos){
        cmd = new PipeCommand(cmd_line_no_sign,args);
        delete [] cmd_line_no_sign;
        return cmd;
    }
    else if(cmd_name == "timeout"){
        cmd = new TimeOutCommand(cmd_line_no_sign,args, _isBackgroundComamnd(cmd_line), cmd_line,
                                 smash.getJobListPtr(),smash.getTimedListPtr());
        delete [] cmd_line_no_sign;
        return cmd;
    }
    if(cmd_name == std::string("chprompt")){
        cmd = new ChangePromptCommand(cmd_line_no_sign, args);
    }
    else if(cmd_name == "showpid"){
        cmd = new ShowPidCommand(cmd_line_no_sign, args);
    }
    else if(cmd_name == "pwd"){
        cmd = new GetCurrDirCommand(cmd_line_no_sign, args);
    }
    else if(cmd_name=="cd"){
        cmd = new ChangeDirCommand(cmd_line_no_sign,args);
    }
    else if(cmd_name == "jobs"){
        cmd = new JobsCommand(cmd_line_no_sign,args, smash.getJobListPtr());
    }
    else if(cmd_name == "kill"){
        cmd = new KillCommand(cmd_line_no_sign,args, smash.getJobListPtr());
    }
    else if(cmd_name == "fg"){
        cmd = new ForegroundCommand(cmd_line_no_sign, args, smash.getJobListPtr());
    }
    else if(cmd_name == "bg"){
        cmd = new BackgroundCommand(cmd_line_no_sign, args, smash.getJobListPtr());
    }
    else if(cmd_name == "quit"){
        cmd = new QuitCommand(cmd_line_no_sign, args, smash.getJobListPtr());
    }
    else if(cmd_name=="head"){
        cmd = new HeadCommand(cmd_line_no_sign,args);
    }
    else{
        cmd = new ExternalCommand(cmd_line_no_sign, args, _isBackgroundComamnd(cmd_line), cmd_line);
    }
    delete[] cmd_line_no_sign;
    return cmd;
}

SmallShell::~SmallShell() {
}

void SmallShell::executeCommand(const char *cmd_line) {
    jobs.removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);
    if(cmd!= nullptr){
        cmd->execute();
        delete cmd;
    }

}

std::string SmallShell::getName() const{
    return name;
}

void SmallShell::changeName(std::string str){
    name = str;
}

std::string SmallShell::getCurDirectory() const {
    char* cur_directory = new char[PATH_MAX];
    if(getcwd(cur_directory, PATH_MAX) == nullptr){
        perror("smash error: getcwd failed");
        return "";
    }
    std::string cur_dir = std::string(cur_directory);
    delete[] cur_directory;
    return cur_dir;
}

pid_t SmallShell::getShellPid() const{
    return smash_pid;
}

JobsList* SmallShell::getJobListPtr() {
    return &jobs;
}

TimeOutList* SmallShell::getTimedListPtr() {
    return &timedCommands;
}

std::string SmallShell::getLastDirectory() {

    return last_directory;
}

void SmallShell::changeCurDirectory(std::string directory) {
    std::string cur_directory = getCurDirectory();
    if(chdir(directory.c_str())==-1){ //const
        perror("smash error: chdir failed");
        return;
    }
    last_directory = cur_directory;
}

void SmallShell::setFgMode(Command *cmd, pid_t pid, int job) {
    fg_pid=pid;
    fg_job = job;
    fg_cmd = new ExternalCommand(cmd->getCmdLine(),cmd->getArgs(),dynamic_cast<ExternalCommand*>(cmd)->getAmpersand(),
                                 cmd->getCmdLineUnmodified(),cmd->getPid());
}

void SmallShell::removeFgMode() {
    fg_pid=-1;
    fg_job = -1;
    delete fg_cmd;
    fg_cmd = nullptr;
}

pid_t SmallShell::getFgPid() const {
    return fg_pid;
}

int SmallShell::getFgJob() const {
    return fg_job;
}

Command* SmallShell::getFgCommand() const{
    return fg_cmd;
}

void SmallShell::removeExpired() {
    timedCommands.removeExpiredCommands();
    jobs.removeFinishedJobs();
}
