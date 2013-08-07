var sqlite = require('../build/Release/jsdbc');

try {
  sqlite.connect({
    type: 'sqlite',
    host: '/Users/moritz/workspace/C/native-mysql/test',
    user: 'moritz',
    password: '',
    database: 'test.db'
  });
} catch(e) {
  console.log(e);
  process.exit(1);
}

sqlite.select('SELECT * FROM test', function(res, err) {
  if(err) throw err;
  console.log(res);
});

/*sqlite.stream('SELECT * FROM test', function(resultSet, err) {
  while(resultSet.next()) {
    console.log(resultSet.getString(1));
  }
  resultSet.close();
});*/

sqlite.select('SELECT * FROM test WHERE id = ?', [2], function(res, err) {
  console.log(res);
});

sqlite.transact(function(con) {
  con.query('INSERT INTO test (name, id) VALUES ("test", 2)', function(res, err) {
    if(err) {
      console.log(err);
      con.rollback();
      con.close();
    } else {
      con.select('SELECT * FROM test', function(res, err) {
        if(err) {
          console.log(err);
          con.rollback();
          con.close();
        } else {
          con.commit();
          con.close();
        }
      })
    }
  });
});