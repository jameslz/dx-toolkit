#include <dxjson/dxjson.h>
#include "unixDGRAM.h"
#include "dxLog.h"
#include "dxLog_helper.h"
#include "mongoLog.h"
#include <boost/lexical_cast.hpp>
#include <deque>
#include <omp.h>
#include <unistd.h>

using namespace std;

namespace DXLog {
  class MongoDbLog : public UnixDGRAMReader {
    private:
      dx::JSON scheme;
      deque<string> que;
      int maxQueueSize;
      string socketPath, hostname;
      bool active;

      void getHostname() {
	 char buf[1001];
	 gethostname(buf, 1000);
        hostname = string(buf);
	 cout << hostname << endl;
      }

      bool ensureIndex(string &errMsg) {
        for (dx::JSON::object_iterator it = scheme.object_begin(); it != scheme.object_end(); ++it) {
	   string key = it->first;
	   dx::JSON index = it->second["mongodb"]["indexes"];
	   cout << index.toString() << endl;

	   for (int i = 0; i < index.size(); i++) {
	     BSONObjBuilder b;
	     for(dx::JSON::object_iterator it2 = index[i].object_begin(); it2 != index[i].object_end(); ++it2)
		b.append(it2->first, int(it2->second));

            if (! MongoDriver::ensureIndex(b.obj(), key, errMsg)) return false;
	   }
	 }
	 return true;
      }

      void selfLog(int level, const string &msg) {
	 string eMsg;
        #pragma omp critical
	 SendMessage2Rsyslog(8, level, "DNAnexusLog", msg, eMsg, msg.size() + 1);
      }

      bool sendMessage(dx::JSON &data, string &errMsg) {
	 if (! ValidateLogData(scheme, data, errMsg)) return false;
	 data["hostname"] = hostname;

	 dx::JSON columns = scheme[data["source"].get<string>()]["mongodb"]["columns"];
	 BSONObjBuilder b;

	 for(dx::JSON::object_iterator it = columns.object_begin(); it != columns.object_end(); ++it) {
	   string key = it->first, typ = it->second.get<string>();
	   if (! data.has(key)) continue;

	   if (typ.compare("string") == 0) {
	     b.append(key, data[key].get<string>());
	   } else if (typ.compare("int") == 0) {
	     b.append(key, int(data[key]));
	   } else if (typ.compare("int64") == 0) {
	     b.append(key, int64(data[key]));
	   } else if (typ.compare("boolean") == 0) {
	     b.append(key, bool(data[key]));
	   } else if (typ.compare("double") == 0) {
	     b.append(key, double(data[key]));
	   }
	 }
        
	 return MongoDriver::insert(b.obj(), data["source"].get<string>(), errMsg);
      };

      void processQueue() {
        string errMsg;
	 while (true) {
	   if (que.size() > 0) {
	     try {
		dx::JSON data = dx::JSON::parse(que.front());
		for (int i = 0; i < 10; i++) {
		  if (! sendMessage(data, errMsg)) {
  		    selfLog(3, errMsg + " Msg: " + que.front());
		    sleep(5);
		  } else break;
		}
	     } catch (std::exception &e) {
		selfLog(3, string(e.what()) + " Msg: " + que.front());
	     }
	
            #pragma omp critical
	     que.pop_front();
	   } else {
	     if (! active) return;
	     sleep(1);
	   }
        }
      };

      bool processMsg() {
	 if (que.size() < maxQueueSize) {
          #pragma omp critical
	   que.push_back(string(buffer));
	 } else {
	   selfLog(3, "Msg Queue Full, drop message " + string(buffer));
	 }

	 return false;
      };

    public:
      MongoDbLog(const dx::JSON &conf) : UnixDGRAMReader(1000 + int(conf["maxMsgSize"])) {
	 scheme = readJSON(conf["scheme"].get<string>());
	 socketPath = conf["socketPath"].get<string>();
	 maxQueueSize = (conf.has("maxQueueSize")) ? int(conf["maxQueueSize"]): 10000;

	 if (conf.has("mongoServer")) DXLog::MongoDriver::setServer(conf["mongoServer"].get<string>());
	 if (conf.has("database")) DXLog::MongoDriver::setDB(conf["database"].get<string>());
      };

      void process() {
	 que.clear();
	 
	 active = true;
	 string errMsg;

	 if (! ensureIndex(errMsg)) {
	   selfLog(3, errMsg);
	   return;
	 }

	 getHostname();

        #pragma omp parallel sections
	 {
	   string errMsg;
	   unlink(socketPath.c_str());
	   run(socketPath, errMsg);
          selfLog(3, errMsg);
	   active = false;

	   #pragma omp section
	   processQueue();
	 }
      };
  };
};

int main(int argc, char **argv) {
  if (argc < 2) {
    cerr << "Usage: dxDbLog configFile\n";
    exit(1);
  }

  try {
    dx::JSON conf = DXLog::readJSON(argv[1]);
    if (! conf.has("maxMsgSize")) conf["maxMsgSize"] = 2000;
    if (! conf.has("scheme")) throw ("log scheme is not specified");
    if (! conf.has("socketPath")) throw ("socketPath is not specified");

    DXLog::MongoDbLog a(conf);
    a.process();
   
  } catch (char *e) {
    cout << e << endl;
    exit(1);
  }
  exit(0);
}