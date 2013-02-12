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

#include <gtest/gtest.h>
#include "dxLog.h"
#include "unixDGRAM.h"
#include "omp.h"

using namespace std;
using namespace DXLog;

const string socketPath = "test_unix_datagram_log";
const string testMsg = "近期活動 €þıœəßð some utf-8 ĸʒ×ŋµåäö𝄞\nNew Line";

class TestDGRAM : public UnixDGRAMReader{
  private:
    bool processMsg(){
      msgs.push_back(string(buffer));
      if (string(buffer).compare("Done") == 0) return true;
      return false;
    };

  public:
    vector<string> msgs;
    TestDGRAM() : UnixDGRAMReader(1000) { msgs.clear(); }
};

TEST(UNIXDGRAMTest, Invalid_Socket) {
  unlink(socketPath.c_str());
  string errMsg;
  ASSERT_FALSE(SendMessage2UnixDGRAMSocket(socketPath, "msg", errMsg));
  ASSERT_EQ(errMsg, "Error when sending log message: No such file or directory"); 
}

TEST(UNIXDGRAMTest, Integration) {
  unlink(socketPath.c_str());
  TestDGRAM test;
  string errMsg, errMsg2;

  omp_set_num_threads(2);
  bool ret_val[5];
  #pragma omp parallel sections
  {
    ret_val[0] = test.run(socketPath, errMsg);

    #pragma omp section
    {
      while (! test.isActive()) { usleep(100); }
      ret_val[1] = SendMessage2UnixDGRAMSocket(socketPath, "msg1", errMsg2);
      ret_val[2] = SendMessage2UnixDGRAMSocket(socketPath, "msg2", errMsg2);
      ret_val[3] = SendMessage2UnixDGRAMSocket(socketPath, "msg3", errMsg2);
      ret_val[4] = SendMessage2UnixDGRAMSocket(socketPath, "Done", errMsg2);
    }
  }
  ASSERT_FALSE(test.isActive());
  ASSERT_EQ(test.msgs.size(), 4);
  ASSERT_EQ(test.msgs[0], "msg1");
  ASSERT_EQ(test.msgs[1], "msg2");
  ASSERT_EQ(test.msgs[2], "msg3");
  ASSERT_EQ(test.msgs[3], "Done");

  for (int i = 0; i < 5; i ++)
    ASSERT_TRUE(ret_val[i]);

  ASSERT_FALSE(SendMessage2UnixDGRAMSocket(socketPath, "msg", errMsg));
  ASSERT_EQ(errMsg, "Error when sending log message: No such file or directory");

  unlink(socketPath.c_str());
}

TEST(UNIXDGRAMTest, Address_Used) {
  unlink(socketPath.c_str());
  TestDGRAM test1, test2;
  string errMsg, errMsg2, errMsg3;

  omp_set_num_threads(2);
  bool ret_val[3];
  #pragma omp parallel sections
  {
    ret_val[0] = test1.run(socketPath, errMsg);

    #pragma omp section
    {
      while (! test1.isActive()) { usleep(100); }
      ret_val[1] = test2.run(socketPath, errMsg2);
      ret_val[2] = SendMessage2UnixDGRAMSocket(socketPath, "Done", errMsg2);
    }
  }
  
  ASSERT_FALSE(test1.isActive());
  ASSERT_FALSE(test2.isActive());
  ASSERT_TRUE(ret_val[0]);
  ASSERT_FALSE(ret_val[1]);
  ASSERT_TRUE(ret_val[2]);

  ASSERT_EQ(errMsg2, "Socket error: Address already in use");
  ASSERT_EQ(test1.msgs.size(), 1);
  ASSERT_EQ(test1.msgs[0], "Done");
  ASSERT_EQ(test2.msgs.size(), 0);
  unlink(socketPath.c_str());
}

class InvalidLogInput {
  public:
    InvalidLogInput() {};

    dx::JSON GetOne(int k, string &errMsg) {
      dx::JSON data(dx::JSON_OBJECT);
      data["source"] = "DX_APP";

      switch(k) {
        case 0: 
          data = dx::JSON(dx::JSON_ARRAY);
          errMsg = "Log input, " + data.toString() + ", is not a JSON object";
          return data;
        case 1:
          data["timestamp"] = "2012-1-1";
          errMsg = "Log timestamp, " + data["timestamp"].toString() + ", is not an integer";
          return data;
        case 2:
          data.erase("source");
          errMsg = "Missing log source";
          return data;
        case 3:
          data["source"] = dx::JSON(dx::JSON_OBJECT);
          errMsg = "Log source, " + data["source"].toString() + ", is not a string";
          return data;
        case 4:
          data["source"] = "app";
          errMsg = "Invalid log source: app";
          return data;
        case 5:
          data["level"] = "x";
          errMsg = "Log level, " + data["level"].toString() + ", is not an integer";
          return data;
        case 6:
          data["level"] = 12;
          errMsg = "Invalid log level: 12";
          return data;
        case 7:
          data["hostname"] = 12;
          errMsg = "Log hostname, 12, is not a string";
          return data;
        default:
          return dx::JSON_NULL;
      }
    }

