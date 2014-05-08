// ----------------------------------------------------------------------
// File: Logging.hh
// Author: Andreas-Joachim Peters - CERN
// ----------------------------------------------------------------------

/************************************************************************
 * DIAMOND - the CERN Disk Storage System                               *
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
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.*
 ************************************************************************/

/**
 * @file   Logging.hh
 *
 * @brief  Class for message logging.
 *
 * You can use this class without creating an instance object (it provides a 
 * global singleton). All the 'diamond_<state>' functions require that the logging
 * class inherits from the 'LogId' class. As an alternative as set of static
 * 'diamond_static_<state>' logging functions are provided. To define the log level
 * one uses the static 'SetLogPriority' function. 'SetFilter' allows to filter
 * out log messages which are identified by their function/method name
 * (__FUNCTION__). If you prefix this comma seperated list with 'PASS:' it is 
 * used as an acceptance filter. By default all logging is printed to 'stderr'.
 * You can arrange a log stream filter fan-out using 'AddFanOut'. The fan-out of
 * messages is defined by the source filename the message comes from and mappes
 * to a FILE* where the message is written. If you add a '*' fan-out you can
 * write all messages into this file. If you add a '#' fan-out you can write 
 * all messages which are not in any other fan-out (besides '*') into that file.
 * The fan-out functionality assumes that
 * source filenames follow the pattern <fan-out-name>.xx !!!!
 */

#ifndef __DIAMONDCOMMON_LOGGING_HH__
#define __DIAMONDCOMMON_LOGGING_HH__

/*----------------------------------------------------------------------------*/
#include "common/Namespace.hh"
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
#include <string.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <uuid/uuid.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <pthread.h>
/*----------------------------------------------------------------------------*/

DIAMONDCOMMONNAMESPACE_BEGIN
#define DIAMOND_TEXTNORMAL "\033[0m"
#define DIAMOND_TEXTBLACK  "\033[49;30m"
#define DIAMOND_TEXTRED    "\033[49;31m"
#define DIAMOND_TEXTREDERROR "\033[47;31m\e[5m"
#define DIAMOND_TEXTBLUEERROR "\033[47;34m\e[5m"
#define DIAMOND_TEXTGREEN  "\033[49;32m"
#define DIAMOND_TEXTYELLOW "\033[49;33m"
#define DIAMOND_TEXTBLUE   "\033[49;34m"
#define DIAMOND_TEXTBOLD   "\033[1m"
#define DIAMOND_TEXTUNBOLD "\033[0m"


/*----------------------------------------------------------------------------*/
//! Log Macros usable in objects inheriting from the logId Class
/*----------------------------------------------------------------------------*/
#define diamond_log(__DIAMONDCOMMON_LOG_PRIORITY__ , ...) diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity,  LOG_MASK(__DIAMONDCOMMON_LOG_PRIORITY__) , __VA_ARGS__
#define diamond_debug(...)   diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity, (LOG_DEBUG)  , __VA_ARGS__)
#define diamond_info(...)    diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity, (LOG_INFO)   , __VA_ARGS__)
#define diamond_notice(...)  diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity, (LOG_NOTICE) , __VA_ARGS__)
#define diamond_warning(...) diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity, (LOG_WARNING), __VA_ARGS__)
#define diamond_err(...)     diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity, (LOG_ERR)    , __VA_ARGS__)
#define diamond_crit(...)    diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity, (LOG_CRIT)   , __VA_ARGS__)
#define diamond_alert(...)   diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity, (LOG_ALERT)  , __VA_ARGS__)
#define diamond_emerg(...)   diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, logId, logIdentity, (LOG_EMERG)  , __VA_ARGS__)

#define diamond_log_id(x,y)   char logId[1024];char logIdentity[1024];snprintf(logId,sizeof(logId)-1,"%s", (x) );snprintf(logIdentity,sizeof(logIdentity)-1,"%s", (y) );
#define diamond_static_debug(...)   diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, "","", (LOG_DEBUG)  , __VA_ARGS__)
#define diamond_static_info(...)    diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, "","", (LOG_INFO)   , __VA_ARGS__)
#define diamond_static_notice(...)  diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, "","", (LOG_NOTICE) , __VA_ARGS__)
#define diamond_static_warning(...) diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, "","", (LOG_WARNING), __VA_ARGS__)
#define diamond_static_err(...)     diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, "","", (LOG_ERR)    , __VA_ARGS__)
#define diamond_static_crit(...)    diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, "","", (LOG_CRIT)   , __VA_ARGS__)
#define diamond_static_alert(...)   diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, "","", (LOG_ALERT)  , __VA_ARGS__)
#define diamond_static_emerg(...)   diamond::common::Logging::log(__FUNCTION__,__FILE__, __LINE__, "","", (LOG_EMERG)  , __VA_ARGS__)

