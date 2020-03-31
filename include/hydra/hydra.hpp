#pragma once



#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include <atomic>
#include <memory>
#include <vector>
#include <thread>
#include <map>
#include <stack>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <utility>



namespace hydra {



struct config {
  static constexpr size_t cacheline_size = 64;
};



struct spinlock {

  spinlock() noexcept = default;
  spinlock(spinlock const&) noexcept = delete;
  spinlock& operator = (spinlock const&) noexcept = delete;


  bool try_lock() noexcept {
    if(flag_.load(std::memory_order_relaxed))
      return false;
    return !flag_.exchange(true, std::memory_order_acquire);
  }


  void unlock() noexcept {
    flag_.store(false, std::memory_order_release);
  }

  void lock() noexcept {
    while(!try_lock())
      std::this_thread::yield();
  }


private:

  alignas(config::cacheline_size) std::atomic_bool flag_{false};

}; // spinlock



struct shared_spinlock {

  shared_spinlock() noexcept = default;
  shared_spinlock(spinlock const&) noexcept = delete;
  shared_spinlock& operator = (spinlock const&) noexcept = delete;



  bool try_lock() noexcept {

    if(data_.writer.load(std::memory_order_relaxed))
      return false;

    if(data_.writer.exchange(true, std::memory_order_acquire))
      return false;

    while(data_.readers.load(std::memory_order_acquire) > 0)
      relax();

    return true;
  }



  void unlock() noexcept {
    data_.writer.store(false, std::memory_order_release);
  }



  void lock() noexcept {
    while(!try_lock())
      std::this_thread::yield();
  }



  bool try_lock_shared() noexcept {

    ++data_.readers;

    if(data_.writer.load(std::memory_order_relaxed) ||
       data_.writer.load(std::memory_order_acquire)) {
      --data_.readers;
      return false;
    }

    return true;
  }



  void unlock_shared() noexcept {
    --data_.readers;
  }



  void lock_shared() noexcept {
    while(!try_lock_shared())
      std::this_thread::yield();
  }

private:

  alignas(config::cacheline_size) struct data {
    std::atomic_bool writer{false};
    std::atomic_uint readers{0};
  } data_;



  static void relax() {

    #if defined(_MSC_VER)

    #if defined(_M_AMD64) || defined(_M_IX86)
      _mm_pause();
    #elif defined(_M_ARM)
      __yield();
    #endif // _M_IX86

    #else

    #if defined(__x86_64__) || defined(__i386__)
      __asm__ __volatile__ ("pause");
    #elif defined(__arm__)
      __asm__ __volatile__ ("yield");
    #endif // __i386__

    #endif // _MSC_VER

  }

}; // shared_spinlock



template<typename T, typename L = shared_spinlock> struct synchronized {

  struct unique_access {

    unique_access() = delete;
    unique_access(unique_access const&) = delete;
    unique_access& operator = (unique_access const&) = delete;

    unique_access(synchronized& owner) noexcept:
      guard_(owner.lock_), resource_(owner.resource_) { }

    T& operator * () const noexcept { return resource_; }
    T* operator -> () const noexcept { return &resource_; }

  private:

    std::unique_lock<L> guard_;
    T& resource_;

  }; // unique_access



  struct shared_access {

    shared_access() = delete;
    shared_access(shared_access const&) = delete;
    shared_access& operator = (shared_access const&) = delete;

    shared_access(synchronized const& owner) noexcept:
      guard_(owner.lock_), resource_(owner.resource_) { }

    T const& operator * () const noexcept { return resource_; }
    T const* operator -> () const noexcept { return &resource_; }

  private:

    std::shared_lock<L> guard_;
    T const& resource_;

  }; // shared_access



  synchronized() = default;
  synchronized(synchronized const&) = delete;
  synchronized& operator = (synchronized const&) = delete;
  synchronized(synchronized&&) = default;
  synchronized& operator = (synchronized&&) = default;

  template<typename... Args>
  synchronized(Args&&... args):
    resource_(std::forward<Args>(args)...) { }

private:

  L lock_;
  T resource_;

}; // synchronized



struct scheduler_traits {
  using lock_type = spinlock;
  using clock_type = std::chrono::system_clock;
  static inline typename clock_type::duration const precision = std::chrono::seconds(1);
}; // scheduler_traits



template<typename Tr = scheduler_traits>
struct scheduler {
  using lock_type = typename Tr::lock_type;
  using clock_type = typename Tr::clock_type;
  using time_point = typename clock_type::time_point;
  using duration = typename clock_type::duration;

  struct task {
    time_point previous;
    duration interval;
    time_point next;
    std::function<void ()> callable;
  }; // task
  typedef std::unique_ptr<task> task_ptr;
  typedef std::multimap<time_point, task_ptr> tasks;
  typedef std::vector<task_ptr> rescheduled_tasks;

  scheduler(): stopping_(false) { }
  scheduler(scheduler const&) = delete;
  scheduler& operator = (scheduler const&) = delete;
  ~scheduler() { stop(); }

  template<typename F>
  bool every(duration const& interval, F&& f) {
    time_point const now = clock_type::now();
    f();
    time_point const next = now + interval;
    using namespace std;
    task_ptr tp = make_unique<task>(task{now, interval, next, forward<F>(f)});
    unique_lock<lock_type> g(tasks_lock_);
    tasks_.insert({ next, move(tp) });
    return true;
  }