    int NumInput() { return 8; }
};

TEST(DXLOGTest, Invalid_Log_Input) {
  string errMsg1, errMsg2;
  InvalidLogInput input;

  for (int i = 0; i < input.NumInput(); i++) {
    dx::JSON data = input.GetOne(i, errMsg1);
    ASSERT_FALSE(ValidateLogData(data, errMsg2));
    ASSERT_EQ(errMsg1, errMsg2);
  }
};

class ValidLogInput {
  public:
    ValidLogInput() {};

    dx::JSON GetOne(int k, string &head) {
      dx::JSON data(dx::JSON_OBJECT);
      string errMsg;

      switch(k) {
        case 0:
          data["source"] = "DX_APP";
          head = "<14>DX_APP ";
          return data;
        case 1:
          data["level"] = 1;
          data["source"] = "DX_CM";
          head = "<9>DX_CM " ;
          return data;
        case 2:
          data["level"] = 4;
          data["source"] = "DX_EM";
          data["hostname"] = "localhost";
          data["timestamp"] = utcMS();
          head = "<12>DX_EM " ;
          return data;
        default:
          return dx::JSON_NULL;
      }
    }

    int NumInput() { return 3; }
};

TEST(DXLOGTest, Log_Input_Default_Value) {
  string errMsg;
  
  dx::JSON data(dx::JSON_OBJECT);
  data["source"] = "DX_H";

  ASSERT_TRUE(ValidateLogData(data, errMsg));
  
  ASSERT_EQ(errMsg, "");
  ASSERT_EQ(data["level"].get<int>(), 6);
  ASSERT_EQ(data["hostname"].type(), dx::JSON_STRING);
  ASSERT_EQ(data["timestamp"].type(), dx::JSON_INTEGER);
}

TEST(DXLOGTest, Rsyslog_Byte_Seq) {
  unlink(socketPath.c_str());
  TestDGRAM test;
  string errMsg, errMsg2;
  ValidLogInput input;
  int n = input.NumInput();

  omp_set_num_threads(2);
  vector<string> msgs1, msgs2;
  #pragma omp parallel sections
  {
    test.run(socketPath, errMsg);

    #pragma omp section
    {
      while (! test.isActive()) { usleep(100); }

      string msg, errMsg;
      for (int i = 0; i < n; i++) {
        dx::JSON data = input.GetOne(i, msg);
        msgs1.push_back(msg + data.toString());

        if (! ValidateLogData(data, errMsg)) throwString(errMsg);

        msgs2.push_back(msg + data.toString());
        SendMessage2Rsyslog(int(data["level"]), data["source"].get<string>(), data.toString(), errMsg2, socketPath);
      }
      SendMessage2UnixDGRAMSocket(socketPath, "Done", errMsg2);
    }
  }
  ASSERT_FALSE(test.isActive());
  ASSERT_EQ(test.msgs.size(), n+1);
  for (int i = 0; i < n-1; i++)
    ASSERT_NE(test.msgs[i], msgs1[i]);
  ASSERT_EQ(test.msgs[n-1], msgs1[n-1]);

  for (int i = 0; i < n; i++)
    ASSERT_EQ(test.msgs[i], msgs2[i]);
  ASSERT_EQ(test.msgs[n], "Done");

  unlink(socketPath.c_str());
}

bool verify_mongodb_schema(const dx::JSON &schema, string &errMsg) {
  try{
    ValidateDBSchema(schema);
    return true;
  } catch (const string &msg) {
    errMsg = msg;
    return false;
  }
}

