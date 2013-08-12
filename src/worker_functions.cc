/**
The MIT License (MIT)

Copyright (c) 2013 Moritz Schulze

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/
#include "worker_functions.h"

#include <zdb/zdb.h>
#include <v8.h>

#include "baton.h"

#include <stdio.h>
#include <iostream>
#include <cstring>

using namespace v8;

namespace nodezdb {

static Result* parseResult(Connection_T* connection) {
  int rowsChanged = Connection_rowsChanged(*connection);
  UpdateResult* updateResult = new UpdateResult();
  updateResult->affectedRows = rowsChanged;
  return updateResult;
}

static Result* parseResult(Connection_T* connection, ResultSet_T* result) {
  int columnCount = ResultSet_getColumnCount(*result);

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
  return selectResult;
}

void queryWork(uv_work_t* req) {
  QueryBaton* baton = static_cast<QueryBaton*>(req->data);
  int error = 0;
  Connection_T connection = baton->connectionHolder->getConnection();
  ResultSet_T result;
  TRY
    if(baton->isSelect) {
      result = Connection_executeQuery(connection, baton->query.c_str());
    } else {
      Connection_execute(connection, baton->query.c_str());
    }
  CATCH(SQLException)
    baton->result = new EmptyResult(Connection_getLastError(connection));
    error = 1;
  END_TRY;

  if(!error) {
    if(!baton->useResultSet) {
      if(baton->isSelect) {
        baton->result = parseResult(&connection, &result);
      } else {
        baton->result = parseResult(&connection);
      }
    } else {
      baton->result = new StreamingResult(result, baton->connectionHolder->getConnection());
      return;
    }
  }
  baton->connectionHolder->closeConnection();
}

void afterQuery(uv_work_t* req, int bla) {
  HandleScope scope;
  BatonWithResult* baton = static_cast<BatonWithResult*>(req->data);
  Handle<Value> result = baton->result->getResultObject();
  Handle<Value> error = String::New(baton->result->errorText.c_str());
  Handle<Value> argv[] = { error, result };
  baton->callback->Call(Context::GetCurrent()->Global(), 2, argv);
  delete baton;
  scope.Close(Undefined());
}

void queryWithoutResult(uv_work_t* req) {
  QueryBatonWithoutCallback* baton = static_cast<QueryBatonWithoutCallback*>(req->data);
  Connection_T connection = baton->connectionHolder->getConnection();
  TRY
    Connection_execute(connection, baton->query.c_str());
  CATCH(SQLException)
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
  PreparedStatement_T pstmt = nullptr;
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
      if(baton->isSelect) {
        result = PreparedStatement_executeQuery(pstmt);
      } else {
        PreparedStatement_execute(pstmt);
      }
    CATCH(SQLException)
      error = 1;
      baton->result = new EmptyResult(Connection_getLastError(connection));
    END_TRY;
  }

  if(!error) {
    if(!baton->useResultSet) {
      if(baton->isSelect) {
        baton->result = parseResult(&connection, &result);
      } else {
        baton->result = parseResult(&connection);
      }
    } else {
      baton->result = new StreamingResult(result, baton->connectionHolder->getConnection());
      return;
    }
  }
  baton->connectionHolder->closeConnection();
}
}
