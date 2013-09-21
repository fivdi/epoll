/*
 * Make sure process terminates when 'almsost nothing' is actually done.
 */
var Epoll = require("../build/Release/epoll").Epoll,
  epoll = new Epoll(function(){});

console.log('do-almost-nothing');

epoll.add(0, Epoll.EPOLLIN).remove(0).close();

