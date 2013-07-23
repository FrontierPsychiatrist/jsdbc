var mysql = require('../build/Release/nativemysql');

mysql.login(function(res) {
  console.log(res);
});