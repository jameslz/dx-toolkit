// Copyright (C) 2013 DNAnexus, Inc.
//
// This file is part of dx-toolkit (DNAnexus platform client libraries).
//
//   Licensed under the Apache License, Version 2.0 (the "License"); you may
//   not use this file except in compliance with the License. You may obtain a
//   copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
//   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
//   License for the specific language governing permissions and limitations
//   under the License.

#ifndef DXLOG_HELPER_H
#define DXLOG_HELPER_H

#include "dxjson/dxjson.h"
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace std;

namespace DXLog {
  void throwString(const string &msg);

  string myPath();
  
  string getHostname();

  long long int utcMS();

  // throw a const string with detailed information if there is an error
  void ValidateLogLevel(const dx::JSON &level);
  void ValidateLogFacility(const dx::JSON &facility);
  void ValidateLogMsgSize(const dx::JSON &msgSize);
  void ValidateDBSchema(const dx::JSON &schema);
  bool ValidateLogData(dx::JSON &message, string &errMsg);

  /** Rsyslog head format: <pri>tag, where pri is a combination of facility and level
   *  Reture false if values of facility and level are not correct
   */
  bool formMessageHead(int facility, int level, const string &tag, string &head, string &errMsg);

  // generate a random string of size n
  string randomString(int n);

  /** Send message to rsyslog
   *  Return true if succeeded; otherwise errMsg contains the detaialed information of error
   */  
  bool SendMessage2Rsyslog(int level, const string &tag, const string &message, string &errMsg, const string &socketPath = "/dev/log");
};
#endif