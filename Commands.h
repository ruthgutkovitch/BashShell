#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <stack>
#include <string>
#include <limits.h>
#include <time.h>
#include <fcntl.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_CHARACTERS (80)
#define N (10)


const std::string WHITESPACE = " \n\r\t\f\v";

char* copyFromConst(const char* original);

class Command {
protected:
    const char* cmd_line;
    std::vector<std::string> args;
    pid_t pid;
    const char* cmd_line_unmodified;
public:
    Command(const char* cmd_line, std::vector<std::string> args, pid_t pid = 0, const char* cmd_line_unmodified = nullptr);
    virtual ~Command() {
        delete [] cmd_line;
        if(cmd_line_unmodified != nullptr){
            delete [] cmd_line_unmodified;
        }
    }
    virtual void execute() = 0;
    pid_t getPid();
    const char* getCmdLine();
    const char* getCmdLineUnmodified();
    std::vector<std::string> getArgs(){
        return args;
    }
    void setPid(pid_t new_pid){
        pid = new_pid;
    }
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line, std::vector<std::string> args);
    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
    bool ampersand;
public:
    ExternalCommand(const char* cmd_line, std::vector<std::string> args, bool ampersand, const char* cmd_line_unmodified,
                    pid_t pid = 0);
    virtual ~ExternalCommand() {

    }
    void execute() override;
    bool getAmpersand() const{
        return ampersand;
    }
    void setAmpersand(bool value) {
        ampersand = value;
    }

};

class PipeCommand : public Command {
public:
    PipeCommand(const char* cmd_line,std::vector<std::string> args);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
public:
    explicit RedirectionCommand(const char* cmd_line, std::vector<std::string> args);
    virtual ~RedirectionCommand() {}
    void execute() override;

};

class ChangePromptCommand : public BuiltInCommand {
public:
    ChangePromptCommand(const char* cmd_line, std::vector<std::string> args);
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    ChangeDirCommand(const char* cmd_line,std::vector<std::string> args);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line, std::vector<std::string> args);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line,std::vector<std::string> args);
    virtual ~ShowPidCommand() {}
    void execute() override;
};


class JobsList;
class QuitCommand : public BuiltInCommand {
    JobsList* jobs_ptr;
public:
    QuitCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};


class JobsList {

public:
    class JobEntry {
        Command* command;
        time_t time;
        int job_id;
        bool isStopped;
    public:
        JobEntry(Command* command, time_t time, int job_id, bool isStopped):
                command(new ExternalCommand(command->getCmdLine(),
                                            command->getArgs(), true, command->getCmdLineUnmodified(), command->getPid())), time(time),
                job_id(job_id), isStopped(isStopped) {}

        ~JobEntry(){
            delete command;
        };
        JobEntry(const JobEntry& job): command(new ExternalCommand(job.command->getCmdLine(),
                                                                   job.command->getArgs(), true, job.command->getCmdLineUnmodified(), job.command->getPid())),
                                       time(job.time),
                                       job_id(job.job_id), isStopped(job.isStopped)   {}

        Command* getCommand();
        time_t getTime();
        int getJobId();
        bool checkIsStopped();
        void changeIsStopped(bool status);

    };
    std::list<JobEntry> jobs_list;

public:
    JobsList() = default;
    ~JobsList() = default ;
    void addJob(Command* cmd, bool isStopped = false);
    void addTimeoutJob(Command* cmd,pid_t pid, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    void printJobById(int jobId);
    pid_t getPidByJobId(int jobId); // need implement
    int getNumOfJobs();
    void addFgJob(Command* cmd,int job_id);

};

class JobsCommand : public BuiltInCommand {
    JobsList* jobs_ptr;
public:
    JobsCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* jobs_ptr;
public:
    KillCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobs_ptr;
public:
    ForegroundCommand(const char* cmd_line, std::vector<std::string> args, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs_ptr;
public:
    BackgroundCommand(const char* cmd_line,std::vector<std::string> args, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class HeadCommand : public BuiltInCommand {
public:
    HeadCommand(const char* cmd_line,std::vector<std::string> args);
    virtual ~HeadCommand() {}
    void execute() override;
};

class TimeOutList {
public:
    class TimeOutEntry{
        Command* command;
        const time_t end_time;
    public:
        TimeOutEntry(Command* command, time_t end_time):
                command(new ExternalCommand(command->getCmdLine(),
                                            command->getArgs(), dynamic_cast<ExternalCommand*>(command)->getAmpersand(),
                                            command->getCmdLineUnmodified(), command->getPid())),
                end_time(end_time) {}

        ~TimeOutEntry(){
            delete command;
        };
        TimeOutEntry(const TimeOutEntry& timedOut):
                command(new ExternalCommand(timedOut.command->getCmdLine(),timedOut.command->getArgs(),
                                            dynamic_cast<ExternalCommand*>(timedOut.command)->getAmpersand(),
                                            timedOut.command->getCmdLineUnmodified(), timedOut.command->getPid())),
                end_time(timedOut.end_time)
        {}
        Command* getCommand();
        //time_t getTime();
        void setAmpersand(bool value);
        time_t getEndTime() const;
        //void setDuration(int cur_duration);
    };
    std::list<TimeOutEntry> timedCommands;
    TimeOutList() = default;
    ~TimeOutList() =default;
    void addCommand(Command* command,time_t end_time);
    void removeExpiredCommands();
    //void setAlarm(int duration);

};

class TimeOutCommand: public ExternalCommand{
    JobsList* jobs;
    TimeOutList* timed;
    JobsList* TimeoutJobs;
public:
    TimeOutCommand(const char* cmd_line, std::vector<std::string> args,bool ampersand,
                   const char* cmd_line_unmodified, JobsList* jobs,TimeOutList* timed);
    virtual ~TimeOutCommand() {};
    void execute() override;
};



class SmallShell {
private:
    SmallShell();

    std::string name;
    pid_t smash_pid;
    std::string last_directory;
    JobsList jobs;
    pid_t fg_pid;
    int fg_job;
    Command* fg_cmd;
    TimeOutList timedCommands;

public:
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    std::string getName() const;
    void changeName(std::string str);
    std::string getCurDirectory() const;
    pid_t getShellPid() const;
    JobsList* getJobListPtr();
    TimeOutList* getTimedListPtr();
    //int getStackSize() const;
    std::string getLastDirectory();
    void changeCurDirectory(std::string directory);
    void setFgMode(Command* cmd,pid_t pid,int job=-1);
    void removeFgMode();
    pid_t getFgPid() const;
    int getFgJob() const;
    Command* getFgCommand() const;
    void removeExpired();
};

#endif //SMASH_COMMAND_H_
