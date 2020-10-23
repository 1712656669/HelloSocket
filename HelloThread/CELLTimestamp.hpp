#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_

#include <chrono>
using namespace std::chrono;

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
		return this->getElapseTimeInMicroSec() * 0.001;
	}

	//获取微妙
	long long getElapseTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
protected:
	time_point<high_resolution_clock> _begin;
};

#endif //_CELLTimestamp_hpp_
