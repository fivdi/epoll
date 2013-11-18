{
  "targets": [{
    "target_name": "epoll",
    "include_dirs" : [
      "<!(node -e \"require('nan')\")"
    ],
    "sources": [
      "./src/epoll.cc"
    ]
  }]
}

