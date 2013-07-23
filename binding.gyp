{
  "targets": [
    {
      "target_name": "nativemysql",
      "sources": ["src/nativemysql.cc"],
      "link_settings" : {
        "libraries": ["-lmysqlclient", "-lz"]
      }
    }
  ]
}
