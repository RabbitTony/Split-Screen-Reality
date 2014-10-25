#ifndef stopwatchHDR
#define stopwatchHDR

#include <ctime>

class stopwatch 
{

	private:

		volatile clock_t start;
		volatile double alarmTime;
		volatile double alarmTime_us;
		volatile bool setAlarm;
		volatile bool setAlarm_us;

	public:

		stopwatch() 
		{
			setAlarm = false;
			alarmTime = 0;
			alarmTime_us = 0;
			setAlarm_us = false;

			start = clock();
		}

		void alarm(double);
		bool isAlarming(void);
		void reset(void);
		double read(void);
		
		void alarm_us(double);
		double read_us(void);
};

#endif
