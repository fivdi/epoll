module.exports = (() => {
  const osType = require('os').type();
  if (osType == 'Linux') {
    require('bindings')('epoll.node');
  } else {
    console.warn(`epoll is built for Linux. Reported OS: ${osType}. Begin mock mode.`);
    return { Epoll: {} }
  }
})()