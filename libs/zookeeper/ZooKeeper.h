//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef EBBRT_ZOOKEEPER_H_
#define EBBRT_ZOOKEEPER_H_

#include <cstdio>
#include <ebbrt/Debug.h>
#include <ebbrt/Future.h>
#ifndef __ebbrt__
#include <ebbrt/hosted/NodeAllocator.h>
#else
#include <ebbrt/native/Multiboot.h>
#endif
#include <ebbrt/SharedEbb.h>
#include <ebbrt/SharedIOBufRef.h>
#include <ebbrt/SpinLock.h>
#include <ebbrt/Timer.h>
#include <poll.h>
#include <string>

#include "zookeeper/include/zookeeper.h"

namespace ebbrt {

constexpr int ZkConnectionTimeoutMs = 60000;
constexpr int ZkIoEventTimer = 1000;

class ZooKeeper : public ebbrt::Timer::Hook {
 public:
  enum struct Event : int {  // event values declared in
                             // zookeeper/src/zk_adaptor.h
    Nil = 0,
    Create = 1,  // ZOO_CREATED_EVENT,
    Delete,  // ZOO_DELETE_EVENT,
    Change,  // ZOO_CHANGED_EVENT,
    ChildChange,  // ZOO_CHILD_EVENT,
    Session = -1,  // ZOO_SESSION_EVENT,
    Error = -4  // ZOO_NOTWATCHING_EVENT
  };
  enum struct ConnectionStatus : int {
    Nil = 0,
    Expired = -112,  // ZOO_EXPIRED_SESSION_STATE,
    AuthFailed = -113,  // ZOO_AUTH_FAILED_STATE,
    Conneting = 1,  // ZOO_CONNECTING_STATE,
    Associating = 2,  // ZOO_ASSOCIATING_STATE,
    Connected = 3,  // ZOO_CONNECTED_STATE
    NotConnected = 999,  // NOTCONNECTED_STATE_DEF
  };
  enum struct Flag : int {
    Nil = 0,
    Ephemeral = 0x01,  // ZOO_EPHEMERAL,
    Sequenced = 0x10,  // ZOO_SEQUENCED,
    SequencedEphemeral = 0x11  // (ZOO_EPHEMERAL | ZOO_SEQUENCED)
  };

  typedef struct Stat ZkStat;
  /* Contents of the ZkStat structure:
      int64_t czxid;
      int64_t mzxid;
      int64_t ctime;
      int64_t mtime;
      int32_t version;
      int32_t cversion;
      int32_t aversion;
      int64_t ephemeralOwner;
      int32_t dataLength;
      int32_t numChildren;
      int64_t pzxid; */
  struct Znode {
    int err;
    std::string value;
    ZkStat stat;
  };
  struct ZnodeChildren {
    int err;
    std::vector<std::string> values;
    ZkStat stat;
  };

  /* Watcher is the base callback type for 'watch' events handlers
   */
  struct Watcher {
    virtual ~Watcher() {}
    virtual void WatchHandler(int type, int state, const char* path) = 0;
  };

