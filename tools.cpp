#include "tools.h"
#include <cstring>
static int _daylight_active;
static long _current_timezone;
int get_daylight_active()
{
    return _daylight_active;
}

static int is_leap_year(time_t year)
{
    if (year % 4)
        return 0; /* A year not divisible by 4 is not leap. */
    else if (year % 100)
        return 1; /* If div by 4 and not 100 is surely leap. */
    else if (year % 400)
        return 0; /* If div by 100 *and* not by 400 is not leap. */
    else
        return 1; /* If div by 100 and 400 is leap. */
}

semphore::semphore(int count)
{
    _count = count;
}
semphore::~semphore()
{
}

void semphore::post(int count)
{
    std::unique_lock<std::recursive_mutex> lock(_mutex);
    _count += count;

    if (count <= 1)
    {
        _cond.notify_one();
    }
    else
    {
        _cond.notify_all();
    }
}

void semphore::wait()
{
    std::unique_lock<std::recursive_mutex> lock(_mutex);
    while (_count == 0)
    {
        _cond.wait(lock);
    }
    --_count;
}

std::string exeDir(bool isExe /*= true*/)
{
    auto path = exePath(isExe);
    return path.substr(0, path.rfind('/') + 1);
}

std::string exeName(bool isExe /*= true*/)
{
    auto path = exePath(isExe);
    return path.substr(path.rfind('/') + 1);
}

std::string exePath(bool isExe)
{
    char buffer[PATH_MAX * 2 + 1] = {0};
    int n = -1;

#if defined(_WIN32)
    n = GetModuleFileNameA(isExe ? nullptr : (HINSTANCE)&__ImageBase, buffer, sizeof(buffer));
    for (auto &ch : buffer)
    {
        if (ch == '\\')
            ch = '/';
    }
#elif defined(__linux__)
    n = readlink("/proc/self/exe", buffer, sizeof(buffer));
#endif
    std::string filePath;
    if (n <= 0)
        filePath = "./";
    else
        filePath = buffer;

    return filePath;
}

std::string getTimeStr(const char *fmt, time_t time)
{
    if (!time)
    {
        time = ::time(nullptr);
    }
    auto tm = getLocalTime(time);
    size_t size = strlen(fmt) + 64;
    std::string ret;
    ret.resize(size);
    size = std::strftime(&ret[0], size, fmt, &tm);
    if (size > 0)
    {
        ret.resize(size);
    }
    else
    {
        ret = fmt;
    }
    return ret;
}

struct tm getLocalTime(time_t sec)
{
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &sec);
#else
    no_locks_localtime(&tm, sec);
#endif //_WIN32
    return tm;
}

// 根据日志文件名返回GMT UNIX时间戳
time_t getLogFileTime(const std::string &full_path)
{
    auto name = getFileName(full_path.data());
    struct tm tm{0};
    if (!strptime(name, "%Y-%m-%d", &tm))
    {
        return 0;
    }
    // 此函数会把本地时间转换成GMT时间戳
    return mktime(&tm);
}

bool is_special_dir(const char *path)
{
    return strcmp(path, ".") == 0 || strcmp(path, "..") == 0;
}

bool is_dir(const char *path)
{
    auto dir = opendir(path);
    if (!dir)
    {
        return false;
    }
    closedir(dir);
    return true;
}

const char *getFileName(const char *file)
{
    auto pos = strrchr(file, '/');
#ifdef _WIN32
    if (!pos)
    {
        pos = strrchr(file, '\\');
    }
#endif
    return pos ? pos + 1 : file;
}

// 获取系统时区配置
void local_time_init()
{
    /* Obtain timezone and daylight info. */
    tzset(); /* Now 'timezome' global is populated. */
#if defined(__linux__) || defined(__sun)
    _current_timezone = timezone;
#elif defined(_WIN32)
    time_t time_utc;
    struct tm tm_local;

    // Get the UTC time
    time(&time_utc);

    // Get the local time
    // Use localtime_r for threads safe for linux
    // localtime_r(&time_utc, &tm_local);
    localtime_s(&tm_local, &time_utc);

    time_t time_local;
    struct tm tm_gmt;

    // Change tm to time_t
    time_local = mktime(&tm_local);

    // Change it to GMT tm
    // gmtime_r(&time_utc, &tm_gmt);//linux
    gmtime_s(&tm_gmt, &time_utc);

    int time_zone = tm_local.tm_hour - tm_gmt.tm_hour;
    if (time_zone < -12)
    {
        time_zone += 24;
    }
    else if (time_zone > 12)
    {
        time_zone -= 24;
    }

    _current_timezone = time_zone;
#else
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    _current_timezone = tz.tz_minuteswest * 60L;
#endif
    time_t t = time(NULL);
    struct tm *aux = localtime(&t);
    _daylight_active = aux->tm_isdst;
}

