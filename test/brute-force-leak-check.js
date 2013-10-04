/*
 * Create a million epoll instances and use each of them to detect a single
 * event. The goal here is ensure that memory usage doesn't constantly increase
 * over time. Using valgrind might be a better idea here.
 *
 * This test expects a newline as input on stdin. It polls for events on stdin
 * but doesn't read stdin until it has been notified about the availability of
 * input data 1000000 times.
 */
var Epoll = require('../build/Release/epoll').Epoll,
  util = require('./util'),
  count = 0,
  stdin = 0; // fd for stdin

function once() {
  var epoll = new Epoll(function (err, fd, events) {
    epoll.remove(fd).close();

    count++;

    if (count % 1e5 === 0) {
      console.log('           ' + count + ' instances created and events detected ');
    }
    if (count < 1e6) {
      once();
    } else {
      util.read(fd); // read stdin (the newline)
    }
  });

  epoll.add(stdin, Epoll.EPOLLIN);
}

once();

