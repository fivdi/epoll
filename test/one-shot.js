/*
 * 
 */
var Epoll = require("../build/Release/epoll").Epoll,
  util = require('./util'),
  eventCount = 0,
  epoll,
  fd = 0; // stdin 

console.log('one-shot');

epoll = new Epoll(function (err, fd, events) {
  eventCount++;

  if (eventCount === 1 && events & Epoll.EPOLLIN) {
    setTimeout(function () {
      util.read(fd);

      epoll.remove(fd).close();

      if (eventCount === 1) {
        console.log('one-shot ok');
      }
    }, 500);
  } else {
    console.log('one-shot *** Error: unexpected event');
  }
});

epoll.add(fd, Epoll.EPOLLIN | Epoll.EPOLLONESHOT);

console.log('...Please press the enter key once.');

