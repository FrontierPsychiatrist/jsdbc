var database = require('jsdbc');

try {
  mysql.connect({
    type: 'mysql',
    host: 'localhost',
    user: 'user',
    password: 'password',
    port: 3306,
    database: 'database'
  });
} catch(e) {
  console.log(e);
  process.exit(1);
}

database.transact( function(con) {
  con.query('INSERT INTO table (name ,id) VALUES ("FrontierPsychiatrist", 1)', function(result, err) {
    if(err) {
      console.log(err);
      con.rollback();
      con.close();
    } else {
      console.log(result);
      con.query('SELECT * FROM tabke', function(result, err) {
        if(err) {
          console.log(err);
          con.rollback();
          con.close();
        } else {
          console.log(result);
          con.commit();
          con.close();
        }
      })
    }
  });
});

database.query("SELECT name FROM table WHERE id = ?", [1], function(data, err) {
  if(err) throw err;
  console.log(data);
});

database.stream("SELECT * FROM table", function(resultSet, err) {
  if (err) throw err;
  while(resultSet.next()) {
    console.log(resultSet.getString(1));
  }
  resultSet.close();
});
