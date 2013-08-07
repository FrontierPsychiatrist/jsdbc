var postgres = require('../build/Release/jsdbc');

try {
  postgres.connect({
    type: 'postgresql',
    host: 'localhost',
    user: 'moritz',
    password: '',
    port: 5432,
    database: 'pgtest'
  });
} catch(e) {
  console.log(e);
  process.exit(1);
}

postgres.select('SELECT * FROM test', function(res, err) {
  if(err) throw err;
  console.log(res);
});

/*postgres.query('SELECT * FROM test WHERE id = ?', ["1"], function(res, err) {
  if(err) throw err;
  console.log(res);
});*/


postgres.transact( function(connection) {
  connection.query("INSERT INTO test (name, id) VALUES ('test', 1)", function(res, err) {
    if(err) {
      console.log("INSERT " + err);
      connection.rollback();
      connection.close();
    } else {
      connection.select('SELECT * FROM test', function(res, err) {
        if(err) {
          console.log("SELECT" + err);
          connection.rollback();
          connection.close();
        } else {
          connection.commit();
          connection.close();
        }
      });
    }
  })
});