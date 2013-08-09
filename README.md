jsdbc
=======
This is a database client for node.js that uses [libzdb](http://www.tildeslash.com/libzdb/) to query some relational databases. Internally it uses a connection pool. It supports prepared statements and result sets. You can use it with MySQL, Postgres, Oracle and SQLite databases.

How to build
------------
You need libzdb installed, with headers. Just run

    node-gyp configure && node-gyp build

to build the module.

jsdbc is also available on npm, so you can run

    npm install jsdbc

as well.

How to use
----------
Include it in node.js

    var mysql = require('jsdbc');

Connect with
    
    mysql.connect({
      type: 'mysql',
      host: 'localhost',
      user: 'myuser,
      password: 'mypassword',
      port: 3306,
      database: 'mydatabase'
    });

These values are non optional and must be set.

Type has to be one of mysql, postgresql, oracle or sqlite.

For sqlite the port is optional. host contains the path to the database file and database is the database file name.

    var sqlite = require('jsdbc');

    sqlite.connect({
      type: 'sqlite',
      host: '/var/sqlite',
      database: 'database.db'
    });

Querying
--------
jsdbc has one method for selecting and one for other queries.

    mysql.select('SELECT * FROM table', function(err, res) {
      if(err) throw err;
      console.log(res);
    });
    //--> [ {name: 'FrontierPsychiatrist', id: '1', age: '27'}, {name : ...}]

    mysql.query("UPDATE table SET name = 'MCS' WHERE id = 1", function(err, res) {
      if(err) throw err;
      console.log(res);
    });
    //--> {affectedRows: 1}

    //you can leave the callback function and no result parsing will take place.
    mysql.query("UPDATE table SET name = 'MCS' WHERE mid = 1");

Prepared statements
-------------------
Prepared Statements are also allowed, they are used if the second argument is an array:
    
    mysql.select('SELECT * FROM table WHERE id = ?', [1], function(err, res) {
      if(err) throw err;
      console.log(res);
    });
    //--> [ {name: 'FrontierPsychiatrist', id: '1', age: '27'} ]

The callback function is mandatory for prepared statements.

Transactions
------------
Transactions are used like this:

    mysql.transact( function(connection) {
      connection.query('INSERT INTO table (id, name) VALUES (1, "FrontierPsychiatrist")', function(err, res) {
        if(err) {
          connection.rollback();
          connection.close();
          throw err;
        } else {
          connection.select('SELECT * FROM table WERE id = ?', [1], function(err, res) {//contains an error!
            if(err) {
              connection.rollback();
              connection.close();
              throw err;
            } else {
              connection.commit();
              connection.close();
            }
          })
        }
      );
    });

You pass a function that takes a connection object to mysql.transact and everything executed on this object will be transactional. Make sure to close your connection! If the connection object gets garbage collected by V8 the connection will be closed, but garbage collection does only happen that often and the connection pool isn't infinite.

Result Sets
-----------
The standard query approach loads all resulting rows into memory. This can be unfortunate if you have a large number of rows selected. To overcome this, the standard streaming result set approach is implemented.

    mysql.stream("SELECT * FROM table WHERE age > ?", [18], function(err, resultSet) {
      if (err) throw err;
      while(resultSet.next()) {
        console.log(resultSet.getString(1));
      }
      resultSet.close();
    });

Again, it is important that you close your result set to return the connection to the pool.

* Currently, only getString is implemented
* mixing transactions and streaming result sets is not yet possible

Future features
---------------
* currently all fields in a prepared statement are treated as strings, this will be improved.
* a method to obtain the lastInserId from inserts

Known bugs
----------
* SQlite has issues with parallel transactions and result sets.
* Executing .query with a select statement on a MySQL database won't return the connection to the pool. Use .select