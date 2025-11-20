#pragma once

#include <deque>
#include <functional>

class ScopeGuard {
public:
    using HandlerType = std::function<void()>;

    ScopeGuard(const HandlerType& fn)
    {
        deque_.push_front(fn);
    }

    ~ScopeGuard()
    {
        for (auto& handler : deque_) {
            handler();
        }
    }

    ScopeGuard& operator+=(const HandlerType& fn)
    {
        deque_.push_front(fn);
        return *this;
    }

private:
    std::deque<HandlerType> deque_;
};
