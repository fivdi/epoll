var Epoll = require('../build/Release/epoll').Epoll,
  fs = require('fs'),
  valuefd = fs.openSync('/sys/class/gpio/gpio18/value', 'r'),
  buffer = new Buffer(1),
  count = 0,
  poller;

poller = new Epoll(function (err, fd, events) {
  if (err) throw err;

  fs.readSync(fd, buffer, 0, 1, 0);
  console.log('button pressed, value is ' + buffer.toString());

  count++;
  if (count === 20) {
    poller.remove(fd).close();
  }
});

fs.readSync(valuefd, buffer, 0, 1, 0);
poller.add(valuefd, Epoll.EPOLLPRI);

