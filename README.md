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

  * Epoll(callback) - Constructor. The callback is called when events occur and
it gets three arguments (err, fd, events).
  * add(fd, events) - Register file descriptor fd for the event types specified
by events.
  * remove(fd) - Deregister file descriptor fd.
  * modify(fd, events) - Change the event types associated with file descriptor
fd.
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

## Examples

Now for two examples, one for the Raspberry Pi and one for the BeagleBone.

## Raspberry Pi

The following example shows how epoll can be used to detect interrupts from a
momentary push button connected to GPIO #18 (pin P1-12) on the Raspberry Pi.

Export GPIO #18 as an interrupt generating input using the pi-export script
from the examples directory.

    $ [sudo] pi-export

```bash
#!/bin/sh
echo 18 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio18/direction
echo both > /sys/class/gpio/gpio18/edge
```

Then run pi-watch-button to be notified every time the button is pressed and
released.

    $ [sudo] node pi-watch-button

pi-watch-button terminates automatically after 30 seconds.

```js
var Epoll = require('../build/Release/epoll').Epoll,
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
pi-unexport script.

    $ [sudo] pi-unexport

```bash
#!/bin/sh
echo 18 > /sys/class/gpio/unexport
```

## BeagleBone

The following example shows how epoll can be used to detect interrupts from a
momentary push button connected to GPIO #117 on the BeagleBone.

Export GPIO #117 as an interrupt generating input using the bb-export script
from the examples directory.

    $ [sudo] bb-export

```bash
#!/bin/sh
#!/bin/sh
echo 117 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio117/direction
echo both > /sys/class/gpio/gpio117/edge
```

Then run bb-watch-button to be notified every time the button is pressed and
released.

    $ [sudo] node bb-watch-button

bb-watch-button terminates automatically after 30 seconds.

```js
var Epoll = require('../build/Release/epoll').Epoll,
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
```

When bb-watch-button has terminated, GPIO #117 can be unexported using the
bb-unexport script.

    $ [sudo] bb-unexport

```bash
#!/bin/sh
echo 117 > /sys/class/gpio/unexport
```

## Limitations

