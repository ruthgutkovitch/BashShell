#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

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
        command_vector.push_back(cur_word);
        if(cmd_line[i] == '\0'){ // check if we reached end of line
            break;
        }
    }
    return command_vector;
}

//checks whether a string is a number
bool stringIsNumber(std::string& str){
    if(str.length()==1){
        return isdigit(str[0]);
    }
    return (!str.empty() && str.find_first_not_of("0123456789", 1) == string::npos) &&
           (str[0] == '-' || str[0] == '+' || isdigit(str[0]));
}

char* copyFromConst(const char* original){
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

int SmallShell::getStackSize() const {
    return dirs.size();
}


std::string SmallShell::getLastDirectory() const { //const??
    std::string currDirectory = dirs.pop();
    std::string lastDirectory = dirs.top();
    dirs.push(currDirectory);
    return lastDirectory;
}

void SmallShell::changeCurDirectory(std::string directory) {
    dirs.push(directory);
}

std::string SmallShell::getName() const{
    return name;
}
void SmallShell::changeName(std::string str){
    name = str;
}

std::string SmallShell::getCurDirectory() const {
    return dirs.top();
}

SmallShell::SmallShell(){
    name = "smash";
    smash_pid = getpid();
    char* cur_directory = new char[PATH_MAX];
    if(getcwd(cur_directory, PATH_MAX) == nullptr){
        //print error
    }
    dirs.push(std::string(cur_directory));
    delete[] cur_directory;

}

SmallShell::~SmallShell() {
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    // For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */

    //bool ampersand = _isBackgroundComamnd(cmd_line);
    //_removeBackgroundSign(cmd_line);
    Command * cmd = nullptr;
    std::vector<std::string> args = readCommand(cmd_line);
    if(args.size() == 0){
        // empty cmd_line
        return nullptr;
    }
    std::string cmd_name = args[0];
    //check not built in commands first - we need to preserve ampersand for them
    //built in command dont need ampersand therefore we will remove it *********************
    char* cmd_line_no_sign = copyFromConst(cmd_line);
    _removeBackgroundSign(cmd_line_no_sign);
    args = readCommand(cmd_line_no_sign);
    if(args.size() == 0){
        // empty cmd_line
        return nullptr;
    }
    SmallShell& smash = SmallShell::getInstance();
    cmd_name = args[0];
    if(cmd_name == std::string("chprompt")){
        cmd = new ChangePromptCommand(cmd_line_no_sign, args);
    }
    else if(cmd_name == "showpid"){
        cmd = new ShowPidCommand(cmd_line_no_sign, args);
    }
    else if(cmd_name == "pwd"){
        cmd = new GetCurrDirCommand(cmd_line_no_sign, args);
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
    else if(cmd_name == "quit"){
        cmd = new QuitCommand(cmd_line_no_sign, args, smash.getJobListPtr());
    }
    delete[] cmd_line_no_sign;
    //built in commands **********************************
    return cmd;
}

Command::Command(const char* cmd_line, std::vector<std::string> args, pid_t pid): cmd_line(cmd_line),
                                                                                  args(args), pid(0)
{
}

pid_t Command::getPid(){
    return pid;
}

const char* Command::getCmdLine(){
    return cmd_line;
}

BuiltInCommand::BuiltInCommand(const char* cmd_line, std::vector<std::string> args):
        Command(cmd_line, args) {}

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
        smash.changeName(args[1]); // change name to first argument and ignore the rest
    }
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line, std::vector<std::string> args):
        BuiltInCommand(cmd_line, args){
}

void GetCurrDirCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    std::cout << smash.getCurDirectory() << std::endl;
}

/*
JobsList::JobEntry(Command* command, time_t time, int job_id, bool isStopped) : command(command),
time(time),
job_id(job_id), isStopped(isStopped) {
    command = command;
    time = time;
    job_id = job_id;
    isStopped = isStopped;
}
 */

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
    return last_job;
}

// find the maximal job_id in the list and creates a new job with job_id = max_job_id + 1
void JobsList::addJob(Command* cmd, bool isStopped){
    int max_job_id = 0;
    getLastJob(&max_job_id);
    int cur_job_id = max_job_id + 1;
    JobsList::JobEntry cur_job(cmd, time(nullptr), cur_job_id, isStopped);
    jobs_list.push_back(cur_job);
}

