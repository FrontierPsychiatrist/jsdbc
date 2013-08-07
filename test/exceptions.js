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

try {
  mysql.select();
} catch(e) {
  console.log(e);
}

try {
  mysql.select('TEST', [1]);
} catch(e) {
  console.log(e);
}

try {
  mysql.select('TEST', 1);
} catch(e) {
  console.log(e);
}

try {
  mysql.select(123);
} catch(e) {
  console.log(e);
}

try {
  mysql.select('TEST', [1], 'Hallo');
} catch(e) {
  console.log(e);
}

try {
  mysql.stream('TEST', [1], 'Hallo');
} catch(e) {
  console.log(e);
}

try {
  mysql.stream(function() {});
} catch(e) {
  console.log(e);
}

mysql.transact( function(con) {
  try {
    con.select(function() {});
  } catch(e) {
    console.log(e);
  }  
});