TEST(DXLogTest, MongoDB_Schema) {
  string errMsg;
  
  dx::JSON schema(dx::JSON_ARRAY);
  ASSERT_FALSE(verify_mongodb_schema(schema, errMsg));
  ASSERT_EQ(errMsg, "Mongodb schema, " + schema.toString() + ", is not a JSON object");

  schema = dx::JSON(dx::JSON_OBJECT);
  schema["DX_H"] = dx::JSON(dx::JSON_ARRAY);
  ASSERT_FALSE(verify_mongodb_schema(schema, errMsg));
  ASSERT_EQ(errMsg, "DX_H mongodb schema, " + schema["DX_H"].toString() + ", is not a JSON object");

  schema.erase("DX_H");
  schema["DX_H"] = dx::JSON(dx::JSON_OBJECT);
  ASSERT_FALSE(verify_mongodb_schema(schema, errMsg));
  ASSERT_EQ(errMsg, "DX_H: missing collection");

  schema["DX_H"]["collection"] = "h";
  ASSERT_TRUE(verify_mongodb_schema(schema, errMsg));
}

TEST(DXLOGTest, logger) {
  logger log;

  string errMsg1, errMsg2;
  InvalidLogInput input;
  
  for (int i = 0; i < input.NumInput(); i++) {
    dx::JSON data = input.GetOne(i, errMsg1);
    ASSERT_FALSE(log.Log(data, errMsg2));
    ASSERT_EQ(errMsg1, errMsg2);
  }

  unlink(socketPath.c_str());
  TestDGRAM test;
  vector<string> msgs;

  ValidLogInput input2;
  int n = input2.NumInput();

  omp_set_num_threads(2);
  #pragma omp parallel sections
  {
    test.run(socketPath, errMsg1);

    #pragma omp section
    {
      while (!test.isActive()) { usleep(100); }

      string msg, errMsg;
      for (int i = 0; i < n; i++) {
        dx::JSON data = input2.GetOne(i, msg);
        msgs.push_back(msg + data.toString());
        if (! log.Log(data, errMsg1, socketPath)) throwString(errMsg1);
      }
      SendMessage2UnixDGRAMSocket(socketPath, "Done", errMsg2);
    }
  }
  ASSERT_FALSE(test.isActive());
  ASSERT_EQ(test.msgs.size(), n+1);
  for (int i = 0; i < n-1; i++)
    ASSERT_NE(test.msgs[i], msgs[i]);
  ASSERT_EQ(test.msgs[n-1], msgs[n-1]);
  ASSERT_EQ(test.msgs[n], "Done");
  
  unlink(socketPath.c_str());
}

TEST(AppLogTest, AppLog_Config) {
  string errMsg;

  dx::JSON conf(dx::JSON_ARRAY);
  ASSERT_FALSE(AppLog::initEnv(conf, errMsg));
  ASSERT_EQ(errMsg, "App log config, " + conf.toString() + ", is not a JSON object");

  conf = dx::JSON(dx::JSON_OBJECT);
  ASSERT_FALSE(AppLog::initEnv(conf, errMsg));
  ASSERT_EQ(errMsg, "Missing socketPath in App log config");

  conf["socketPath"] = dx::JSON(dx::JSON_OBJECT);
  ASSERT_FALSE(AppLog::initEnv(conf, errMsg));
  ASSERT_EQ(errMsg, "socketPath, " + conf["socketPath"].toString() + ", is not a JSON array of strings");

  conf["socketPath"] = dx::JSON(dx::JSON_ARRAY);
  ASSERT_FALSE(AppLog::initEnv(conf, errMsg));
  ASSERT_EQ(errMsg, "Size of socketPath is smaller than 2");

  conf["socketPath"].push_back("log1");
  conf["socketPath"].push_back(1);
  ASSERT_FALSE(AppLog::initEnv(conf, errMsg));
  ASSERT_EQ(errMsg, "socketPath, " + conf["socketPath"].toString() + ", is not a JSON array of strings");

  conf["socketPath"][1] = "log1";
  ASSERT_TRUE(AppLog::initEnv(conf, errMsg));

  conf["socketPath"].push_back(1);
  ASSERT_TRUE(AppLog::initEnv(conf, errMsg));
}

bool setSocketPath() {
  string errMsg;
  dx::JSON conf(dx::JSON_OBJECT);
  conf["socketPath"] = dx::JSON(dx::JSON_ARRAY);
  conf["socketPath"].push_back(socketPath + "1");
  conf["socketPath"].push_back(socketPath + "2");

  return AppLog::initEnv(conf, errMsg);
}

void verifyAppLogData(const dx::JSON &data, const string &msg, int level, int64_t t1, int64_t t2) {
  ASSERT_EQ(int(data["level"]), level);
  ASSERT_EQ(data["msg"].get<string>(), msg);
  ASSERT_EQ(data["timestamp"].type(), dx::JSON_INTEGER);
  int64_t t = int64_t(data["timestamp"]);
  ASSERT_FALSE(t < t1);
  ASSERT_TRUE(t <= t2);
}

