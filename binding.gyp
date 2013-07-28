{
  "targets": [
    {
      "target_name": "nativemysql",
      "sources": ["src/nativemysql.cc", "src/worker_functions.cc"],
      "link_settings" : {
        "libraries": ["-lzdb"]
      },
      "include_dirs": ["/usr/local/include/zdb"]
    }
  ]
}
