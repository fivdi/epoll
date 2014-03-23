#ifdef __linux__

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <map>
#include <list>
#include <uv.h>
#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <node_version.h>
#include <nan.h>
#include "epoll.h"

// TODO - strerror isn't threadsafe, use strerror_r instead
// TODO - use uv_strerror rather than strerror_r for libuv errors?

static int epfd_g;

static uv_sem_t sem_g;
static uv_async_t async_g;

static struct epoll_event event_g;
static int errno_g;


/*
 * Watcher thread
 */
static void *watcher(void *arg) {
  int count;

  while (true) {
    // Wait till the event loop says it's ok to poll. The semaphore serves more
    // than one purpose.
    // - It synchronizing access to '1 element queue' in variables
    //   event_g and errno_g.
    // - When level-triggered epoll is used, the default when EPOLLET isn't
    //   specified, the event triggered by the last call to epoll_wait may be
    //   trigged again and again if the condition that triggered it hasn't been
    //   cleared yet. Waiting prevents multiple triggers for the same event.
    // - It forces a context switch from the watcher thread to the event loop
    //   thread.
    uv_sem_wait(&sem_g);

    do {
      count = epoll_wait(epfd_g, &event_g, 1, -1);
    } while (count == -1 && errno == EINTR);

    errno_g = count == -1 ? errno : 0;

    // Errors returned from uv_async_send are silently ignored.
    uv_async_send(&async_g);
  }

  return 0;
}


static int start_watcher() {
  static bool watcher_started = false;
  pthread_t theread_id;

  if (watcher_started)
    return 0;

  epfd_g = epoll_create1(0);
  if (epfd_g == -1)
    return errno;

  int err = uv_sem_init(&sem_g, 1);
  if (err < 0) {
    // node v0.11.5 uses uv v0.11.7. The uv v0.11.7 version of uv_sem_init
    // returns -errno on error. uv_sem_init in lower versions of node returns
    // -1 on error. So, if the node version is less than v0.11.5, set err to
    // -errno.
#if ! NODE_VERSION_AT_LEAST(0, 11, 5)
    err = -errno;
#endif
    close(epfd_g);
    return -err;
  }

  err = uv_async_init(uv_default_loop(), &async_g, Epoll::DispatchEvent);
  if (err < 0) {
    // node v0.11.5 uses uv v0.11.7. The uv v0.11.7 version of uv_async_init
    // returns -errno on error. uv_async_init in lower versions of node returns
    // -1 on error. So, if the node version is less than v0.11.5, set err to
    // -errno.
#if ! NODE_VERSION_AT_LEAST(0, 11, 5)
    err = -errno;
#endif
    close(epfd_g);
    uv_sem_destroy(&sem_g);
    return -err;
  }

  // Prevent async_g from keeping event loop alive, for the time being.
  uv_unref((uv_handle_t *) &async_g);

  err = pthread_create(&theread_id, 0, watcher, 0);
  if (err != 0) {
    close(epfd_g);
    uv_sem_destroy(&sem_g);
    uv_close((uv_handle_t *) &async_g, 0);
    return err;
  }

  watcher_started = true;

  return 0;
}


/*
 * Epoll
 */
v8::Persistent<v8::FunctionTemplate> Epoll::constructor;
std::map<int, Epoll*> Epoll::fd2epoll;


Epoll::Epoll(NanCallback *callback)
  : callback_(callback), closed_(false) {
};


Epoll::~Epoll() {
  // v8 decides when and if destructors are called. In particular, if the
  // process is about to terminate, it's highly likely that destructors will
  // not be called. This is therefore not the place for calling the likes of
  // uv_unref, which, in general, must be called to terminate a process
  // gracefully!
  NanScope();

  if (callback_) delete callback_;
};


void Epoll::Init(v8::Handle<v8::Object> exports) {
  NanScope();

  // Constructor
  v8::Local<v8::FunctionTemplate> ctor = v8::FunctionTemplate::New(Epoll::New);
  NanAssignPersistent(v8::FunctionTemplate, constructor, ctor);
  ctor->SetClassName(NanSymbol("Epoll"));
  ctor->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "add", Add);
  NODE_SET_PROTOTYPE_METHOD(ctor, "modify", Modify);
  NODE_SET_PROTOTYPE_METHOD(ctor, "remove", Remove);
  NODE_SET_PROTOTYPE_METHOD(ctor, "close", Close);

  v8::Local<v8::ObjectTemplate> proto = ctor->PrototypeTemplate();
  proto->SetAccessor(NanSymbol("closed"), GetClosed);

  NODE_DEFINE_CONSTANT(ctor, EPOLLIN);
  NODE_DEFINE_CONSTANT(ctor, EPOLLOUT);
  NODE_DEFINE_CONSTANT(ctor, EPOLLRDHUP);
  NODE_DEFINE_CONSTANT(ctor, EPOLLPRI); // The reason this addon was created!
  NODE_DEFINE_CONSTANT(ctor, EPOLLERR);
  NODE_DEFINE_CONSTANT(ctor, EPOLLHUP);
  NODE_DEFINE_CONSTANT(ctor, EPOLLET);
  NODE_DEFINE_CONSTANT(ctor, EPOLLONESHOT);

  exports->Set(NanSymbol("Epoll"), ctor->GetFunction());
}


NAN_METHOD(Epoll::New) {
  NanScope();

  if (args.Length() < 1 || !args[0]->IsFunction())
    return NanThrowError("First argument to construtor must be a callback");

  NanCallback *callback = new NanCallback(v8::Local<v8::Function>::Cast(args[0]));

  Epoll *epoll = new Epoll(callback);
  epoll->Wrap(args.This());

  NanReturnValue(args.This());
}


