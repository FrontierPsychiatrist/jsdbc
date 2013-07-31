C-mysql
=======
This is a mysql client for node.js that uses libzdb to query MySQL. Internally it uses a connection pool. It supports
prepared statements.

How to build
------------
You need libzdb installed, with headers. Just run

    node-gyp configure && node-gyp build

to build the module.

How to use
----------
Include it in node.js

    var mysql = require('nativemysql');

(this name will be changed in the future).

Connect with
    
    mysql.connect({
      host: 'localhost',
      user: 'myuser,
      password: 'mypassword',
      port: 3306,
      database: 'mydatabase'
    });

These values are non optional and must be set.

Querying
--------
After connecting you can execute basic queries like this

    mysql.query('SELECT * FROM table', function(data, err) {
      if(err) throw err;
      console.log(data);
    });
    //--> [ {name: 'FrontierPsychiatrist', id: '1', age: '27'}, {name : ...}]

    mysql.query("UPDATE table SET name = 'MCS' WHERE id = 1", function(data, err) {
      if(err) throw err;
      console.log(data);
    });
    //--> {affectedRows: 1}

    //you can leave the callback function and no result parsing will take place.
    mysql.query("UPDATE table SET name = 'MCS' WHERE mid = 1");

Prepared statements
-------------------
Prepared Statements are also allowed, they are used if the second argument is an array:
    
    mysql.query('SELECT * FROM table WHERE id = ?', [1], function(data, err) {
      if(err) throw err;
      console.log(data);
    });
    //--> [ {name: 'FrontierPsychiatrist', id: '1', age: '27'} ]

The callback function is mandatory for prepared statements.

Transactions
------------
Note: transactions are in an early state and may cause memory leaks or crash.

Transactions are used like this:

    mysql.transact( function(connection) {
      connection.query('INSERT INTO table (id, name) VALUES (1, "FrontierPsychiatrist")', function(data, err) {
        if(err) {
          connection.rollback();
          connection.close();
          throw err;
        } else {
          connection.query('SELECT * FROM table WERE id = ?', [1], function(data, err) {//contains an error!
            if(err) {
              connection.rollback();
              connection.close();
              throw err;
            } else {
              connection.commit();
            }
          })
        }
      );
    });

You pass a function that takes a connection object to mysql.transact and everything executed on this object will be transactional. Beware and commit will close the connection, so no further queries are allowed after executing commit. This does not work on MySQL MyISAM tables as they don't support transactions.

Future features
---------------
* Other databases may be included, as libzdb supports them it should be an easy integration.
* currently all fields in a prepared statement are treated as strings, this will be improved.
* A method to stream result sets. Currently, all data selected will be loaded into memory.
* a method to obtain the lastInserId from inserts