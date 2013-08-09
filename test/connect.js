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

mysql.transact( function(con) {
  con.query('INSERT INTO transtest (name ,id) VALUES ("hurr", 1)', function(err, res) {
    if(err) {
      console.log(err);
      con.rollback();
      con.close();
    } else {
      console.log(res);
      con.select('SELEKT * FROM transtest', function(res, err) {
        if(err) {
          console.log(err);
          con.rollback();
          con.close();
        } else {
          console.log(res);
          con.commit();
          con.close();
        }
      })
    }
  });
});

mysql.select("SELECT m_usermail FROM pxm_message WHERE m_usernickname = ? AND m_id = ?", ["moritz", 3], function(err, res) {
  if(err) throw err;
  console.log(res);
});

mysql.select("SELECT * FROM pxm_message WHERE m_id = 1", function(err, res) {
  if(err) throw err;
  console.log(res);
});

mysql.query("UPDATE pxm_message SET m_usermail = 'Hurr2' WHERE m_id = 1",
  function(err, res) {
    if(err) throw err;
    console.log(res);
});

mysql.query('SELECT * FROM pxm_message');