#ifndef _BATONS_H
#define _BATONS_H

#include <v8.h>
#include <uv.h>
#include <zdb/zdb.h>
#include <string>
#include <vector>

#include "result.h"
#include "worker_functions.h"

using namespace v8;

namespace nodezdb {
extern ConnectionPool_T pool;

class ConnectionHolder {
protected:
  Connection_T connection;
public:
  ConnectionHolder(Connection_T _connection) : connection(_connection) {};
  ConnectionHolder() {};
  Connection_T getConnection() {
    return connection;
  };
  virtual void closeConnection() = 0;
  virtual ~ConnectionHolder() {};
};

class StandardConnectionHolder : public ConnectionHolder {
public:
  StandardConnectionHolder() {
    connection = ConnectionPool_getConnection(nodezdb::pool);
  };
  void closeConnection() {
    Connection_close(connection);
  }
};

class TransactionalConnectionHolder : public ConnectionHolder {
public:
  TransactionalConnectionHolder(Connection_T _connection) : ConnectionHolder(_connection) {};
  void closeConnection() {};
};

struct Baton {
  Baton(const char* _query) : query(_query), creationError(0) {};
  virtual ~Baton() {
    delete connectionHolder;
  };
  std::string query;
  uv_work_t request;
  ConnectionHolder* connectionHolder;
  virtual void queueWork() = 0;
  const char* creationError;
};

struct BatonWithResult : public Baton {
  BatonWithResult(Persistent<Function> _callback, const char* _query) : Baton(_query), callback(_callback), useResultSet(false) {};
  virtual ~BatonWithResult() {
    delete result;
    callback.Dispose();
  };
  Result* result;
  Persistent<Function> callback;
  bool useResultSet;
};

struct QueryBaton : public BatonWithResult {
  QueryBaton(Persistent<Function> _callback, const char* _query) :
    BatonWithResult(_callback, _query) {};
  void queueWork() {
    uv_queue_work(uv_default_loop(), &request, queryWork, afterQuery);  
  };
};

struct QueryBatonWithoutCallback : Baton {
  QueryBatonWithoutCallback(const char* _query) : Baton(_query) {};
  std::string errorText;
  void queueWork() {
    uv_queue_work(uv_default_loop(), &request, queryWithoutResult, afterQueryWithoutResult);
  };
};

struct PreparedStatementBaton : public BatonWithResult {
  PreparedStatementBaton(const char* _query, int _numValues, Persistent<Function> _callback) :
    BatonWithResult(_callback, _query), values(_numValues) {};
  ~PreparedStatementBaton() {
    for(unsigned int i = 0; i < values.size(); i++) {
      delete[] values[i];
    }
  }
  int _numValues;
  std::vector<char*> values;
  void queueWork() {
    uv_queue_work(uv_default_loop(), &request, preparedStatement, afterQuery);
  };
};
}
#endif