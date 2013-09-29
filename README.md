## epoll

A low-level Node.js binding for the Linux epoll API for monitoring multiple
file descriptors to see if I/O is possible on any of them.

This module was initially written to detect EPOLLPRI events indicating that
urgent data is available for reading. EPOLLPRI events are triggered by
interrupt generating [GPIO](https://www.kernel.org/doc/Documentation/gpio.txt)
pins. The epoll module is used by [onoff](https://github.com/fivdi/onoff)
to detect such interrupts.

## Installation

    $ [sudo] npm install epoll

## API

  * Epoll(callback) - Constructor. The callback is called when epoll events
    occur and it gets three arguments (err, fd, events).
  * add(fd, events) - Register file descriptor fd for the event types specified
    by events.
  * remove(fd) - Deregister file descriptor fd.
  * modify(fd, events) - Change the event types associated with file descriptor
    fd to those specified by events.
  * close(fd) - Deregisters all file descriptors and free resources.

Event Types

  * Epoll.EPOLLIN
  * Epoll.EPOLLOUT
  * Epoll.EPOLLRDHUP
  * Epoll.EPOLLPRI
  * Epoll.EPOLLERR
  * Epoll.EPOLLHUP
  * Epoll.EPOLLET
  * Epoll.EPOLLONESHOT

## Example - Watching GPIO Inputs

The following example shows how epoll can be used to detect interrupts from a
momentary push-button connected to GPIO #18 (pin P1-12) on the Raspberry Pi.
The same example for the BeagleBone using GPIO #117 is also available in the
[example directory](https://github.com/fivdi/epoll/tree/master/example/watch-button).

The first step is to export GPIO #18 as an interrupt generating input using
the pi-export bash script from the examples directory.

    $ [sudo] ./pi-export

pi-export:
```bash
#!/bin/sh
echo 18 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio18/direction
echo both > /sys/class/gpio/gpio18/edge
```

Then run pi-watch-button to be notified every time the button is pressed and
released. If there is no hardware debounce circuit for the push-button, contact
bounce issues are very likely to be visible on the console output.
pi-watch-button terminates automatically after 30 seconds.

    $ [sudo] node pi-watch-button


pi-watch-button:
```js
var Epoll = require('epoll').Epoll,
  fs = require('fs'),
  valuefd = fs.openSync('/sys/class/gpio/gpio18/value', 'r'),
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
```

When pi-watch-button has terminated, GPIO #18 can be unexported using the
pi-unexport bash script.

    $ [sudo] ./pi-unexport

pi-unexport:
```bash
#!/bin/sh
echo 18 > /sys/class/gpio/unexport
```

## Limitations