  template<typename F>
  bool daily(int hour, int minute, int second, F&& f) {
    using namespace std;
    time_t const now = time(nullptr);
    tm current;
#if _WIN32
    if(!gmtime_s(&current, &now))
#else
    if(!gmtime_r(&now, &current))
#endif
      return false;

    current.tm_hour = hour;
    current.tm_min = minute;
    current.tm_sec = second;
    time_t const today = mktime(&current);
    if(today == -1)
      return false;
    if(now == today)
      f();
    duration const interval = chrono::hours(24);
    time_point const today_point = clock_type::from_time_t(today);
    time_point const next = (now < today) ? today_point : today_point + interval;
    task_ptr tp = make_unique<task>({time_point(), interval, next, forward<F>(f)});
    unique_lock<lock_type> g(tasks_lock_);
    tasks_.insert(next, move(tp));
    return true;
  }

  void schedule() {
    time_point const scheduled = clock_type::now();
    std::unique_lock<lock_type> g(tasks_lock_);
    if(tasks_.empty()) return;
    auto last = tasks_.upper_bound(scheduled);
    for(auto it = tasks_.begin(); it != last; ++it) {
      time_point const now = clock_type::now();
      it->second->callable();
      it->second->previous = now;
      time_point const next = scheduled + it->second->interval;
      it->second->next = next;
      rescheduled_tasks_.push_back(std::move(it->second));
    }
    tasks_.erase(tasks_.begin(), last);
    for(auto& rescheduled: rescheduled_tasks_)
      tasks_.insert({rescheduled->next, std::move(rescheduled)});
    rescheduled_tasks_.clear();
  }

  void run() {
    using namespace std;
    if(worker_.joinable())
      return;
    worker_ = thread([&]() {
      while(!stopping_) {
        unique_lock<mutex> g(cv_mutex_);
        auto const r = cv_.wait_for(g, Tr::precision);
        if(r != cv_status::timeout)
          continue;
        schedule();
      }
      stopping_ = false;
    });
  }

  void stop() {
    if(!worker_.joinable() || stopping_)
      return;
    stopping_ = true;
    cv_.notify_one();
    worker_.join();
  }

private:
  std::atomic<bool> stopping_;
  std::condition_variable cv_;
  std::mutex cv_mutex_;
  std::thread worker_;
  tasks tasks_;
  lock_type tasks_lock_;
  rescheduled_tasks rescheduled_tasks_;
}; // scheduler



template<typename T, typename C = std::stack<std::unique_ptr<T>>> struct pool {
  using container_type = C;
  using size_type = typename C::size_type;
  using pointer_type = typename C::value_type;

  pool() = default;
  pool(pool const&) = default;
  pool& operator = (pool const&) = default;
  pool(pool&&) = default;
  pool& operator = (pool&&) = default;

  void give_back(pointer_type obj) { return objects_.push(std::move(obj)); }
  size_type size() const { return objects_.size(); }
  bool empty() const { return objects_.empty(); }
  void clear() { objects_ = C(); }

  pointer_type lease() {
    pointer_type r;
    if (objects_.empty())
      return std::move(r = pointer_type{new T});
    r = std::move(objects_.top());
    objects_.pop();
    return r;
  }

private:
  C objects_;
}; // pool



template<typename M, typename L = spinlock>
struct activity {
  using lock_type = L;
  using pool_type = pool<M>;
  using message_type = M;
  using pointer_type = typename pool_type::pointer_type;
  using incomings_type = synchronized<std::vector<pointer_type, L>>;
  using processings_type = std::vector<pointer_type>;

  activity() noexcept = default;
  activity(activity const&) noexcept = delete;
  activity& operator = (activity const&) noexcept = delete;
  ~activity() { stop(); }
  pointer_type lease() { return pool_.lease(); }
  bool active() const noexcept { return worker_.joinable(); }

  bool enqueue(pointer_type&& m) {
    if(!m) return false;
    if(!worker_.joinable()) return false;
    incomings_.push_back(std::move(m));
    new_message_.notify_one();
    return true;
  }

  void stop() noexcept {
    if(!worker_.joinable()) return;
    stopped_ = true;
    new_message_.notify_one();
    worker_.join();
  }

  template<typename H>
  bool run(H&& handler) {
    if(worker_.joinable()) return false;
    if(stopped_) return false;
    worker_ = std::thread([&]() {
      H inner = std::move(handler);
      while(!stopped_) {
        if(swap_messages())
          process(inner);
        else {
          std::unique_lock<std::mutex> g(new_message_mutex_);
          new_message_.wait(g);
        }
      }
      if(swap_messages())
        process(inner);
      stopped_ = false;
    });
    return true;
  }


private:
  pool_type pool_;
  incomings_type incomings_;
  processings_type processings_;
  std::thread worker_;
  std::condition_variable new_message_;
  std::mutex new_message_mutex_;
  std::atomic<bool> stopped_{false};

  bool swap_messages() {
    typename incomings_type::unique_access g(incomings_);
    if(g->empty()) return false;
    std::swap(*g, processings_);
    return true;
  }

  template<typename H>
  void process(H& handler) {
    for(pointer_type& each: processings_) {
      handler(*each);
      pool_.give_back(std::move(each));
    }
    processings_.clear();
  }
}; // activity



} // hydra
