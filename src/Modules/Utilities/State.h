#pragma once

#include "Utilities/Singleton.h"
#include <map>

template<typename T>
struct CurState : public SingleTon < CurState<T> >
{
    std::map<std::string, T> state;
};
