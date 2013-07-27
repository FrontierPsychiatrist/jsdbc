{
  "targets": [
    {
      "target_name": "nativemysql",
      "sources": ["src/nativemysql.cc"],
      "link_settings" : {
        "libraries": ["-lzdb"]
      },
      "include_dirs": ["/usr/local/include/zdb"]
    }
  ]
}
