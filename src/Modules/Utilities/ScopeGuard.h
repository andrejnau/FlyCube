#pragma once

#include <deque>
#include <functional>

class ScopeGuard {
public:
    using HandlerType = std::function<void()>;

    ScopeGuard(const HandlerType& fn)
    {
        m_deque.push_front(fn);
    }

    ~ScopeGuard()
    {
        for (auto& handler : m_deque) {
            handler();
        }
    }

    ScopeGuard& operator+=(const HandlerType& fn)
    {
        m_deque.push_front(fn);
        return *this;
    }

private:
    std::deque<HandlerType> m_deque;
};
