#include "worker_functions.h"

#include <zdb/zdb.h>
#include <v8.h>

#include "baton.h"

using namespace v8;

static Result* parseResult(Connection_T* connection, ResultSet_T* result) {
  Result* out;
  int rowsChanged = Connection_rowsChanged(*connection);
  int columnCount = ResultSet_getColumnCount(*result); 
  if(columnCount != 0) {//There are rows
    SelectResult* selectResult = new SelectResult();
    selectResult->fieldNames.resize( columnCount );
    
    for(int i = 1; i <= columnCount; i++) {
      const char* columnName = ResultSet_getColumnName(*result, i);  
      selectResult->fieldNames[i - 1] = columnName;
    } 

    unsigned int rowCounter = 0;
    while( ResultSet_next(*result) ) {
      Row* row = new Row();
      row->fieldValues.resize( columnCount );
      for(int i = 1; i <= columnCount; i++) {
        long size = ResultSet_getColumnSize(*result, i);
        char* temp = new char[ size ];
        memcpy(temp, ResultSet_getString(*result, i), size + 1);
        row->fieldValues[i - 1] = temp;
      }
      selectResult->rows.push_back(row);
      rowCounter++;
    }
    out = selectResult;
  } else {
      UpdateResult* updateResult = new UpdateResult();
      updateResult->affectedRows = rowsChanged;
      out = updateResult;
  }
  return out;
}

void queryWork(uv_work_t* req) {
  QueryBaton* baton = static_cast<QueryBaton*>(req->data);
  int error = 0;
  Connection_T connection = baton->connectionHolder->getConnection();
  ResultSet_T result;
  TRY
    result = Connection_executeQuery(connection, baton->query.c_str());
  CATCH(SQLException)
    baton->result = new EmptyResult(Connection_getLastError(connection));
    error = 1;
  END_TRY;
  
  if(!error) {
    if(!baton->useResultSet) {
      baton->result = parseResult(&connection, &result);
    } else {
      baton->result = new StreamingResult(result);
      //TODO: no! No one will close the connection!
      return;
    }
  }
  baton->connectionHolder->closeConnection();
}

void afterQuery(uv_work_t* req, int bla) {
  BatonWithResult* baton = static_cast<BatonWithResult*>(req->data);
  Handle<Value> result = baton->result->getResultObject();
  Handle<Value> error = String::New(baton->result->errorText.c_str());
  Handle<Value> argv[] = { result, error };
  baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
  delete baton;
}

void queryWithoutResult(uv_work_t* req) {
  QueryBatonWithoutCallback* baton = static_cast<QueryBatonWithoutCallback*>(req->data);
  int error = 0;
  Connection_T connection = baton->connectionHolder->getConnection();
  ResultSet_T result;
  TRY
    result = Connection_executeQuery(connection, baton->query.c_str());
  CATCH(SQLException)
    error = 1;
    baton->errorText = Connection_getLastError(connection);
  END_TRY;
  baton->connectionHolder->closeConnection();
}

void afterQueryWithoutResult(uv_work_t* req, int bla) {
  QueryBatonWithoutCallback* baton = static_cast<QueryBatonWithoutCallback*>(req->data);
  //todo: if error text is not empty throw an error
  delete baton;
}

void preparedStatement(uv_work_t* req) {
  PreparedStatementBaton* baton = static_cast<PreparedStatementBaton*>(req->data);
  Connection_T connection = baton->connectionHolder->getConnection();
  PreparedStatement_T pstmt;
  int error = 0;
  TRY
    pstmt = Connection_prepareStatement(connection, baton->query.c_str());
  CATCH(SQLException)
    baton->result = new EmptyResult(Connection_getLastError(connection));
    error = 1;
  END_TRY;

  if(!error) {
    for(unsigned int i = 0; i < baton->values.size() && !error; i++) {
      TRY
        PreparedStatement_setString(pstmt, i+1, baton->values[i]);
      CATCH(SQLException)
        error = 1;
        baton->result = new EmptyResult(Connection_getLastError(connection));
      END_TRY;
    }
  }
  ResultSet_T result;
  if(!error) {
    TRY
      result = PreparedStatement_executeQuery(pstmt);
    CATCH(SQLException)
      error = 1;
      baton->result = new EmptyResult(Connection_getLastError(connection));
    END_TRY;
  }

  if(!error) {
    Result* res = parseResult(&connection, &result);
    baton->result = res;
  }
  baton->connectionHolder->closeConnection();
}