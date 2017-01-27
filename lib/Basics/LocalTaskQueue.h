////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Daniel H. Larkin
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_BASICS_LOCAL_TASK_QUEUE_H
#define ARANGODB_BASICS_LOCAL_TASK_QUEUE_H 1

#include <memory>
#include <queue>

#include "Basics/Common.h"
#include "Basics/ConditionVariable.h"
#include "Basics/Mutex.h"
#include "Basics/asio-helper.h"

namespace arangodb {
namespace basics {

class LocalTaskQueue;

class LocalTask : public std::enable_shared_from_this<LocalTask> {
 public:
  LocalTask() = delete;
  LocalTask(LocalTask const&) = delete;
  LocalTask& operator=(LocalTask const&) = delete;

  explicit LocalTask(LocalTaskQueue* queue);
  virtual ~LocalTask() {}

  virtual void run() = 0;
  void dispatch();

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief the underlying queue
  //////////////////////////////////////////////////////////////////////////////

  LocalTaskQueue* _queue;
};

class LocalCallbackTask
    : public std::enable_shared_from_this<LocalCallbackTask> {
 public:
  LocalCallbackTask() = delete;
  LocalCallbackTask(LocalCallbackTask const&) = delete;
  LocalCallbackTask& operator=(LocalCallbackTask const&) = delete;

  explicit LocalCallbackTask(LocalTaskQueue* queue, std::function<void()> cb);
  virtual ~LocalCallbackTask() {}

  virtual void run();
  void dispatch();

 protected:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief the underlying queue
  //////////////////////////////////////////////////////////////////////////////

  LocalTaskQueue* _queue;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief the callback executed by run() (any exceptions will be caught and
  /// ignored; must not call queue->setStatus())
  //////////////////////////////////////////////////////////////////////////////

  std::function<void()> _cb;
};

class LocalTaskQueue {
 public:
  LocalTaskQueue(LocalTaskQueue const&) = delete;
  LocalTaskQueue& operator=(LocalTaskQueue const&) = delete;

  explicit LocalTaskQueue(boost::asio::io_service*);

  ~LocalTaskQueue();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief exposes underlying io_service
  //////////////////////////////////////////////////////////////////////////////

  boost::asio::io_service* ioService();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief enqueue a task to be run
  //////////////////////////////////////////////////////////////////////////////

  void enqueue(std::shared_ptr<LocalTask>);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief enqueue a callback task to be run after all normal tasks finish;
  /// useful for cleanup tasks
  //////////////////////////////////////////////////////////////////////////////

  void enqueueCallback(std::shared_ptr<LocalCallbackTask>);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief join a single task. reduces the number of waiting tasks and wakes
  /// up the queues's dispatchAndWait() routine
  //////////////////////////////////////////////////////////////////////////////

  void join();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief dispatch all tasks, including those that are queued while running,
  /// and wait for all tasks to join
  //////////////////////////////////////////////////////////////////////////////

  void dispatchAndWait();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief set status of queue
  //////////////////////////////////////////////////////////////////////////////

  void setStatus(int);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief return overall status of queue tasks
  //////////////////////////////////////////////////////////////////////////////

  int status();

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief io_service to dispatch tasks to
  //////////////////////////////////////////////////////////////////////////////

  boost::asio::io_service* _ioService;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief internal task queue
  //////////////////////////////////////////////////////////////////////////////

  std::queue<std::shared_ptr<LocalTask>> _queue;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief internal task queue
  //////////////////////////////////////////////////////////////////////////////

  std::queue<std::shared_ptr<LocalCallbackTask>> _callbackQueue;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief condition variable
  //////////////////////////////////////////////////////////////////////////////

  arangodb::basics::ConditionVariable _condition;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief internal mutex
  //////////////////////////////////////////////////////////////////////////////

  Mutex _mutex;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief number of dispatched, non-joined tasks
  //////////////////////////////////////////////////////////////////////////////

  size_t _missing;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief overall status of queue tasks
  //////////////////////////////////////////////////////////////////////////////

  int _status;
};

}  // namespace arangodb::basics
}  // namespace arangodb

#endif