/*----------------------------------------------------------------------------*/
//! Log Macros to check if a function would log in a certain log level
/*----------------------------------------------------------------------------*/
#define DIAMOND_LOGS_DEBUG   diamond::common::Logging::shouldlog(__FUNCTION__,(LOG_DEBUG)  )
#define DIAMOND_LOGS_INFO    diamond::common::Logging::shouldlog(__FUNCTION__,(LOG_INFO)   )
#define DIAMOND_LOGS_NOTICE  diamond::common::Logging::shouldlog(__FUNCTION__,(LOG_NOTICE) )
#define DIAMOND_LOGS_WARNING diamond::common::Logging::shouldlog(__FUNCTION__,(LOG_WARNING))
#define DIAMOND_LOGS_ERR     diamond::common::Logging::shouldlog(__FUNCTION__,(LOG_ERR)    )
#define DIAMOND_LOGS_CRIT    diamond::common::Logging::shouldlog(__FUNCTION__,(LOG_CRIT)   )
#define DIAMOND_LOGS_ALERT   diamond::common::Logging::shouldlog(__FUNCTION__,(LOG_ALERT)  )
#define DIAMOND_LOGS_EMERG   diamond::common::Logging::shouldlog(__FUNCTION__,(LOG_EMERG)  )


#define DIAMONDCOMMONLOGGING_CIRCULARINDEXSIZE 10000

/*----------------------------------------------------------------------------*/
//! Class implementing DIAMOND logging
/*----------------------------------------------------------------------------*/
class LogId
{
public:
  // ---------------------------------------------------------------------------
  //! Set's the logid and trace identifier
  // ---------------------------------------------------------------------------

  void
  SetLogId (const char* newlogid, const char* td = "<service>")
  {
    if (newlogid != logId)
      snprintf(logId, sizeof (logId) - 1, "%s", newlogid);
    snprintf(logIdentity, sizeof (logIdentity) - 1, "%s", td);
  }

  // ---------------------------------------------------------------------------
  //! Constructor
  // ---------------------------------------------------------------------------

  LogId ()
  {
    uuid_t uuid;
    uuid_generate_time(uuid);
    uuid_unparse(uuid, logId);
    sprintf(logIdentity, "<service>");
  }

  // ---------------------------------------------------------------------------
  //! Destructor
  // ---------------------------------------------------------------------------

  ~LogId () { }

  char logId[1024];
  char logIdentity[1024];
};

// ---------------------------------------------------------------------------
//! Class wrapping global singleton objects for logging
// ---------------------------------------------------------------------------

class Logging
{
private:
public:
  typedef std::vector< unsigned long > LogCircularIndex; //< typedef for circular index pointing to the next message position int he log array
  typedef std::vector< std::vector <std::string> > LogArray; //< typdef for log message array 

  static LogCircularIndex gLogCircularIndex; //< global circular index
  static LogArray gLogMemory; //< global logging memory
  static unsigned long gCircularIndexSize; //< global circular index size
  static int gLogMask; //< log mask
  static int gPriorityLevel; //< log priority
  static pthread_mutex_t gMutex; //< global mutex
  static std::string gUnit; //< global unit name
  static std::set<std::string> gAllowFilter; ///< global list of function names allowed to log
  static std::set<std::string> gDenyFilter; ///< global list of function names denied to log
  static int gShortFormat; //< indiciating if the log-output is in short format
  static std::map<std::string, FILE*> gLogFanOut; //< here one can define log fan-out to different file descriptors than stderr

  // ---------------------------------------------------------------------------
  //! Set the log priority (like syslog)
  // ---------------------------------------------------------------------------

  static void
  SetLogPriority (int pri)
  {
    gLogMask = LOG_UPTO(pri);
    gPriorityLevel = pri;
  }

