var mysql = require('../build/Release/jsdbc');

try {
  mysql.connect({
    type: 'mysql',
    host: 'localhost',
    user: 'pxm',
    password: 'pxm',
    port: 3306,
    database: 'pxmboard'
  });
} catch(e) {
  console.log(e);
  process.exit(1);
}

mysql.stream("SELECT * FROM pxm_message", function(resultSet, err) {
  if (err) throw err;
  while(resultSet.next()) {
    console.log(resultSet.getString(1) + " " + resultSet.getString(5));
  }
  resultSet.close();
});

mysql.stream("SELECT * FROM pxm_message WHERE m_id = ?", [1], function(resultSet, err) {
  if (err) throw err;
    while(resultSet.next()) {
      console.log(resultSet.getString(1) + " " + resultSet.getString(5));
    }
  resultSet.close();
});