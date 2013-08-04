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

mysql.transact( function(con) {
  con.query('INSERT INTO transtest (name ,id) VALUES ("hurr", 1)', function(result, err) {
    if(err) {
      console.log(err);
      con.rollback();
      con.close();
    } else {
      console.log(result);
      con.query('SELECT * FROM transtest', function(result, err) {
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

mysql.query("SELECT m_usermail FROM pxm_message WHERE m_usernickname = ? AND m_id = ?", ["moritz", 3], function(data, err) {
  if(err) throw err;
  console.log(data);
});

mysql.query("SELECT * FROM pxm_message WHERE m_id = 1", function(data, err) {
  if(err) throw err;
  console.log(data);
});

mysql.query("UPDATE pxm_message SET m_usermail = 'Hurr2' WHERE m_id = 1",
  function(result, err) {
    if(err) throw err;
    console.log(result);
});

mysql.query('SELECT * FROM pxm_message');