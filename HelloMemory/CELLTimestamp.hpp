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

	//��ȡ��ǰ��
	double getElapseSecond()
	{
		return getElapseTimeInMicroSec() * 0.000001;
	}

	//��ȡ����
	double getElapseTimeInMilliSec()
	{
		return getElapseTimeInMicroSec() * 0.001;
	}

	//��ȡ΢��
	long long getElapseTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
protected:
	time_point<high_resolution_clock> _begin;
};

#endif //_CELLTimestamp_hpp_
