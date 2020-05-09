#pragma once

template<typename T>
class IDestroyer
{
public:
    virtual void QueryOnDelete(T resource) = 0;
};