void writeLog(const string &msg, const vector<bool> &desired) {
  for (int i = 0; i < 10; i++)
    ASSERT_EQ(desired[i], AppLog::log(msg, i-1));
}

TEST(AppLogTest, SOCKET_NOT_EXIST) {
  ASSERT_TRUE(setSocketPath());
  for (int i = 0; i < 10; i++)
    ASSERT_FALSE(AppLog::log(testMsg, i-1));
}

TEST(AppLogTest, High_Priority_Socket_Only) {
  unlink((socketPath + "1").c_str());
  ASSERT_TRUE(setSocketPath());
  TestDGRAM test;
  string errMsg, errMsg2;
  int64_t t1 = utcMS();

  vector<bool> desired;
  for (int i = 0; i < 10; i ++)
    desired.push_back(false);
  desired[1] = desired[2] = desired[3] = true;

  omp_set_num_threads(2);
  #pragma omp parallel sections
  {
    test.run(socketPath + "1", errMsg);
    
    #pragma omp section
    {
      while (!test.isActive()) { usleep(100); }
     
      writeLog(testMsg, desired);
      SendMessage2UnixDGRAMSocket(socketPath + "1", "Done", errMsg2);
    }
  }
  
  ASSERT_FALSE(test.isActive());
  ASSERT_EQ(test.msgs.size(), 4);
  int64_t t2 = utcMS();
  for (int i = 0; i < 3; i++)
    verifyAppLogData(dx::JSON::parse(test.msgs[i]), testMsg, i, t1, t2);

  ASSERT_EQ(test.msgs[3], "Done");

  unlink((socketPath + "1").c_str());
}

TEST(AppLogTest, Low_Priority_Socket_Only) {
  unlink((socketPath + "2").c_str());
  ASSERT_TRUE(setSocketPath());
  TestDGRAM test;
  string errMsg, errMsg2;
  int64_t t1 = utcMS();

  vector<bool> desired;
  for (int i = 0; i < 10; i ++)
    desired.push_back(true);
  desired[0] = desired[1] = desired[2] = desired[3] = desired[9] = false;

  omp_set_num_threads(2);
  #pragma omp parallel sections
  {
    test.run(socketPath + "2", errMsg);
    
    #pragma omp section
    {
      while (!test.isActive()) { usleep(100); }
     
      writeLog(testMsg, desired);
      SendMessage2UnixDGRAMSocket(socketPath + "2", "Done", errMsg2);
    }
  }
  
  ASSERT_FALSE(test.isActive());
  ASSERT_EQ(test.msgs.size(), 6);
  int64_t t2 = utcMS();
  for (int i = 3; i < 8 ; i++)
    verifyAppLogData(dx::JSON::parse(test.msgs[i-3]), testMsg, i, t1, t2);

  ASSERT_EQ(test.msgs[5], "Done");

  unlink((socketPath + "2").c_str());
}

TEST(AppLogTest, Write_Log) {
  unlink((socketPath + "1").c_str());
  unlink((socketPath + "2").c_str());
  ASSERT_TRUE(setSocketPath());
  TestDGRAM test1, test2;
  string errMsg, errMsg2, errMsg3, errMsg4;

  vector<bool> desired;
  for (int i = 0; i < 10; i ++)
    desired.push_back(true);
  desired[0] = desired[9] = false;

  int64_t t1 = utcMS();
  omp_set_num_threads(3);
  #pragma omp parallel sections
  {
    test1.run(socketPath + "1", errMsg);
    
    #pragma omp section
    {
      test2.run(socketPath + "2", errMsg2);
    }
    
    #pragma omp section
    {
      while (!test1.isActive()) { usleep(100); }
      while (!test2.isActive()) { usleep(100); }
     
      writeLog(testMsg, desired);
      SendMessage2UnixDGRAMSocket(socketPath + "1", "Done", errMsg3);
      SendMessage2UnixDGRAMSocket(socketPath + "2", "Done", errMsg4);
    }
  }
  
  ASSERT_FALSE(test1.isActive());
  ASSERT_EQ(test1.msgs.size(), 4);
  int64_t t2 = utcMS();
  for (int i = 0; i < 3 ; i++)
    verifyAppLogData(dx::JSON::parse(test1.msgs[i]), testMsg, i, t1, t2);
  ASSERT_EQ(test1.msgs[3], "Done");

  ASSERT_FALSE(test2.isActive());
  ASSERT_EQ(test2.msgs.size(), 6);
  for (int i = 0; i < 5 ; i++)
    verifyAppLogData(dx::JSON::parse(test2.msgs[i]), testMsg, i+3, t1, t2);
  ASSERT_EQ(test2.msgs[5], "Done");

  unlink((socketPath + "2").c_str());
  unlink((socketPath + "1").c_str());
}

