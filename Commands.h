#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <stack>
#include <string>
#include <limits.h>
#include <time.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_CHARACTERS (80)

const std::string WHITESPACE = " \n\r\t\f\v";

class Command {
protected:
    const char* cmd_line;
    std::vector<std::string> args;
    pid_t pid;
    const char* cmd_line_unmodified;
public:
    Command(const char* cmd_line, std::vector<std::string> args, pid_t pid = 0, const char* cmd_line_unmodified = nullptr);
    virtual ~Command() {
    }
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    pid_t getPid();
    virtual const char* getCmdLine();
    const char* getCmdLineUnmodified();
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
    virtual ~ExternalCommand() {}
    void execute() override;
    const char* getCmdLine() override;
    //char* readArguments(std::vector<std::string> args);
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand {
public:
    ChangePromptCommand(const char* cmd_line, std::vector<std::string> args);
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
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
                command(command), time(time),
                job_id(job_id), isStopped(isStopped) {}
        ~JobEntry() = default;
        Command* getCommand();
        time_t getTime();
        int getJobId();
        bool checkIsStopped();
        void changeIsStopped(bool status);

    };
    std::list<JobEntry> jobs_list;

public:
    JobsList() = default;
    ~JobsList(); // delete all commands
    void addJob(Command* cmd, bool isStopped = false);
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
    HeadCommand(const char* cmd_line);
    virtual ~HeadCommand() {}
    void execute() override;
};


class SmallShell {
private:
    SmallShell();

    std::string name;
    pid_t smash_pid;
    std::stack<std::string> dirs;
    JobsList jobs;
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
};

#endif //SMASH_COMMAND_H_
