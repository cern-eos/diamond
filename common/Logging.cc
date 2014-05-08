// ----------------------------------------------------------------------
// File: Logging.cc
// Author: Andreas-Joachim Peters - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * DIAMOND - the CERN Disk Storage System                                   *
 * Copyright (C) 2011 CERN/Switzerland                                  *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *           A                                                           *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

/*----------------------------------------------------------------------------*/
#include "common/Namespace.hh"
#include "common/Logging.hh"
/*----------------------------------------------------------------------------*/
#include <pthread.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <unistd.h>
/*----------------------------------------------------------------------------*/

DIAMONDCOMMONNAMESPACE_BEGIN

int Logging::gLogMask = 0;
int Logging::gPriorityLevel = 0;
int Logging::gShortFormat = 0;

Logging::LogArray Logging::gLogMemory;
Logging::LogCircularIndex Logging::gLogCircularIndex;
unsigned long Logging::gCircularIndexSize;
pthread_mutex_t Logging::gMutex;
std::string Logging::gUnit = "none";
std::set<std::string> Logging::gAllowFilter;
std::set<std::string> Logging::gDenyFilter;
std::map<std::string, FILE*> Logging::gLogFanOut;


/*----------------------------------------------------------------------------*/
/** 
 * Should log function
 * 
 * @param func name of the calling function
 * @param priority priority level of the message
 */

/*----------------------------------------------------------------------------*/

bool
Logging::shouldlog (const char* func, int priority)
{
  // short cut if log messages are masked
  if (!((LOG_MASK(priority) & gLogMask)))
    return false;

  // apply filter to avoid message flooding for debug messages
  if (priority >= LOG_INFO)
  {
    if (gDenyFilter.size())
    {
      // this is a normal filter by function name
      if (gDenyFilter.count(func))
      {
        return false;
      }
    }
  }
  return true;
}

/*----------------------------------------------------------------------------*/
/** 
 * Logging function
 * 
 * @param func name of the calling function
 * @param file name of the source file calling
 * @param line line in the source file
 * @param logid log message identifier
 * @param vid virtual id of the caller
 * @param cident client identifier
 * @param priority priority level of the message
 * @param msg the actual log message
 * @return pointer to the log message
 */

/*----------------------------------------------------------------------------*/

