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

mysql.query("SELECT * FROM pxm_message", function(data) {
  console.log(data);
})