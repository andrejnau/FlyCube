#pragma once

template <typename T>
class SingleTon
{
public:
	static T & Instance()
	{
		static T instance;
		return instance;
	}
protected:
	SingleTon()
	{
	}
	virtual ~SingleTon() { }
	SingleTon(const SingleTon&) = delete;
	SingleTon& operator=(const SingleTon&) = delete;
};