const char*
Logging::log (const char* func, const char* file, int line, const char* logid, const char* cident, int priority, const char *msg, ...)
{
  static int logmsgbuffersize = 1024 * 1024;

  // short cut if log messages are masked
  if (!((LOG_MASK(priority) & gLogMask)))
    return "";

  // apply filter to avoid message flooding for debug messages
  if (priority >= LOG_INFO)
  {
    if (gAllowFilter.size())
    {
      // if this is a pass-through filter e.g. we want to see exactly this messages
      if (!gAllowFilter.count(func))
      {
        return "";
      }
    }
    else if (gDenyFilter.size())
    {
      // this is a normal filter by function name
      if (gDenyFilter.count(func))
      {
        return "";
      }
    }
  }

  static char* buffer = 0;

  if (!buffer)
  {
    // 1 M print buffer
    buffer = (char*) malloc(logmsgbuffersize);
  }

  std::string File = file;

  // we show only one hierarchy directory like Acl (assuming that we have only
  // file names like *.cc and *.hh
  File.erase(0, File.rfind("/") + 1);
  File.erase(File.length() - 3);

  static time_t current_time;
  static struct timeval tv;
  static struct timezone tz;
  static struct tm *tm;
  pthread_mutex_lock(&gMutex);

  va_list args;
  va_start(args, msg);


  time(&current_time);
  gettimeofday(&tv, &tz);

  static char linen[16];
  sprintf(linen, "%d", line);

  static char fcident[1024];

  std::string truncname = cident;
  
  // we show only the last 16 bytes of the name
  if (truncname.length() > 16)
  {
    truncname.insert(0, "..");
    truncname.erase(0, truncname.length() - 16);
  }

  char sourceline[64];

  if (gShortFormat)
  {
    tm = localtime(&current_time);
    snprintf(sourceline, sizeof (sourceline) - 1, "%s:%s", File.c_str(), linen);
    sprintf(buffer, "%02d%02d%02d %02d:%02d:%02d time=%lu.%06lu func=%-12s level=%s tid=%016lx source=%-30s ", tm->tm_year - 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, current_time, (unsigned long) tv.tv_usec, func, GetPriorityString(priority), (unsigned long) syscall(SYS_gettid), sourceline);
  }
  else
  {
    sprintf(fcident, "tident=%s", truncname.c_str());
    tm = localtime(&current_time);
    snprintf(sourceline, sizeof (sourceline) - 1, "%s:%s", File.c_str(), linen);
    sprintf(buffer, "%02d%02d%02d %02d:%02d:%02d time=%lu.%06lu func=%-24s level=%s logid=%s unit=%s tid=%016lx source=%-30s %s ", tm->tm_year - 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, current_time, (unsigned long) tv.tv_usec, func, GetPriorityString(priority), logid, gUnit.c_str(), (unsigned long)syscall(SYS_gettid), sourceline, fcident);
  }

  char* ptr = buffer + strlen(buffer);

  // limit the length of the output to buffer-1 length
  vsnprintf(ptr, logmsgbuffersize - (ptr - buffer - 1), msg, args);

  if (gLogFanOut.size())
  {
    // we do log-message fanout
    if (gLogFanOut.count("*"))
    {
      fprintf(gLogFanOut["*"], "%s\n", buffer);
      fflush(gLogFanOut["*"]);
    }
    if (gLogFanOut.count(File.c_str()))
    {
      buffer[15] = 0;

      fprintf(gLogFanOut[File.c_str()], "%s %s%s%s %-30s %s \n", 
              buffer, 
              GetLogColour(GetPriorityString(priority)), 
              GetPriorityString(priority), 
              DIAMOND_TEXTNORMAL, 
	      sourceline,
	      ptr);

      fflush(gLogFanOut[File.c_str()]);
      buffer[15] = ' ';
    }
    else
    {
      if (gLogFanOut.count("#"))
      {
	      buffer[15] = 0;

        fprintf(gLogFanOut["#"], "%s %s%s%s %16s %s \n",
                buffer,
		GetLogColour(GetPriorityString(priority)),
		GetPriorityString(priority),
                DIAMOND_TEXTNORMAL,
		truncname.c_str(),
		ptr
                );

        fflush(gLogFanOut["#"]);
        buffer[15] = ' ';
      }
    }
    fprintf(stderr, "%s\n", buffer);
    fflush(stderr);
  }
  else
  {
    fprintf(stderr, "%s\n", buffer);
    fflush(stderr);
  }
  va_end(args);

  const char* rptr;
  // store into global log memory
  gLogMemory[priority][(gLogCircularIndex[priority]) % gCircularIndexSize] = buffer;
  rptr = gLogMemory[priority][(gLogCircularIndex[priority]) % gCircularIndexSize].c_str();
  gLogCircularIndex[priority]++;
  pthread_mutex_unlock(&gMutex);
  return rptr;
}

/*----------------------------------------------------------------------------*/
/** 
 * Initialize the circular index and logging object
 * 
 */

/*----------------------------------------------------------------------------*/
void
Logging::Init ()
{
  // initialize the log array and sets the log circular size
  gLogCircularIndex.resize(LOG_DEBUG + 1);
  gLogMemory.resize(LOG_DEBUG + 1);
  gCircularIndexSize = DIAMONDCOMMONLOGGING_CIRCULARINDEXSIZE;
  for (int i = 0; i <= LOG_DEBUG; i++)
  {
    gLogCircularIndex[i] = 0;
    gLogMemory[i].resize(gCircularIndexSize);
  }
}

/*----------------------------------------------------------------------------*/
DIAMONDCOMMONNAMESPACE_END