  struct ConnectionWatcher : public ebbrt::ZooKeeper::Watcher {
    virtual void OnConnected(){};
    virtual void OnConnecting(){};
    virtual void OnSessionExpired(){};
    virtual void OnAuthFailed(){};
    virtual void OnAssociating(){};
    void WatchHandler(int type, int state, const char* path) override {
      if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_EXPIRED_SESSION_STATE) {
          OnSessionExpired();
        } else if (state == ZOO_CONNECTED_STATE) {
          OnConnected();
        } else if (state == ZOO_CONNECTING_STATE) {
          OnConnecting();
        } else if (state == ZOO_ASSOCIATING_STATE) {
          OnAssociating();
        } else if (state == ZOO_AUTH_FAILED_STATE) {
          OnAuthFailed();
        } else {
          ebbrt::kabort("ConnectionWatcher: Unsupported session state");
        }
      } else {
        ebbrt::kabort("ConnectionWatcher: Unsupported event type");
      }
    }
  };

  /* PathWatcher watchs are set through zoo_exist() */
  struct PathWatcher : public ebbrt::ZooKeeper::Watcher {
    virtual void OnCreated(const char* path){};
    virtual void OnDeleted(const char* path){};
    virtual void OnChanged(const char* path){};
    virtual void OnNotWatching(const char* path){};
    void WatchHandler(int type, int state, const char* path) override {
      if (type == ZOO_CREATED_EVENT) {
        OnCreated(path);
      } else if (type == ZOO_DELETED_EVENT) {
        OnDeleted(path);
      } else if (type == ZOO_CHANGED_EVENT) {
        OnChanged(path);
      } else if (type == ZOO_NOTWATCHING_EVENT) {
        OnNotWatching(path);
      } else {
        ebbrt::kabort("PathWatcher: Unsupported event type");
      }
    }
  };

  /* ChildWatcher watchs are set through zoo_get_children() */
  struct ChildWatcher : public ebbrt::ZooKeeper::Watcher {
    virtual void OnChildChanged(const char* path){};
    virtual void OnNotWatching(const char* path){};
    void WatchHandler(int type, int state, const char* path) override {
      if (type == ZOO_CREATED_EVENT) {
        if (type == ZOO_CHILD_EVENT) {
          OnChildChanged(path);
        } else if (type == ZOO_NOTWATCHING_EVENT) {
          OnNotWatching(path);
        } else {
          ebbrt::kabort("ChildWatcher: Unsupported event type");
        }
      }
    }
  };

  class WatchEvent : public ebbrt::ZooKeeper::Watcher {
   public:
    WatchEvent(int type, ebbrt::MovableFunction<void()> func)
        : type_(type), func_(std::move(func)){};
    void WatchHandler(int type, int state, const char* path) override {
      if (type == type_)
        event_manager->Spawn(std::move(func_));
      else {
        ebbrt::kabort(
            "WatchEvent for non-specified type: %d (configured: %d)\n", type,
            type_);
      }
    }

   private:
    int type_;
    ebbrt::MovableFunction<void()> func_;
  };

  static EbbRef<ZooKeeper> Create(EbbId id,
                                  Watcher* connection_watcher = nullptr,
                                  int timeout_ms = ZkConnectionTimeoutMs,
                                  int timer_ms = ZkIoEventTimer,
                                  std::string server_hosts = std::string());

  static ZooKeeper& HandleFault(EbbId id) {
    LocalIdMap::ConstAccessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (!found)
      throw std::runtime_error("Failed to find root for ZooKeeper Ebb");

    auto rep = boost::any_cast<ZooKeeper*>(accessor->second);
    EbbRef<ZooKeeper>::CacheRef(id, *rep);
    return *rep;
  }

  void Fire() override;
  ~ZooKeeper();
  // disable copies
  ZooKeeper(const ZooKeeper&) = delete;
  ZooKeeper& operator=(const ZooKeeper&) = delete;
  Future<Znode> New(const std::string& path,
                    const std::string& value = std::string(),
                    ZooKeeper::Flag flags = ZooKeeper::Flag::Nil);
  Future<bool> Exists(const std::string& path, Watcher* watch = nullptr);
  Future<Znode> Get(const std::string& path, Watcher* watch = nullptr);
  Future<ZnodeChildren> GetChildren(const std::string& path,
                                    Watcher* watch = nullptr);
  // Future<std::string> GetValue(const std::string& path,
  //                             Watcher* watch = nullptr);
  Future<Znode> Delete(const std::string& path, int version = -1);
  Future<Znode> Set(const std::string& path, const std::string& value,
                    int version = -1);
  Future<Znode> Stat(const std::string& path, Watcher* watch = nullptr);

  void CLI(char* line);
  static void PrintZnode(Znode* zkr);
  static void PrintZnodeChildren(ZnodeChildren* zkcr);

 private:
  ZooKeeper(ebbrt::ZooKeeper::Watcher* connection_watcher, int timeout_ms,
            int timer_ms);
  ZooKeeper(const std::string& server_hosts,
            ebbrt::ZooKeeper::Watcher* connection_watcher, int timeout_ms,
            int timer_ms);
  static void process_watch_event(zhandle_t*, int type, int state,
                                  const char* path, void* ctx);
  static void data_completion(int rc, const char* value, int value_len,
                              const ZkStat* stat, const void* data);
  static void stat_completion(int rc, const ZkStat* stat, const void* data);
  static void string_completion(int rc, const char* name, const void* data);
  static void strings_completion(int rc, const struct String_vector* strings,
                                 const ZkStat* stat, const void* data);
  static void void_completion(int rc, const void* data);
  static int startsWith(const char* line, const char* prefix);
  static void print_stat(ZkStat* stat);
  SpinLock lock_;
  zhandle_t* zk_ = nullptr;
  Watcher* connection_watcher_ = nullptr;
};  // end ebbrt::ZooKeeper
}  // end namespace
#endif  // EBBRT_ZOOKEEPER_H
