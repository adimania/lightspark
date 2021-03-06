/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#include <semaphore.h>
#include <iostream>

enum LOG_LEVEL { LOG_NO_INFO=0, LOG_ERROR=1, LOG_NOT_IMPLEMENTED=2,LOG_CALLS=3,LOG_TRACE=4};

#define LOG(level,esp)					\
{							\
	if(level<=Log::getLevel())			\
	{						\
		Log l(level);				\
		l() << esp << std::endl;		\
	}						\
}

class Log
{
private:
	static sem_t mutex;
	static bool loggingInited;
	LOG_LEVEL cur_level;
	bool valid;
	static const char* level_names[];
	static LOG_LEVEL log_level;

public:
	Log(LOG_LEVEL l);
	~Log();
	std::ostream& operator()();
	operator bool() { return valid; }
	static void initLogging(LOG_LEVEL l);
	static LOG_LEVEL getLevel(){return log_level;}

};

#endif
