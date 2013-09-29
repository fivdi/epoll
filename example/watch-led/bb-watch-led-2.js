var Epoll = require('../../build/Release/epoll').Epoll,
  fs = require('fs'),
  valuefd = fs.openSync('/sys/class/gpio/gpio38/value', 'r+'),
  count = 0,
  time;

var poller = new Epoll(function (err, fd, events) {
  var value = new Buffer(1),  // The three Buffers here are global
  var zero = new Buffer('0'), // to improve performance.
  var one = new Buffer('1'),
  var nextValue;

  count++;

  // Read current value and clear interrupt.
  fs.readSync(fd, value, 0, 1, 0);

  // Toggle value. This will eventually result
  // in the next interrupt being triggered.
  nextValue = value[0] === zero[0] ? one : zero;
  fs.writeSync(fd, nextValue, 0, nextValue.length, 0);
});

// Start polling the GPIO value file. This will trigger the first
// interrupt as the value file already has data waiting for a read.
poller.add(valuefd, Epoll.EPOLLPRI);

time = process.hrtime(); // Get start time.

// Print interrupt rate to console after 5 seconds.
setTimeout(function () {
  var rate;

  time = process.hrtime(time); // Get run time.
  rate = Math.floor(count / (time[0] + time[1] / 1E9));
  console.log(rate + ' interrupts per second');

  // Stop polling the GPIO value file and close poller.
  poller.remove(valuefd).close();
}, 5000);

