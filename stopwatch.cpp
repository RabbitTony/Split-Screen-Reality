#include "stopwatch.h"
#include <time.h>

void stopwatch::alarm(double time)
{
	//time in ms

	if (time <= 0) return;

	setAlarm = true;
	setAlarm_us = false;
	alarmTime = time;
}

bool stopwatch::isAlarming(void)
{
	clock_t check = clock();
	if (setAlarm_us == false && setAlarm == true)
	{
		double tms = double(check - start);
		tms = tms / CLOCKS_PER_SEC;
		tms = tms * 1000.0;
		if (tms >= alarmTime ) return true;
	}
	else if (setAlarm_us == true && setAlarm == false)
	{
		double tms = double(check - start);
		tms = tms / CLOCKS_PER_SEC;
		tms = tms * 1000000.0;
		if (tms >= alarmTime_us ) return true;
	}
	
	return false;
}

void stopwatch::reset(void)
{
	start = clock();
}

double stopwatch::read(void)
{
	//time in ms
	
	clock_t check = clock();
	double tt = double(check - start);
	tt = tt / CLOCKS_PER_SEC;
	tt = tt * 1000.0;
	return tt;

}

void stopwatch::alarm_us(double time)
{
	setAlarm = false;
	setAlarm_us = true;
	alarmTime_us = time;
}

double stopwatch::read_us(void)
{
	//time in us

	clock_t check = clock();
	double tt = double(check - start);
	tt = tt / CLOCKS_PER_SEC;
	tt = tt * 1000000.0;
	return tt;
}