// 获取时间戳对应的本地时间
void no_locks_localtime(struct tm *tmp, time_t t)
{
    const time_t secs_min = 60;
    const time_t secs_hour = 3600;
    const time_t secs_day = 3600 * 24;

    t -= _current_timezone;            /* Adjust for timezone. */
    t += 3600 * get_daylight_active(); /* Adjust for daylight time. */
    time_t days = t / secs_day;        /* Days passed since epoch. */
    time_t seconds = t % secs_day;     /* Remaining seconds. */

    tmp->tm_isdst = get_daylight_active();
    tmp->tm_hour = seconds / secs_hour;
    tmp->tm_min = (seconds % secs_hour) / secs_min;
    tmp->tm_sec = (seconds % secs_hour) % secs_min;
#ifndef _WIN32
    tmp->tm_gmtoff = -_current_timezone;
#endif
    /* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure
     * where sunday = 0, so to calculate the day of the week we have to add 4
     * and take the modulo by 7. */
    tmp->tm_wday = (days + 4) % 7;

    /* Calculate the current year. */
    tmp->tm_year = 1970;
    while (1)
    {
        /* Leap years have one day more. */
        time_t days_this_year = 365 + is_leap_year(tmp->tm_year);
        if (days_this_year > days)
            break;
        days -= days_this_year;
        tmp->tm_year++;
    }
    tmp->tm_yday = days; /* Number of day of the current year. */

    /* We need to calculate in which month and day of the month we are. To do
     * so we need to skip days according to how many days there are in each
     * month, and adjust for the leap year that has one more day in February. */
    int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    mdays[1] += is_leap_year(tmp->tm_year);

    tmp->tm_mon = 0;
    while (days >= mdays[tmp->tm_mon])
    {
        days -= mdays[tmp->tm_mon];
        tmp->tm_mon++;
    }

    tmp->tm_mday = days + 1; /* Add 1 since our 'days' is zero-based. */
    tmp->tm_year -= 1900;    /* Surprisingly tm_year is year-1900. */
}

void scanDir(const std::string path, const std::function<bool(const std::string &path, bool isDir)> &cb, bool enter_subdir)
{
    std::string dir = path;
    if (dir.back() == '/')
        dir.pop_back();

    DIR *pDir;
    dirent *pDirent;
    if ((pDir = opendir(dir.c_str())) == nullptr)
    {
        return;
    }
    while ((pDirent = readdir(pDir)) != nullptr)
    {
        if (is_special_dir(pDirent->d_name))
        {
            continue;
        }
        if (pDirent->d_name[0] == '.')
        {
            continue;
        }

        std::string strAbsolutePath = dir + "/" + pDirent->d_name;
        bool isDir = is_dir(strAbsolutePath.c_str());
        if (!cb(strAbsolutePath, isDir))
        {
            break;
        }
        if (isDir && enter_subdir)
        {
            scanDir(strAbsolutePath, cb, enter_subdir);
        }
    }
    closedir(pDir);
}

bool start_with(const std::string &str, const std::string &substr)
{
    return str.find(substr) == 0;
}

bool end_with(const std::string &str, const std::string &substr)
{
    auto pos = str.rfind(substr);
    return pos != std::string::npos && pos == str.size() - substr.size();
}

static long s_gmtoff = 0; // 时间差
static onceToken s_token([]()
                         {
#ifdef _WIN32
                             TIME_ZONE_INFORMATION tzinfo;
                             DWORD dwStandardDaylight;
                             long bias;
                             dwStandardDaylight = GetTimeZoneInformation(&tzinfo);
                             bias = tzinfo.Bias;
                             if (dwStandardDaylight == TIME_ZONE_ID_STANDARD)
                             {
                                 bias += tzinfo.StandardBias;
                             }
                             if (dwStandardDaylight == TIME_ZONE_ID_DAYLIGHT)
                             {
                                 bias += tzinfo.DaylightBias;
                             }
                             s_gmtoff = -bias * 60; // 时间差(分钟)
#else
                             local_time_init();
                             s_gmtoff = getLocalTime(time(nullptr)).tm_gmtoff;
#endif // _WIN32
                         });

long getGMTOff()
{
    return s_gmtoff;
}

std::vector<std::string> split(const std::string &s, const char *delim)
{
    std::vector<std::string> ret;
    size_t last = 0;
    auto index = s.find(delim, last);
    while (index != std::string::npos)
    {
        if (index - last > 0)
        {
            ret.push_back(s.substr(last, index - last));
        }
        last = index + strlen(delim);
        index = s.find(delim, last);
    }
    if (!s.size() || s.size() - last > 0)
    {
        ret.push_back(s.substr(last));
    }
    return ret;
}

std::string getThreadName()
{
#if ((defined(__linux) || defined(__linux__)) && !defined(ANDROID)) || (defined(__MACH__) || defined(__APPLE__)) || (defined(ANDROID) && __ANDROID_API__ >= 26) || defined(__MINGW32__)
    std::string ret;
    ret.resize(32);
    auto tid = pthread_self();
    pthread_getname_np(tid, (char *)ret.data(), ret.size());
    if (ret[0])
    {
        ret.resize(strlen(ret.data()));
        return ret;
    }
    return std::to_string((uint64_t)tid);
#elif defined(_MSC_VER)
    using GetThreadDescriptionFunc = HRESULT(WINAPI *)(_In_ HANDLE hThread, _In_ PWSTR * ppszThreadDescription);
    static auto getThreadDescription = reinterpret_cast<GetThreadDescriptionFunc>(::GetProcAddress(::GetModuleHandleA("Kernel32.dll"), "GetThreadDescription"));

    if (!getThreadDescription)
    {
        std::ostringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    }
    else
    {
        PWSTR data;
        HRESULT hr = getThreadDescription(GetCurrentThread(), &data);
        if (SUCCEEDED(hr) && data[0] != '\0')
        {
            char threadName[MAX_PATH];
            size_t numCharsConverted;
            errno_t charResult = wcstombs_s(&numCharsConverted, threadName, data, MAX_PATH - 1);
            if (charResult == 0)
            {
                LocalFree(data);
                std::ostringstream ss;
                ss << threadName;
                return ss.str();
            }
            else
            {
                if (data)
                {
                    LocalFree(data);
                }
                return to_string((uint64_t)GetCurrentThreadId());
            }
        }
        else
        {
            if (data)
            {
                LocalFree(data);
            }
            return to_string((uint64_t)GetCurrentThreadId());
        }
    }
#else
    if (!thread_name.empty())
    {
        return thread_name;
    }
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
#endif
}