JobsList::JobEntry * JobsList::getJobById(int jobId){
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        if(it->getJobId() == jobId){
            return &*it;
        }
    }
    return nullptr;
}

void JobsList::printJobById(int jobId){
    JobsList::JobEntry* job_to_print_ptr = getJobById(jobId);
    if(job_to_print_ptr != nullptr) {
        std::cout << std::string(job_to_print_ptr->getCommand()->getCmdLine()) << " : " <<
                  job_to_print_ptr->getCommand()->getPid() << std::endl;
    }
}

void JobsList::printJobsList(){
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        std::cout << "[" << it->getJobId() << "] " << std::string(it->getCommand()->getCmdLine())
                  << " : " << it->getCommand()->getPid() << " " <<
                  difftime(it->getTime(), time(nullptr)) << " secs";
        if(it->checkIsStopped()){
            std::cout << " (stopped)";
        }
        std::cout << std::endl;
    }
}

void JobsList::killAllJobs(){
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        if(kill(it->getCommand()->getPid(), SIGKILL) == -1){
            //error
        }
    }
}

void JobsList::removeJobById(int jobId){
    std::list<JobEntry>::iterator it = jobs_list.begin();
    while(it != jobs_list.end()){
        if(it->getJobId() == jobId){
            jobs_list.erase(it); //remove item
        }
        else{
            ++it;
        }
    }
}

void JobsList::removeFinishedJobs(){
    pid_t pid_finished = 0;
    while(pid_finished = waitpid(-1, nullptr, WNOHANG) > 0){ // while there are finished jobs
        for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
            if(it->getCommand()->getPid() == pid_finished){ // find job to remove
                removeJobById(it->getJobId());
            }
        }
    }
}

int JobsList::getNumOfJobs(){
    int counter = 0;
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        counter++;
    }
    return counter;
}

pid_t JobsList::getPidByJobId(int jobId){
    for(std::list<JobEntry>::iterator it = jobs_list.begin(); it != jobs_list.end(); ++it){
        if(it->getJobId() == jobId){
            return it->getCommand()->getPid();
        }
    }
    return 0;
}

JobsCommand::JobsCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs):
        BuiltInCommand(cmd_line, args), jobs_ptr(jobs){
}


void JobsCommand::execute(){
    jobs_ptr->printJobsList();
}

ForegroundCommand::ForegroundCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs):
        BuiltInCommand(cmd_line, args), jobs_ptr(jobs) {
}

void ForegroundCommand::execute(){
    int job_id = 0;
    if(args.size() == 1){ // no arguments
        int* last_job_id = nullptr;
        JobsList::JobEntry* last_job_ptr = jobs_ptr->getLastJob(last_job_id);
        if(last_job_id == nullptr){
            std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            return;
        }
        job_id = *last_job_id;
    }

    else if(args.size() == 2 && stringIsNumber(args[1])) { // has single argument
        // check if the argument is a positive or negative number
        job_id = std::stoi(args[1]); // converts string to int - hopefully works
        JobsList::JobEntry *job = jobs_ptr->getJobById(job_id);
        if (job == nullptr) {
            //no such job
            std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << std::endl;
            return;
        }

    }
    else{ // more then single agrument - invalid number of arguments
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
    }

    jobs_ptr->printJobById(job_id);
    if(kill(jobs_ptr->getPidByJobId(job_id), SIGCONT) == -1){
        //error
    }
    pid_t job_pid = jobs_ptr->getPidByJobId(job_id);
    if(waitpid(job_pid, nullptr, 0) == -1){
        //perror
    }
    jobs_ptr->removeJobById(job_id);
}

QuitCommand::QuitCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs):
        BuiltInCommand(cmd_line, args), jobs_ptr(jobs){
}

void QuitCommand::execute(){
    if(args.size() > 1 && args[1] == "kill"){ // if the first argument is kill
        std::cout << "sending SIGKILL signal to " << jobs_ptr->getNumOfJobs() << " jobs:" << std::endl;
        for(std::list<JobsList::JobEntry>::iterator it = jobs_ptr->jobs_list.begin();
            it != jobs_ptr->jobs_list.end(); ++it){
            std::cout << it->getCommand()->getPid() << ": " << std::string(it->getCommand()->getCmdLine())
                      << std::endl;
            pid_t job_pid = jobs_ptr->getPidByJobId(it->getJobId());
            if(kill(job_pid, SIGKILL) == -1){
                //perror
            }
        }
    }
    //exit
    exit(0);
}


