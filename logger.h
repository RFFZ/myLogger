#include <memory>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <list>
#include <set>
#include "tools.h"

class LogContext;
class Logger;
class LogChannel;

using LogContextPtr = std::shared_ptr<LogContext>;

typedef enum
{
    LTrace = 0,
    LDebug,
    LInfo,
    LWarn,
    LError
} LogLevel;

Logger &getLogger();
void setLogger(Logger *logger);

///////////////////LogWriter///////////////////
/**
 * 写日志器
 */
class LogWriter : public noncopyable
{
public:
    LogWriter() = default;
    virtual ~LogWriter() = default;

    virtual void write(const LogContextPtr &ctx, Logger &logger) = 0;
};
class LogAsyncWriter : public LogWriter
{
public:
    LogAsyncWriter();
    ~LogAsyncWriter();

private:
    void write(const LogContextPtr &ctx, Logger &logger) override;
    void flushAll();
    void run();

private:
    std::shared_ptr<std::thread> m_thread;
    semphore m_sem;
    Logger &m_pLogInstance;
    std::list<std::pair<LogContextPtr, Logger *>> _pending;
    std::mutex m_mutex;
    bool m_bExit;
};

class LogContext : public std::ostringstream
{
public:
    LogContext() = default;
    LogContext(LogLevel level, const char *file, const char *function, int line, const char *module_name, const char *flag);
    ~LogContext() = default;

    const std::string &str();

    LogLevel _level;
    int _line;
    int _repeat = 0;
    std::string _file;
    std::string _function;
    std::string _thread_name;
    std::string _module_name;
    std::string _flag;
    struct timeval _tv;

private:
    bool _got_content = false;
    std::string _content;
};

class LogCapturer
{
public:
    using Ptr = std::shared_ptr<LogCapturer>;

    LogCapturer(Logger &logger, LogLevel level, const char *file, const char *function, int line, const char *flag = "");
    LogCapturer(const LogCapturer &that);
    ~LogCapturer();

    LogCapturer &operator<<(std::ostream &(*func)(std::ostream &));

    template <typename T>
    LogCapturer &operator<<(T &&data)
    {
        if (!_ctx)
        {
            return *this;
        }
        (*_ctx) << std::forward<T>(data);
        return *this;
    }

private:
    LogContextPtr _ctx;
    Logger &_logger;
};

class LogChannel : public noncopyable
{
public:
    LogChannel(const std::string &name, LogLevel level = LTrace);
    virtual ~LogChannel();
    const std::string &name() const;
    void setLevel(LogLevel level);
    static std::string printTime(const timeval &tv);
    virtual void write(const Logger &logger, const LogContextPtr &ctx) = 0;

protected:
    virtual void format(const Logger &logger, std::ostream &ost, const LogContextPtr &ctx, bool enable_color = true, bool enable_detail = true);

protected:
    std::string _name;
    LogLevel _level;
};

class FileChannelBase : public LogChannel
{
public:
    FileChannelBase(const std::string &name = "FileChannelBase", const std::string &path = exePath() + ".log", LogLevel level = LTrace);
    ~FileChannelBase() override;

    void write(const Logger &logger, const LogContextPtr &logContext) override;
    bool setPath(const std::string &path);
    const std::string &path() const;

protected:
    virtual bool open();
    virtual void close();
    virtual size_t size();

protected:
    std::string _path;
    std::ofstream _fstream;
};

class LogFileChannel : public FileChannelBase
{
public:
    LogFileChannel(const std::string &name = "FileChannel", const std::string &dir = exeDir() + "logs/", LogLevel level = LTrace);
    ~LogFileChannel() override = default;
    void write(const Logger &logger, const LogContextPtr &logContext) override;
    /**
     * 设置日志最大保存天数
     * @param max_day 天数
     */
    void setMaxDay(size_t max_day);

    /**
     * 设置日志切片文件最大大小
     * @param max_size 单位MB
     */
    void setFileMaxSize(size_t max_size);

    /**
     * 设置日志切片文件最大个数
     * @param max_count 个数
     */
    void setFileMaxCount(size_t max_count);

private:
    void changeFile(time_t second);
    void checkSize(time_t second);
    void clean();

private:
    bool _can_write = false;
    // 默认最多保存30天的日志文件
    size_t _log_max_day = 30;
    // 每个日志切片文件最大默认128MB
    size_t _log_max_size = 50;
    // 最多默认保持30个日志切片文件
    size_t _log_max_count = 30;
    // 当前日志切片文件索引
    size_t _index = 0;
    int64_t _last_day = -1;
    time_t _last_check_time = 0;
    std::string _dir;
    std::set<std::string> _log_file_map;
};

class LogConsoleChannel : public LogChannel
{
public:
    LogConsoleChannel(const std::string &name = "ConsoleChannel", LogLevel level = LTrace);
    ~LogConsoleChannel() override = default;

    void write(const Logger &logger, const LogContextPtr &logContext) override;
};

class Logger : public std::enable_shared_from_this<Logger>, public noncopyable
{
public:
    friend class LogAsyncWriter;
    using Ptr = std::shared_ptr<Logger>;
    explicit Logger(const std::string &loggerName);
    ~Logger();

    static Logger &Instance();

    void add_channel(const std::shared_ptr<LogChannel> &channel);
    void del(const std::string &name);

    std::shared_ptr<LogChannel> get(const std::string &name);

    void set_writer(const std::shared_ptr<LogWriter> &writer);
    void setLevel(const LogLevel level);
    const std::string &getName() const;

    void write(const LogContextPtr &logContext);

private:
    void write_channels(const LogContextPtr &logContext);
    void writeChannels_l(const LogContextPtr &logContext);

private:
    LogContextPtr _last_log;
    std::string _logger_name;
    std::shared_ptr<LogWriter> _writer;
    std::map<std::string, std::shared_ptr<LogChannel>> _channels;
};

extern Logger *g_defaultLogger;
#define WriteL(level) LogCapturer(getLogger(), level, __FILE__, __FUNCTION__, __LINE__)
#define TraceL WriteL(LTrace)
#define DebugL WriteL(LDebug)
#define InfoL WriteL(LInfo)
#define WarnL WriteL(LWarn)
#define ErrorL WriteL(LError)