TEST(AppLogTest, AppLog_Done) {
  unlink((socketPath + "1").c_str());
  unlink((socketPath + "2").c_str());
  ASSERT_TRUE(setSocketPath());
  TestDGRAM test1, test2;
  string errMsg, errMsg2, errMsg3, errMsg4;

  ASSERT_FALSE(AppLog::done(errMsg));
  ASSERT_EQ(errMsg, socketPath + "1: Error when sending log message: No such file or directory, " + socketPath + "2: Error when sending log message: No such file or directory");

  omp_set_num_threads(3);
  bool ret_val;
  #pragma omp parallel sections
  {
    test1.run(socketPath + "1", errMsg);
    
    #pragma omp section
    {
      while (!test1.isActive()) { usleep(100); }
      ret_val = AppLog::done(errMsg2);
    }
  }
  ASSERT_FALSE(ret_val);
  ASSERT_EQ(errMsg2, socketPath + "2: Error when sending log message: No such file or directory");

  ASSERT_FALSE(test1.isActive());
  ASSERT_FALSE(AppLog::done(errMsg));
  ASSERT_EQ(errMsg, socketPath + "1: Error when sending log message: No such file or directory, " + socketPath + "2: Error when sending log message: No such file or directory");

  #pragma omp parallel sections
  {
    test2.run(socketPath + "2", errMsg);
    
    #pragma omp section
    {
      while (!test2.isActive()) { usleep(100); }
      ret_val = AppLog::done(errMsg2);
    }
  }
  ASSERT_FALSE(ret_val);
  ASSERT_EQ(errMsg2, socketPath + "1: Error when sending log message: No such file or directory");
  ASSERT_FALSE(test2.isActive());
  ASSERT_FALSE(AppLog::done(errMsg));
  ASSERT_EQ(errMsg, socketPath + "1: Error when sending log message: No such file or directory, " + socketPath + "2: Error when sending log message: No such file or directory");

  #pragma omp parallel sections
  {
    test1.run(socketPath + "1", errMsg);
    
    #pragma omp section
    {
      test2.run(socketPath + "2", errMsg);
    }
    
    #pragma omp section
    {
      while (!test1.isActive()) { usleep(100); }
      while (!test2.isActive()) { usleep(100); }

      ret_val = AppLog::done(errMsg2);
    }
  }
  ASSERT_TRUE(ret_val);
  ASSERT_FALSE(test1.isActive());
  ASSERT_FALSE(test2.isActive());
  ASSERT_FALSE(AppLog::done(errMsg));
  ASSERT_EQ(errMsg, socketPath + "1: Error when sending log message: No such file or directory, " + socketPath + "2: Error when sending log message: No such file or directory");
  unlink((socketPath + "1").c_str());
  unlink((socketPath + "2").c_str());
}

bool setSocketPath2() {
  string errMsg;
  dx::JSON conf(dx::JSON_OBJECT);
  conf["socketPath"] = dx::JSON(dx::JSON_ARRAY);
  conf["socketPath"].push_back(socketPath);
  conf["socketPath"].push_back(socketPath);

  return AppLog::initEnv(conf, errMsg);
}

TEST(AppLogTest, AppLog_Done_SingleSocket) {
  unlink((socketPath).c_str());
  ASSERT_TRUE(setSocketPath2());
  TestDGRAM test;
  string errMsg, errMsg2;

  ASSERT_FALSE(AppLog::done(errMsg));
  ASSERT_EQ(errMsg, socketPath + ": Error when sending log message: No such file or directory");

  omp_set_num_threads(2);
  bool ret_val;
  #pragma omp parallel sections
  {
    test.run(socketPath, errMsg);
    
    #pragma omp section
    {
      while (!test.isActive()) { usleep(100); }
      ret_val = AppLog::done(errMsg2);
    }
  }
  ASSERT_TRUE(ret_val);
  ASSERT_FALSE(test.isActive());
  ASSERT_FALSE(AppLog::done(errMsg));
  ASSERT_EQ(errMsg, socketPath + ": Error when sending log message: No such file or directory");
  unlink((socketPath).c_str());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}