#include "mlogger.h"

namespace mlog
{
    logger::logger()
    {
        _severity.compare = [](const severity_level &lhs, const severity_level& rhs) -> bool
        {
            return lhs >= rhs;
        };
        _severity.value = mlog::trace;
    }

    filter_level filter_level::operator <= (const severity_level& request)
    {
        filter_level res;
        res.compare = [](const severity_level &lhs, const severity_level& rhs) -> bool
        {
            return lhs <= rhs;
        };
        res.value = request;
        return res;
    }

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

    filter_level logger::severity() const
    {
        return _severity;
    }

    void logger::set_filter(const filter_level& filter)
    {
        _severity = filter;
    }

    logger& logger::get()
    {
        static logger instance;
        return instance;
    }

    bool logger::check_severity(const severity_level& request) const
    {
        bool res = _severity.compare(request, _severity.value);
        return res;
    }
}