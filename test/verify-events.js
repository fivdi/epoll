/*
 * Verify that add and modify accept all valid event types. See issue #2.
 */
var Epoll = require("../build/Release/epoll").Epoll,
  epoll = new Epoll(function(){}),
  stdin = 0; // fd for stdin

try {
  epoll.add(0, Epoll.EPOLLIN).remove(0)
    .add(0, Epoll.EPOLLOUT).remove(0)
    .add(0, Epoll.EPOLLRDHUP).remove(0)
    .add(0, Epoll.EPOLLPRI).remove(0)
    .add(0, Epoll.EPOLLERR).remove(0)
    .add(0, Epoll.EPOLLHUP).remove(0)
    .add(0, Epoll.EPOLLET).remove(0)
    .add(0, Epoll.EPOLLONESHOT)
    .modify(0, Epoll.EPOLLIN)
    .modify(0, Epoll.EPOLLOUT)
    .modify(0, Epoll.EPOLLRDHUP)
    .modify(0, Epoll.EPOLLPRI)
    .modify(0, Epoll.EPOLLERR)
    .modify(0, Epoll.EPOLLHUP)
    .modify(0, Epoll.EPOLLET)
    .modify(0, Epoll.EPOLLONESHOT)
    .remove(0);
} catch (ex) {
  console.log('*** Error: ' + ex.message);
} finally {
  epoll.close();
}

