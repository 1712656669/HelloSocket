#ifndef _CELL_TIMESTAMP_HPP_
#define _CELL_TIMESTAMP_HPP_

#include <chrono>

using namespace std::chrono;

class CELLTime
{
public:
	//获取当前时间戳  毫秒
	static time_t getNowInMilliSec()
	{
		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}

};

class CELLTimestamp
{
public:
	CELLTimestamp()
	{
		update();
	}

	~CELLTimestamp()
	{}

	void update()
	{
		_begin = high_resolution_clock::now();
	}

	//获取当前妙
	double getElapseSecond()
	{
		return getElapseTimeInMicroSec() * 0.000001;
	}

	//获取毫妙
	double getElapseTimeInMilliSec()
	{
		return getElapseTimeInMicroSec() * 0.001;
	}

	//获取微妙
	long long getElapseTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	time_point<high_resolution_clock> _begin;
};

#endif // !_CELL_TIMESTAMP_HPP_
