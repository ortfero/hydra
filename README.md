# hydra

C++20 header-only library for passing message to threads

## Installation

Drop contents of `include` at your include path


## Tests

hydra uses [just](https://github.com/casey/just) to build tests:

```shell
cd hydra
just test
```

## Interface

```cpp
namespace hydra {

    template<typename M, typename Q = mpsc_queue<M>>
    class activity {
    public:
        using message_type = M;
        using queue_type = Q;
        using size_type = typename Q::size_type;
        using batch_type = batch<Q>;

        activity() noexcept;
        activity(activity const&) noexcept;
        activity& operator=(activity const&) noexcept;
        bool active() const noexcept;
        sequence claim() noexcept;
        message_type& operator[](sequence n) noexcept;
        void reserve(size_type n) noexcept;
        size_type blocks_count() const noexcept;
        template<typename Rep, typename Period> sequence claim_for(std::chrono::duration<Rep, Period> const& duration) noexcept;
        void publish(sequence n) noexcept;
        void stop() noexcept;
        template<typename H> bool run(H&& handler);
    }; // activity

    
    template<typename Q>
    class batch {
    public:
        using size_type = typename Q::size_type;
        using value_type = typename Q::value_type;
        batch() = delete;
        batch(const batch&) = delete;
        batch& operator=(const batch&) = delete;
        batch(Q& queue) noexcept;

        size_type size() const noexcept;
        sequence try_fetch();
        value_type& operator[](sequence n);
        std::uint32_t fetched_count() const noexcept;
        void fetched();
    };   // batch

} // namespace hydra
```


## Usage

### Simple message

```cpp
#include <hydra/activity.hpp>


struct message {
  int value;
}; // message


class consumer {
private:
  constexpr auto queue_size = 65536;
  
  using activity = hydra::activity<message>;

  activity activity_;

public:

  consumer() = default;
  consumer(consumer const&) = delete;
  consumer& operator = (consumer const&) = delete;
  consumer(consumer&&) = default;
  consumer& operator = (consumer&&) = default;

  bool run() {
    activity_.reserve(queue_size);
    return activity_.run([this](auto& messages) {
      while(auto sequence = messages.try_fetch()) {
        process(messages[sequence]);
        messages.fetched();
      }  
    });
  }

  void publish(int value) {
    auto const sequence = activity_.claim();
    activity_[n].value = value;
    activity_.publish(sequence);
  }

private:

  void process(message const& m) {
    std::printf("message: %d\n", m.value);
  }
}; // consumer


void main() {
  auto c = consumer{};
  if(!c.run())
    return;
  for(auto i = 0u; i != 10000; ++i) {
    c.publish(i);
  }
}
  
```

## Dependencies

* [usync](https://github.com/ortfero/usync)


## License

hydra licensed under [MIT license](https://opensource.org/licenses/MIT).

