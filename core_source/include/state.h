#pragma once

#include <singleton.h>
#include <map>

template<typename T>
struct CurState : public SingleTon < CurState<T> >
{
    std::map<std::string, T> state;
};