#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <cstdint>
#include <mutex>

template <typename T>
class tbuf {
    std::array<T, 3> buffer;

    std::atomic_uint32_t state;

    struct state_expanded {
        std::uint8_t write;
        std::uint8_t latest;
        std::uint8_t read;
        std::uint8_t other;
    };

    state_expanded get_state () const noexcept {
        return std::bit_cast<state_expanded> (state.load());
    }
    void set_state (state_expanded s) noexcept {
        state.store(std::bit_cast<std::uint32_t> (s));
    }
    std::mutex mut;

public:
    tbuf () {
        set_state({.write=0, .latest=1, .read=1, .other=2});
    }
    tbuf (T first, T second, T third) :
        buffer({std::move(first), std::move(second), std::move(third)})
    {
        set_state({.write=0, .latest=1, .read=1, .other=2});
    }

    T& producer_write_buffer () noexcept {
        return buffer[get_state().write];
    }
    const T& producer_latest_buffer () noexcept {
        return buffer[get_state().latest];
    }

    void producer_flip () {
        std::lock_guard lock{mut};

        auto prev = get_state ();
        auto next = state_expanded ();

        next.read = prev.read;
        next.latest = prev.write;

        if (prev.latest == prev.read) {
            // Currently reading from 'latest'
            next.write = prev.other;
            next.other = prev.latest;
        } else {
            // Currently reading from 'other'
            next.write = prev.latest;
            next.other = prev.other;
        }

        set_state (next);
    }

    const T& consumer_read_buffer () {
        std::lock_guard lock{mut};

        auto prev = get_state ();
        auto next = state_expanded ();

        next.write = prev.write;
        next.other = prev.other;
        next.latest = prev.latest;

        next.read = prev.latest;

        set_state (next);
        return buffer[next.read];
    }
};