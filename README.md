## epoll

A low-level Node.js binding for the Linux epoll API for monitoring multiple
file descriptors to see if I/O is possible on any of them.

This module was initially written to detect EPOLLPRI events indicating that
urgent data is available for reading. EPOLLPRI events are triggered by
interrupt generating [GPIO](https://www.kernel.org/doc/Documentation/gpio.txt)
pins. The epoll module is used by [onoff](https://github.com/fivdi/onoff)
to detect interrupts.

## Installation

    $ [sudo] npm install epoll

## API

  * Epoll(callback) - Constructor. The callback is called when events occur and it gets three
arguments (err, fd, events).
  * add(fd, events) - Register file descriptor fd for the event types specified by events.
  * remove(fd) - Deregister file descriptor fd.
  * modify(fd, events) - Change the event types associated with file descriptor fd.
  * close(fd) - Deregisters all file descriptors and frees resources.

Event Types

  * Epoll.EPOLLIN
  * Epoll.EPOLLOUT
  * Epoll.EPOLLRDHUP
  * Epoll.EPOLLPRI
  * Epoll.EPOLLERR
  * Epoll.EPOLLHUP
  * Epoll.EPOLLET
  * Epoll.EPOLLONESHOT

## Example

```js
var Epoll = require('epoll').Epoll,
  fs = require('fs'),
  valuefd = fs.openSync('/sys/class/gpio/gpio18/value', 'r'),
  buffer = new Buffer(1);

var poller = new Epoll(function (err, fd, events) {
  fs.readSync(fd, buffer, 0, 1, 0);
  console.log(buffer.toString() === '1' ? 'pressed' : 'released');
});

fs.readSync(valuefd, buffer, 0, 1, 0);
poller.add(valuefd, Epoll.EPOLLPRI);

setTimeout(function () {
  poller.remove(valuefd).close();
}, 30000);
```

## Limitations