NAN_METHOD(Epoll::Add) {
  NanScope();

  Epoll *epoll = ObjectWrap::Unwrap<Epoll>(args.This());

  if (epoll->closed_)
    return NanThrowError("add can't be called after calling close");

  // Epoll.EPOLLET is -0x8000000 on ARM and an IsUint32 check fails so
  // check for IsNumber instead.
  if (args.Length() < 2 || !args[0]->IsInt32() || !args[1]->IsNumber())
    return NanThrowError("incorrect arguments passed to add"
      "(int fd, int events)");

  int err = epoll->Add(args[0]->Int32Value(), args[1]->Int32Value());
  if (err != 0)
    return NanThrowError(strerror(err), err);

  NanReturnValue(args.This());
}


NAN_METHOD(Epoll::Modify) {
  NanScope();

  Epoll *epoll = ObjectWrap::Unwrap<Epoll>(args.This());

  if (epoll->closed_)
    return NanThrowError("modify can't be called after calling close");

  // Epoll.EPOLLET is -0x8000000 on ARM and an IsUint32 check fails so
  // check for IsNumber instead.
  if (args.Length() < 2 || !args[0]->IsInt32() || !args[1]->IsNumber())
    return NanThrowError("incorrect arguments passed to modify"
      "(int fd, int events)");

  int err = epoll->Modify(args[0]->Int32Value(), args[1]->Int32Value());
  if (err != 0)
    return NanThrowError(strerror(err), err);

  NanReturnValue(args.This());
}


NAN_METHOD(Epoll::Remove) {
  NanScope();

  Epoll *epoll = ObjectWrap::Unwrap<Epoll>(args.This());

  if (epoll->closed_)
    return NanThrowError("remove can't be called after calling close");

  if (args.Length() < 1 || !args[0]->IsInt32())
    return NanThrowError("incorrect arguments passed to remove(int fd)");

  int err = epoll->Remove(args[0]->Int32Value());
  if (err != 0)
    return NanThrowError(strerror(err), err);

  NanReturnValue(args.This());
}


NAN_METHOD(Epoll::Close) {
  NanScope();

  Epoll *epoll = ObjectWrap::Unwrap<Epoll>(args.This());

  if (epoll->closed_)
    return NanThrowError("close can't be called more than once");

  int err = epoll->Close();
  if (err != 0)
    return NanThrowError(strerror(err), err);

  NanReturnNull();
}


NAN_GETTER(Epoll::GetClosed) {
  NanScope();

  Epoll *epoll = ObjectWrap::Unwrap<Epoll>(args.This());

  NanReturnValue(v8::Boolean::New(epoll->closed_));
}


int Epoll::Add(int fd, uint32_t events) {
  struct epoll_event event;
  event.events = events;
  event.data.fd = fd;

  if (epoll_ctl(epfd_g, EPOLL_CTL_ADD, fd, &event) == -1)
    return errno;

  fd2epoll.insert(std::pair<int, Epoll*>(fd, this));
  fds_.push_back(fd);

  // Keep event loop alive. uv_unref called in Remove.
  uv_ref((uv_handle_t *) &async_g);

  // Prevent GC for this instance. Unref called in Remove.
  Ref();

  return 0;
}


int Epoll::Modify(int fd, uint32_t events) {
  struct epoll_event event;
  event.events = events;
  event.data.fd = fd;

  if (epoll_ctl(epfd_g, EPOLL_CTL_MOD, fd, &event) == -1)
    return errno;

  return 0;
}


int Epoll::Remove(int fd) {
  if (epoll_ctl(epfd_g, EPOLL_CTL_DEL, fd, 0) == -1)
    return errno;

  fd2epoll.erase(fd);
  fds_.remove(fd);

  if (fd2epoll.empty())
    uv_unref((uv_handle_t *) &async_g);
  Unref();

  return 0;
}


int Epoll::Close() {
  closed_ = true;

  delete callback_;
  callback_ = 0;
  
  std::list<int>::iterator it = fds_.begin();
  for (; it != fds_.end(); it = fds_.begin()) {
    int err = Remove(*it);
    if (err != 0)
      return err; // TODO - Returning here may leave things messed up.
  }

  return 0;
}


void Epoll::DispatchEvent(uv_async_t* handle, int status) {
  // This method is executed in the event loop thread.
  // By the time flow of control arrives here the original Epoll instance that
  // registered interest in the event may no longer have this interest. If
  // this is the case, the event will be silently ignored.

  std::map<int, Epoll*>::iterator it = fd2epoll.find(event_g.data.fd);
  if (it != fd2epoll.end()) {
    it->second->DispatchEvent(errno_g, &event_g);
  }

  uv_sem_post(&sem_g);
}


void Epoll::DispatchEvent(int err, struct epoll_event *event) {
  NanScope();

  if (err) {
    v8::Local<v8::Value> args[1] = {
      v8::Exception::Error(v8::String::New(strerror(err)))
    };
    callback_->Call(1, args);
  } else {
    v8::Local<v8::Value> args[3] = {
      NanNewLocal<v8::Value>(v8::Null()),
      v8::Integer::New(event->data.fd),
      v8::Integer::New(event->events)
    };
    callback_->Call(3, args);
  }
}


extern "C" void Init(v8::Handle<v8::Object> exports) {
  NanScope();

  Epoll::Init(exports);
  
  // TODO - Is it a good idea to throw an exception here?
  if (int err = start_watcher())
    NanThrowError(strerror(err), err);
}

NODE_MODULE(epoll, Init)

#endif

