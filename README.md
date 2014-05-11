## epoll

A low-level Node.js binding for the Linux epoll API for monitoring multiple
file descriptors to see if I/O is possible on any of them.

This module was initially written to detect EPOLLPRI events indicating that
urgent data is available for reading. EPOLLPRI events are triggered by
interrupt generating [GPIO](https://www.kernel.org/doc/Documentation/gpio/)
pins. The epoll module is used by [onoff](https://github.com/fivdi/onoff)
to detect such interrupts.

## Installation

    $ [sudo] npm install epoll

**BeagleBone Prerequisites**

Before installing epoll on stock Ångström on the BeagleBone three Python
modules need to be installed; python-compiler, python-misc, and
python-multiprocessing. They can be installed with the following commands:

```bash
$ opkg update
$ opkg install python-compiler
$ opkg install python-misc
$ opkg install python-multiprocessing
```

## API

  * Epoll(callback) - Constructor. The callback is called when epoll events
    occur and it gets three arguments (err, fd, events).
  * add(fd, events) - Register file descriptor fd for the event types specified
    by events.
  * remove(fd) - Deregister file descriptor fd.
  * modify(fd, events) - Change the event types associated with file descriptor
    fd to those specified by events.
  * close() - Deregisters all file descriptors and free resources.

Event Types

  * Epoll.EPOLLIN
  * Epoll.EPOLLOUT
  * Epoll.EPOLLRDHUP
  * Epoll.EPOLLPRI
  * Epoll.EPOLLERR
  * Epoll.EPOLLHUP
  * Epoll.EPOLLET
  * Epoll.EPOLLONESHOT

Event types can be combined with | when calling add or modify. For example,
Epoll.EPOLLPRI | Epoll.EPOLLONESHOT could be passed to add to detect a single
GPIO interrupt.

## Example - Watching Buttons

The following example shows how epoll can be used to detect interrupts from a
momentary push-button connected to GPIO #18 (pin P1-12) on the Raspberry Pi.
The source code is available in the [example directory]
(https://github.com/fivdi/epoll/tree/master/example/watch-button) and can
easily be modified for using a different GPIO on the Pi or a different platform
such as the BeagleBone.

The first step is to export GPIO #18 as an interrupt generating input using
the export bash script from the examples directory.

    $ [sudo] ./export

export:
```bash
#!/bin/sh
echo 18 > /sys/class/gpio/export
echo in > /sys/class/gpio/gpio18/direction
echo both > /sys/class/gpio/gpio18/edge
```

Then run watch-button to be notified every time the button is pressed and
released. If there is no hardware debounce circuit for the push-button, contact
bounce issues are very likely to be visible on the console output.
watch-button terminates automatically after 30 seconds.

    $ [sudo] node watch-button

watch-button:
```js
var Epoll = require('epoll').Epoll,
  fs = require('fs'),
  valuefd = fs.openSync('/sys/class/gpio/gpio18/value', 'r'),
  buffer = new Buffer(1);

// Create a new Epoll. The callback is the interrupt handler.
var poller = new Epoll(function (err, fd, events) {
  // Read GPIO value file. Reading also clears the interrupt.
  fs.readSync(fd, buffer, 0, 1, 0);
  console.log(buffer.toString() === '1' ? 'pressed' : 'released');
});

// Read the GPIO value file before watching to
// prevent an initial unauthentic interrupt.
fs.readSync(valuefd, buffer, 0, 1, 0);

// Start watching for interrupts.
poller.add(valuefd, Epoll.EPOLLPRI);

// Stop watching after 30 seconds.
setTimeout(function () {
  poller.remove(valuefd).close();
}, 30000);
```

When watch-button has terminated, GPIO #18 can be unexported using the
unexport bash script.

    $ [sudo] ./unexport

unexport:
```bash
#!/bin/sh
echo 18 > /sys/class/gpio/unexport
```

## Example - Interrupts per Second

The following example shows how epoll can be used to detect interrupts when the
state of an LED connected to GPIO #38 on the BeagleBone changes.
The source code is available in the [example directory]
(https://github.com/fivdi/epoll/tree/master/example/watch-led) and can
easily be modified for using a different GPIO on the BeagleBone or a different
platform such as the Raspberry Pi.

The goal here is to determine how many interrupts can be handled per second.

The first step is to export GPIO #38 as an interrupt generating output (!) using
the export bash script from the examples directory.

    $ [sudo] ./export

export:
```bash
#!/bin/sh
echo 38 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio38/direction
echo both > /sys/class/gpio/gpio38/edge
```

Then run watch-led. watch-led toggles the state of the LED every time it
detects an interrupt. Each toggle will trigger the next interrupt. After five
seconds, watch-led prints the number of interrupts it detected per second. The
LED is turned on and off several thousand times per second so no blinking will
be visible, the LED will light at about half brightness.

    $ [sudo] node watch-led

watch-led:
```js
var Epoll = require('epoll').Epoll,
  fs = require('fs'),
  valuefd = fs.openSync('/sys/class/gpio/gpio38/value', 'r+'),
  value = new Buffer(1),  // The three Buffers here are global
  zero = new Buffer('0'), // to improve performance.
  one = new Buffer('1'),
  count = 0,
  time;

// Create a new Epoll. The callback is the interrupt handler.
var poller = new Epoll(function (err, fd, events) {
  var nextValue;

  count++;

  // Read GPIO value file. Reading also clears the interrupt.
  fs.readSync(fd, value, 0, 1, 0);

  // Toggle GPIO value. This will eventually result
  // in the next interrupt being triggered.
  nextValue = value[0] === zero[0] ? one : zero;
  fs.writeSync(fd, nextValue, 0, nextValue.length, 0);
});

time = process.hrtime(); // Get start time.

// Start watching for interrupts. This will trigger the first interrupt
// as the value file already has data waiting for a read.
poller.add(valuefd, Epoll.EPOLLPRI);

// Print interrupt rate to console after 5 seconds.
setTimeout(function () {
  var rate;

  time = process.hrtime(time); // Get run time.
  rate = Math.floor(count / (time[0] + time[1] / 1E9));
  console.log(rate + ' interrupts per second');

  // Stop watching.
  poller.remove(valuefd).close();
}, 5000);
```

When watch-led has terminated, GPIO #38 can be unexported using the
unexport bash script.

    $ [sudo] ./unexport

unexport:
```bash
#!/bin/sh
echo 38 > /sys/class/gpio/unexport
```

Here are some results from the "Interrupts per Second" example.

**BeagleBone, 720MHz, Ångström v2012.12, Kernel 3.8.13, epoll v0.0.6:**

Node.js | Interrupts / Second
:---: | ---:
v0.11.7 | 7152
v0.10.20 | 5861
v0.8.22 | 6098

**BeagleBone Black, 1GHz, Ångström v2012.12, Kernel 3.8.13, epoll v0.0.8:**

Node.js | Interrupts / Second
:---: | ---:
v0.11.8 | 9760

**Raspberry Pi, 700Mhz, Raspbian, Kernel 3.2.27+, epoll v0.0.6:**

Node.js | Interrupts / Second
:---: | ---:
v0.11.07 | 4071
v0.10.16 | 3530
v0.8.14 | 3591

