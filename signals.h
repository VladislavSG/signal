#pragma once
#include <functional>
#include "intrusive_list.h"

namespace signals
{

template <typename T>
struct signal;

template <typename... Args>
struct signal<void (Args...)>
{
  using slot_t = std::function<void(Args...)>;

  struct connection : intrusive::list_element<struct connection_tag> {
    connection() noexcept
        : sig(nullptr), slot({}) {};
    connection(signal* sig, slot_t slot) noexcept
        : sig(sig)
        , slot(std::move(slot)) {
      sig->connections.push_back(*this);
    }

    connection(connection const&) = delete;
    connection& operator=(connection const&) = delete;

    connection(connection&& other) noexcept
        : sig(other.sig)
        , slot(std::move(other.slot)) {
      move(other);
    };
    connection& operator=(connection&& other) noexcept {
      if (this != &other) {
        disconnect();
        sig = other.sig;
        slot = std::move(other.slot);
        move(other);
      }
      return *this;
    };

    void disconnect() noexcept {
      if (sig == nullptr)
        return;
      for (iteration_token* tok = sig->top_token; tok != nullptr; tok = tok->next) {
        if (&*tok->it == this) {
          ++tok->it;
        }
      }
      unlink();
      sig = nullptr;
      slot = {};
    };

    ~connection() {
      disconnect();
    }

    friend struct signal;

  private:
    void move(connection& other) {
      if (sig != nullptr) {
        sig->connections.insert(sig->connections.as_iterator(other), *this);
        other.unlink();
        for (iteration_token* tok = sig->top_token; tok != nullptr; tok = tok->next) {
          if (tok->it != sig->connections.end() && &*tok->it == &other) {
            tok->it = sig->connections.as_iterator(*this);
          }
        }
      }
    }

    signal* sig;
    slot_t slot;
  };

  using connections_t = intrusive::list<connection, struct connection_tag>;

  struct iteration_token {
    explicit iteration_token(signal const* sig) noexcept
        : sig(sig), next(sig->top_token)
    {
      sig->top_token = this;
    };

    iteration_token(const iteration_token&) = delete;
    iteration_token& operator=(const iteration_token&) = delete;
    ~iteration_token() {
      if (sig != nullptr)
        sig->top_token = sig->top_token->next;
    }

  private:
    signal const* sig;
    typename connections_t::const_iterator it;

    iteration_token* next;

    friend struct signal;
  };

  signal() = default;

  signal(signal const&) = delete;
  signal& operator=(signal const&) = delete;

  ~signal() {
    for (iteration_token* tok = top_token; tok != nullptr; tok = tok->next) {
      tok->sig = nullptr;
    }
    for (auto it = ++connections.begin(); it != ++connections.end(); ++it) {
      auto copy = it;
      (--copy)->disconnect();
    }
  };

  connection connect(std::function<void (Args...)> slot) noexcept {
    return signals::signal<void(Args...)>::connection(this, std::move(slot));
  };

  void operator()(Args...args) const {
    iteration_token tok(this);
    tok.it = connections.begin();
    while (tok.it != connections.end()) {
      auto copy = tok.it;
      ++tok.it;
      copy->slot(args...);
      if (tok.sig == nullptr) {
        return;
      }
    }
  };


private:
  connections_t connections;
  mutable iteration_token* top_token = nullptr;
};

}