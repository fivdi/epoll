var Epoll = require('../../build/Release/epoll').Epoll,
  fs = require('fs'),
  valuefd = fs.openSync('/sys/class/gpio/gpio117/value', 'r'),
  buffer = new Buffer(1);

var poller = new Epoll(function (err, fd, events) {
  fs.readSync(fd, buffer, 0, 1, 0);
  console.log(buffer.toString() === '1' ? 'pressed' : 'released');
});

// Read value file before polling to prevent an initial unauthentic interrupt
fs.readSync(valuefd, buffer, 0, 1, 0);
poller.add(valuefd, Epoll.EPOLLPRI);

setTimeout(function () {
  poller.remove(valuefd).close();
}, 30000);