void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
    delete cmd;
}

pid_t SmallShell::getShellPid() const{
    return smash_pid;
}

JobsList* SmallShell::getJobListPtr() {
    return &jobs;
}

ShowPidCommand::ShowPidCommand(const char *cmd_line,std::vector<std::string> args):BuiltInCommand(cmd_line,args) {
}

void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    std::cout << "smash pid is " << smash.getShellPid() << std::endl;
}

//ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd);

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
                std::cerr << "smash error:kill:job-id <" << job_id << "> does not exist" << std::endl;
                return;
            }
            if(kill(job_pid, signal) == -1) {
                //perror
            }
            std::cout<<"signal number "<< signal <<" was sent to pid "<< job_pid <<std::endl;

        }
    }
    else{
        std::cerr<<"smash error:kill:invalid arguments"<<std::endl;
    }

}

BackgroundCommand::BackgroundCommand(const char *cmd_line, std::vector <std::string> args, JobsList *jobs):
        BuiltInCommand(cmd_line,args),jobs_ptr(jobs){}

void BackgroundCommand::execute() {
    if(args.size() == 2 && stringIsNumber(args[1])){
        int job_id = std::stoi(args[1]);
        JobsList::JobEntry * ptr = jobs_ptr->getJobById(job_id);
        if ( ptr!= nullptr){
            if(ptr->checkIsStopped()){
                if (kill(jobs_ptr->getPidByJobId(job_id), SIGCONT) == -1){
                    //perror
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
    else if(args.size()==1){

        //JobEntry *getLastStoppedJob(int *jobId);
        int* last_stopped_job_id=nullptr;
        JobsList::JobEntry* ptr = jobs_ptr->getLastStoppedJob(last_stopped_job_id);
        if (ptr!= nullptr){
            if (kill(jobs_ptr->getPidByJobId(*last_stopped_job_id), SIGCONT) == -1){
                // perror
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
        //finish this
    else{
        std::cerr<<"smash error: bg: invalid arguments"<<std::endl;
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line,std::vector<std::string> args, char **plastPwd):
BuiltInCommand(cmd_line,args){ ///finish
}

void ChangeDirCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    if (args.size()>2){
        std::cerr<<"smash error: cd: too many arguments"<<std::endl;
        return;
    }
    else if (args.size()==2){
        std::string directory = args[1];
        if (args[1]=="-"){
            if(smash.getStackSize()==1){
                std::cerr<<"smash error: cd: OLDPWD not set"<<std::endl;
                return;
            }
            else{
                directory = smash.getLastDirectory();
            }
        }
        else if (args[1]==".."){
            std::string currDir = smash.getCurDirectory();
            directory = currDir.substr(0,currDir.find_last_of("/"));
        }

        if(chdir(directory)==-1){ //const
            //perror
        }
        else{
            smash.changeCurDirectory(directory);
        }

    }


}

HeadCommand::HeadCommand(const char *cmd_line,std::vector<std::string> args) :
BuiltInCommand(cmd_line,args){
}

void HeadCommand::execute() {
    int numOfLines;
    std::string path;
    if (args.size()==2){
        numOfLines = N;
        path = args[1];
    }
    else if(args.size()==3 && stringIsNumber(args[1])){
        numOfLines = std::stoi(args[1]);
        path = args[2];
    }
    else{
        std::cerr<<"smash error: head: not enough arguments"<<std::endl;
        return ;
    }
    int file=open(path,O_RDONLY);
    if(file==-1){
        //perror("")
    }
    char ch, buffer[INT64_MAX];
    size_t index =0, lines = 0;
    size_t readRes = 0, writeRes=0;
    while((readRes=read(file,&ch,1))!=0){
        if(readRes<0){
            //perror
            return -1;
        }
        if(ch=="\n"){
            buffer[index]=ch;
            buffer[index +1]='\0';
            ch = 0;
            index = 0;
        }
        writeRes = 0;
        size_t buffer_len = strlen(buffer);
        while(writeRes!=buffer_len){
            size_t res = write(STDOUT_FILENO,buffer+writeRes,buffer_len-writeRes);
            if(res<0){
                //perror
                return -1;
            }
            writeRes += res;
        }
        if(lines==numOfLines){
            break;
        }
        lines ++;
        else{
            buffer[index++]=ch
        }


    }
    if(close(file)<0){
        //perror
        return -1;
    }
}


