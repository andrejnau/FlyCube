#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>

#if defined(__ANDROID__) || defined(ANDROID)
#include "android/log.h"

#include <jni.h>
#endif

#if defined(__ANDROID__) || defined(ANDROID)
#ifndef LOG_TAG
#define LOG_TAG "logger"
#endif
#define MLOG_RUN(lvl, ...) __android_log_print(mlog::get_value(lvl), LOG_TAG, __VA_ARGS__);
#else
#define MLOG_RUN(lvl, ...) \
    printf(__VA_ARGS__);   \
    printf("\n");          \
    fflush(stdout);
#endif

#define MLOG(lvl, ...)                           \
    if (mlog::logger::get().check_severity(lvl)) \
        MLOG_RUN(lvl, __VA_ARGS__);

#define DBG(...) MLOG(mlog::debug, __VA_ARGS__);

namespace mlog {
enum severity_level { trace, debug, info, warning, error, fatal };

#if defined(__ANDROID__) || defined(ANDROID)
static int32_t get_value(const severity_level& lvl)
{
    switch (lvl) {
    case trace:
        return ANDROID_LOG_VERBOSE;
    case debug:
        return ANDROID_LOG_DEBUG;
    case info:
        return ANDROID_LOG_INFO;
    case warning:
        return ANDROID_LOG_WARN;
    case error:
        return ANDROID_LOG_ERROR;
    case fatal:
        return ANDROID_LOG_FATAL;
    default:
        return ANDROID_LOG_VERBOSE;
    }
}
#endif

struct filter_level {
    using compare_type = std::function<bool(const severity_level&, const severity_level&)>;
    compare_type compare;
    severity_level value;

#if 0
        filter_level filter_level::operator >= (const severity_level& request)
        {
            filter_level res;
            res.compare = [](const severity_level &lhs, const severity_level& rhs) -> bool
            {
                return lhs >= rhs;
            };
            res.value = request;
            return res;
        }

        filter_level filter_level::operator< (const severity_level& request)
        {
            filter_level res;
            res.compare = [](const severity_level &lhs, const severity_level& rhs) -> bool
            {
                return lhs < rhs;
            };
            res.value = request;
            return res;
        }

        filter_level filter_level::operator>(const severity_level& request)
        {
            filter_level res;
            res.compare = [](const severity_level &lhs, const severity_level& rhs) -> bool
            {
                return lhs > rhs;
            };
            res.value = request;
            return res;
        }

        filter_level filter_level::operator == (const severity_level& request)
        {
            filter_level res;
            res.compare = [](const severity_level &lhs, const severity_level& rhs) -> bool
            {
                return lhs == rhs;
            };
            res.value = request;
            return res;
        }
#endif
};

class logger {
public:
    static logger& get()
    {
        static logger instance;
        return instance;
    }
    void set_filter(const filter_level& filter)
    {
        _severity = filter;
    }
    bool check_severity(const severity_level& request) const
    {
        bool res = _severity.compare(request, _severity.value);
        return res;
    }
    filter_level severity() const
    {
        return _severity;
    }

private:
    filter_level _severity;
    logger()
    {
        _severity.compare = [](const severity_level& lhs, const severity_level& rhs) -> bool { return lhs >= rhs; };
        _severity.value = mlog::trace;
    }
    logger(const logger&) = delete;
    logger(logger&&) = delete;
    logger& operator=(const logger&) = delete;
    logger& operator=(logger&&) = delete;
};
} // namespace mlog
