#include "tbuf.hpp"
#include <cstdio>
#include <thread>

struct data {
    using data_t = int;
    data_t a;
    data_t b;
    data_t c;
};

tbuf<data> buffer{};

void wait () {
    std::printf("Waiting for input...");
    std::getchar();
}

void slow_test () {
    auto read_one = [](){
        puts("Reading");
        auto& r = buffer.consumer_read_buffer();
        std::printf("Read:{%i,%i,%i}\n",r.a, r.b, r.c);
    };
    auto write_one = [](int n){
        puts("Writing");
        auto& w = buffer.producer_write_buffer();
        w.a = n;
        w.b = n+1;
        w.c = n+2;
        buffer.producer_flip ();
        std::printf("Write:{%i,%i,%i}\n",w.a, w.b, w.c);
    };

    for (data::data_t n=0;; n += 3) {
        write_one (n);
        wait ();
        read_one ();
        wait ();
        write_one (n);
        wait ();
        write_one (n);
        wait ();
        read_one ();
        wait ();
        read_one ();
        wait ();
    }
}

void producer ()
{
    for (data::data_t n=0;; n += 3) {
        auto& w = buffer.producer_write_buffer();
        
        w.a = n;
        w.b = n+1;
        w.c = n+2;

        buffer.producer_flip();
    }
}

void consumer ()
{
    data prev{};
    int success = 0;
    for (;;) {
        int fail = 0;
        const auto& r = buffer.consumer_read_buffer();

        if (r.a +1 != r.b) {
            fail |= 1;
        }

        if (r.b +1 != r.c) {
            fail |= 2;
        }

        if (prev.a > r.a) {
            fail |= 4;
        }

        if (fail) {
            std::printf("Sync error: prev={%i,%i,%i}, recv={%i,%i,%i}, fail=%x, success=%i\n",
                prev.a,prev.b,prev.c,
                r.a,r.b,r.c,
                fail, success);
            success = 0;
        } else {
            success++;
        }

        prev = r;
    }
}

void speed_sync_test () {
    std::thread p{producer};
    std::thread c{consumer};

    p.join(); 
    c.join();
}

int main() {
    speed_sync_test();
    slow_test();
}