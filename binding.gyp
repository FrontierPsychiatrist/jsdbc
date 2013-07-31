{
  "targets": [
    {
      "target_name": "nativemysql",
      "sources": ["src/nativemysql.cc", "src/worker_functions.cc", "src/result_set.cc"],
      "link_settings" : {
        "libraries": ["-lzdb"]
      },
      "include_dirs": ["/usr/local/include/zdb"]
    }
  ]
}
