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

mysql.stream("SELECT * FROM pxm_message WHERE m_id = 1", function(err, resultSet) {
  if (err) throw err;
  while(resultSet.next()) {
    console.log(resultSet.getString(1) + " " + resultSet.getString(5));
  }
  resultSet.close();
});

mysql.stream("SELECT * FROM pxm_message WHERE m_id = ?", [1], function(err, resultSet) {
  if (err) throw err;
    while(resultSet.next()) {
      console.log('With pstmt: ' + resultSet.getString(1) + " " + resultSet.getString(5));
    }
  resultSet.close();
});

mysql.stream("SELECT m_id FROM pxm_message WHERE m_id = 1", function(err, resultSet) {
  if(err) throw err;
  var data = ['String by name'];
  while(resultSet.next()) {
    data.push(resultSet.getString('m_id'));
    console.log(data);
  }
  resultSet.close();
});

mysql.stream("SELECT m_id FROM pxm_message WHERE m_id = 1", function(err, resultSet) {
  if(err) throw err;
  var data = ['getInt'];
  while(resultSet.next()) {
    data.push(resultSet.getInt(1));
    console.log(data);
  }
  resultSet.close();
});

mysql.stream("SELECT m_id FROM pxm_message WHERE m_id = 1", function(err, resultSet) {
  if(err) throw err;
  var data = ['getIntByName'];
  while(resultSet.next()) {
    data.push(resultSet.getInt('m_id'));
    console.log(data);
  }
  resultSet.close();
});

mysql.stream("SELECT m_id FROM pxm_message WHERE m_id = 1", function(err, resultSet) {
  if(err) throw err;
  var data = ['getDouble'];
  while(resultSet.next()) {
    data.push(resultSet.getDouble(1));
    console.log(data);
  }
  resultSet.close();
});

mysql.stream("SELECT m_id FROM pxm_message WHERE m_id = 1", function(err, resultSet) {
  if(err) throw err;
  var data = ['getDoubleByName'];
  while(resultSet.next()) {
    data.push(resultSet.getDouble('m_id'));
    console.log(data);
  }
  resultSet.close();
});

mysql.stream("SELECT * FROM pxm_message", function(err, resultSet) {
  var data = ['ColumnNames'];
  for(var i = 0; i < resultSet.getColumnCount(); i++) {
    data.push(resultSet.getColumnName(i+1));
  }
  console.log(data);
  resultSet.close();
});