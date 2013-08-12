{
  "targets": [
    {
      "target_name": "jsdbc",
      "sources": ["src/jsdbc.cc", "src/worker_functions.cc", "src/result_set.cc", "src/transact.cc"],
      "link_settings" : {
        "libraries": ["-lzdb"]
      },
      "include_dirs": ["/usr/local/include/zdb", "/usr/include/zdb"],
      "conditions" : [
        ["OS=='mac'", {
          "xcode_settings": {
            "OTHER_CPLUSPLUSFLAGS" : ["-std=c++11", "-stdlib=libc++"],
            "GCC_ENABLE_CPP_EXCEPTIONS": 'YES',
            "MACOSX_DEPLOYMENT_TARGET" : "10.8"
          }
          }],
        ["OS=='linux'", {
          "cflags_cc": [
            "-std=c++11",
            "-Wall",
            "-pedantic",
            "-fexceptions"
          ]
        }]
      ]
    }
  ]
}
