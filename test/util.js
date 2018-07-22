'use strict';

var fs = require('fs');

module.exports = {
  read: function (fd) {
    var buf = Buffer.alloc(1024);
    fs.readSync(fd, buf, 0, buf.length, null);
  }
};

