var mysql = require('../build/Release/nativemysql');

try {
  mysql.connect({
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
  console.log("123" + resultSet);
  while(resultSet.next()) {
    console.log(resultSet.getString(1) + " " + resultSet.getString(5));
  }
});