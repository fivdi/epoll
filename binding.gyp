{
  "targets": [{
    "target_name": "epoll",
    "conditions": [[
      "OS == \"linux\"", {
        "cflags": [
          "-Wno-unused-local-typedefs",
          "-Wno-deprecated-declarations"
        ]
      }]
    ],
    "include_dirs" : [
      "<!(node -e \"require('nan')\")"
    ],
    "sources": [
      "./src/epoll.cc"
    ]
  }]
}

