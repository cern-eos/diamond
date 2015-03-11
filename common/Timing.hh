// ----------------------------------------------------------------------
// File: Timing.hh
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
 * @file   Timing.hh
 *
 * @brief  Class providing real-time code measurements.
 *
 *
 */


#ifndef __DIAMONDCOMMON__TIMING__HH
#define __DIAMONDCOMMON__TIMING__HH

#include "common/Namespace.hh"
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string>

DIAMONDCOMMONNAMESPACE_BEGIN

/*----------------------------------------------------------------------------*/
//! Class implementing comfortable time measurements through methods/functions
//!
//! Example
//! diamond::common::Timing tm("Test");
//! COMMONTIMING("START",&tm);
//! ...
//! COMMONTIMING("CHECKPOINT1",&tm);
//! ...
//! COMMONTIMING("CHECKPOINT2",&tm);
//! ...
//! COMMONTIMING("STOP", &tm);
//! tm.Print();
//! fprintf(stdout,"realtime = %.02f", tm.RealTime());
/*----------------------------------------------------------------------------*/
class Timing {
public:
  struct timeval tv;
  std::string tag;
  std::string maintag;
  Timing* next;
  Timing* ptr;

  // ---------------------------------------------------------------------------
  //! Constructor - used only internally
  // ---------------------------------------------------------------------------

  Timing (const char* name, struct timeval &i_tv) {
    memcpy(&tv, &i_tv, sizeof (struct timeval));
    tag = name;
    next = 0;
    ptr = this;
  }
  // ---------------------------------------------------------------------------
  //! Constructor - tag is used as the name for the measurement in Print
  // ---------------------------------------------------------------------------

  Timing (const char* i_maintag) {
    tag = "BEGIN";
    next = 0;
    ptr = this;
    maintag = i_maintag;
  }


  // ---------------------------------------------------------------------------
  //! Get time elapsed between the two tags in miliseconds
  // ---------------------------------------------------------------------------

  float
  GetTagTimelapse (const std::string& tagBegin, const std::string& tagEnd) {
    float time_elapsed = 0;
    Timing* ptr = this->next;
    Timing* ptrBegin = 0;
    Timing* ptrEnd = 0;

    while (ptr) {
      if (tagBegin.compare(ptr->tag.c_str()) == 0) {
        ptrBegin = ptr;
      }

      if (tagEnd.compare(ptr->tag.c_str()) == 0) {
        ptrEnd = ptr;
      }

      if (ptrBegin && ptrEnd) break;

      ptr = ptr->next;
    }

    if (ptrBegin && ptrEnd) {
      time_elapsed = static_cast<float> (((ptrEnd->tv.tv_sec - ptrBegin->tv.tv_sec) *1000000 +
                                          (ptrEnd->tv.tv_usec - ptrBegin->tv.tv_usec)) / 1000.0);
    }

    return time_elapsed;
  }


  // ---------------------------------------------------------------------------
  //! Print method to display measurements on STDERR
  // ---------------------------------------------------------------------------

  void
  Print () {
    char msg[512];
    Timing* p = this->next;
    Timing* n;
    std::cerr << std::endl;
    while ((n = p->next)) {

      sprintf(msg, "                                        [ %12s ] %20s <=> %-20s : %.03f\n", maintag.c_str(), p->tag.c_str(), n->tag.c_str(), (float) ((n->tv.tv_sec - p->tv.tv_sec) *1000000 + (n->tv.tv_usec - p->tv.tv_usec)) / 1000.0);
      std::cerr << msg;
      p = n;
    }
    n = p;
    p = this->next;
    sprintf(msg, "                                        = %12s = %20s <=> %-20s : %.03f\n", maintag.c_str(), p->tag.c_str(), n->tag.c_str(), (float) ((n->tv.tv_sec - p->tv.tv_sec) *1000000 + (n->tv.tv_usec - p->tv.tv_usec)) / 1000.0);
    std::cerr << msg;
  }

  // ---------------------------------------------------------------------------
  //! Return total Realtime
  // ---------------------------------------------------------------------------

  double
  RealTime () {
    Timing* p = this->next;
    Timing* n;
    while ((n = p->next)) {
      p = n;
    }
    n = p;
    p = this->next;
    return (double) ((n->tv.tv_sec - p->tv.tv_sec) *1000000 + (n->tv.tv_usec - p->tv.tv_usec)) / 1000.0;
  }

  // ---------------------------------------------------------------------------
  //! Destructor
  // ---------------------------------------------------------------------------

  virtual
  ~Timing () {
    Timing* n = next;
    if (n) delete n;
  };

  // ---------------------------------------------------------------------------
  //! Wrapper Function to hide difference between Apple and Linux
  // ---------------------------------------------------------------------------

  static void
  GetTimeSpec (struct timespec &ts) {
#ifdef __APPLE__
    struct timeval tv;
    gettimeofday(&tv, 0);
    ts.tv_sec = tv.tv_sec;
    ts.tv_nsec = tv.tv_usec * 1000;
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
  }

  // ---------------------------------------------------------------------------
  //! Time Conversion Function for ISO8601 time strings
  // ---------------------------------------------------------------------------

  static std::string
  UnixTimstamp_to_ISO8601 (time_t now) {
    struct tm *utctime;
    char str[21];
    utctime = gmtime(&now);
    strftime(str, 21, "%Y-%m-%dT%H:%M:%SZ", utctime);
    return str;
  }

  // ---------------------------------------------------------------------------
  //! Time Conversion Function for strings to ISO8601 time
  // ---------------------------------------------------------------------------

  static time_t
  ISO8601_to_UnixTimestamp (std::string iso) {
    tzset();
    char temp[64];
    memset(temp, 0, sizeof (temp));
    strncpy(temp, iso.c_str(), (iso.length() < 64) ? iso.length() : 64);

    struct tm ctime;
    memset(&ctime, 0, sizeof (struct tm));
    strptime(temp, "%FT%T%z", &ctime);

    time_t ts = mktime(&ctime) - timezone;
    return ts;
  }

  static
  std::string
  utctime (time_t ttime) {
    struct tm utc;
    gmtime_r(&ttime, &utc);
    static const char wday_name[][4] = {
      "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char mon_name[][4] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static char result[40];
    sprintf(result, "%.3s, %02d %.3s %d %.2d:%.2d:%.2d GMT",
            wday_name[utc.tm_wday],
            utc.tm_mday,
            mon_name[utc.tm_mon],
            1900 + utc.tm_year,
            utc.tm_hour,
            utc.tm_min,
            utc.tm_sec);
    return std::string(result);
  }
};

// ---------------------------------------------------------------------------
//! Macro to place a measurement throughout the code
// ---------------------------------------------------------------------------
#define COMMONTIMING( __ID__,__LIST__)                                \
  do {								\
    struct timeval tp;						\
    struct timezone tz;						\
    gettimeofday(&tp, &tz);					\
    (__LIST__)->ptr->next=new diamond::common::Timing(__ID__,tp);	\
    (__LIST__)->ptr = (__LIST__)->ptr->next;			\
  } while(0);


DIAMONDCOMMONNAMESPACE_END

#endif
