{
  "targets": [{
    "target_name": "epoll",
    "conditions": [[
      '"<!(echo $V)" != "1"', {
        "cflags": [
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

