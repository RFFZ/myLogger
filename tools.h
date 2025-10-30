#ifndef TOOLS_H
#define TOOLS_H

#include <mutex>
#include <condition_variable>
#include <functional>
#include "onceToken.h"
#include <vector>
#if defined(_WIN32)
#include <windows.h>

#elif defined(__linux__)
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#endif

#if !defined(_WIN32)
#define _unlink unlink
#define _rmdir rmdir
#define _access access
#endif

#define INSTANCE_IMP(class_name, ...)                                               \
    class_name &class_name::Instance()                                              \
    {                                                                               \
        static std::shared_ptr<class_name> s_instance(new class_name(__VA_ARGS__)); \
        static class_name &s_instance_ref = *s_instance;                            \
        return s_instance_ref;                                                      \
    }

std::string exePath(bool isExe = true);
std::string exeName(bool isExe = true);
std::string exeDir(bool isExe = true);

time_t getLogFileTime(const std::string &full_path);
bool is_special_dir(const char *path);
bool is_dir(const char *path);

const char *getFileName(const char *file);
void scanDir(const std::string path, const std::function<bool(const std::string &path, bool isDir)> &cb, bool enter_subdir = false);

void local_time_init();
// 无锁 考虑时区 夏令时的时间  线程安全
void no_locks_localtime(struct tm *tmp, time_t t);
std::string getTimeStr(const char *fmt, time_t time = 0);
struct tm getLocalTime(time_t sec);

bool end_with(const std::string &str, const std::string &substr);
bool start_with(const std::string &str, const std::string &substr);

std::string getThreadName();
long getGMTOff();
std::vector<std::string> split(const std::string &s, const char *delim);

class semphore
{
public:
    semphore(int count = 0);
    ~semphore();

    void post(int count = 1);
    void wait();

private:
    int _count;
    std::recursive_mutex _mutex;
    std::condition_variable_any _cond;
};

// 禁止拷贝基类
class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}

private:
    // 禁止拷贝
    noncopyable(const noncopyable &that) = delete;
    noncopyable(noncopyable &&that) = delete;
    noncopyable &operator=(const noncopyable &that) = delete;
    noncopyable &operator=(noncopyable &&that) = delete;
};

#endif