  // ---------------------------------------------------------------------------
  //! Set the log unit name
  // ---------------------------------------------------------------------------

  static void
  SetUnit (const char* unit)
  {
    gUnit = unit;
  }

  // ---------------------------------------------------------------------------
  //! Set the log filter
  // ---------------------------------------------------------------------------

  void
  SetFilter (std::set<std::string> allow, std::set<std::string> deny)
  {
    // Clear both maps 
    gDenyFilter.clear();
    gAllowFilter.clear();

    gDenyFilter = deny;
    gAllowFilter = allow;
  }

  // ---------------------------------------------------------------------------
  //! Return priority as string
  // ---------------------------------------------------------------------------

  static const char*
  GetPriorityString (int pri)
  {
    if (pri == (LOG_INFO)) return "INFO ";
    if (pri == (LOG_DEBUG)) return "DEBUG";
    if (pri == (LOG_ERR)) return "ERROR";
    if (pri == (LOG_EMERG)) return "EMERG";
    if (pri == (LOG_ALERT)) return "ALERT";
    if (pri == (LOG_CRIT)) return "CRIT ";
    if (pri == (LOG_WARNING)) return "WARN ";
    if (pri == (LOG_NOTICE)) return "NOTE ";

    return "NONE ";
  }

  // ---------------------------------------------------------------------------
  //! Return priority int from string
  // ---------------------------------------------------------------------------

  static int
  GetPriorityByString (const char* pri)
  {
    if (!strcmp(pri, "info")) return LOG_INFO;
    if (!strcmp(pri, "debug")) return LOG_DEBUG;
    if (!strcmp(pri, "err")) return LOG_ERR;
    if (!strcmp(pri, "emerg")) return LOG_EMERG;
    if (!strcmp(pri, "alert")) return LOG_ALERT;
    if (!strcmp(pri, "crit")) return LOG_CRIT;
    if (!strcmp(pri, "warning")) return LOG_WARNING;
    if (!strcmp(pri, "notice")) return LOG_NOTICE;
    return -1;
  }

  // ---------------------------------------------------------------------------
  //! Initialize Logger
  // ---------------------------------------------------------------------------
  static void Init ();

  // ---------------------------------------------------------------------------
  //! Add a tag fanout filedescriptor to the logging module
  // ---------------------------------------------------------------------------

  static void
  AddFanOut (const char* tag, FILE* fd)
  {
    gLogFanOut[tag] = fd;
  }

  // ---------------------------------------------------------------------------
  //! Add a tag fanout alias to the logging module
  // ---------------------------------------------------------------------------

  static void
  AddFanOutAlias (const char* alias, const char* tag)
  {
    if (gLogFanOut.count(tag))
    {
      gLogFanOut[alias] = gLogFanOut[tag];
    }
  }

  // ---------------------------------------------------------------------------
  //! Get a color for a given logging level
  // ---------------------------------------------------------------------------

  static const char*
  GetLogColour (const char* loglevel)
  {
    if (!strcmp(loglevel, "INFO ")) return DIAMOND_TEXTGREEN;
    if (!strcmp(loglevel, "ERROR")) return DIAMOND_TEXTRED;
    if (!strcmp(loglevel, "WARN ")) return DIAMOND_TEXTYELLOW;
    if (!strcmp(loglevel, "NOTE ")) return DIAMOND_TEXTBLUE;
    if (!strcmp(loglevel, "CRIT ")) return DIAMOND_TEXTREDERROR;
    if (!strcmp(loglevel, "EMERG")) return DIAMOND_TEXTBLUEERROR;
    if (!strcmp(loglevel, "ALERT")) return DIAMOND_TEXTREDERROR;
    if (!strcmp(loglevel, "DEBUG")) return "";
    return "";
  }

  // ---------------------------------------------------------------------------
  //! Check if we should log in the defined level/filter
  // ---------------------------------------------------------------------------
  static bool shouldlog (const char* func, int priority);

  // ---------------------------------------------------------------------------
  //! Log a message into the global buffer
  // ---------------------------------------------------------------------------
  static const char* log (const char* func, const char* file, int line, const char* logid, const char* midentity, int priority, const char *msg, ...);

};

/*----------------------------------------------------------------------------*/
DIAMONDCOMMONNAMESPACE_END

#endif
