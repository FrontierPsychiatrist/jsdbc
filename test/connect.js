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

mysql.query("SELECT * FROM pxm_message WHERE m_id = 1", function(data, err) {
  if(err) {
    console.log("Fehler: " + err)
  } else {
    console.log(data);
  }
});

mysql.query("SELECT * FROM pxm_message WHERE m_id = 1", function(data, err) {
  if(err) {
    console.log("Fehler: " + err)
  } else {
    console.log(data);
  }
});


mysql.query("UPDATE pxm_message SET m_usermail = 'Hurr' WHERE m_id = 1",
  function(result, err) {
    if(err) {
      console.log("Fehler: " + err);
    } else {
      console.log(result);
    }
});