#include <sea_util/sea_spsc_ring_buffer.h>
#include "catch.hpp"

TEST_CASE("SPSCRingBuffer: push and pop single item", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    REQUIRE(buf.Size() == 0);

    REQUIRE(buf.TryPush(42));
    REQUIRE(buf.Size() == 1);

    int val = 0;
    REQUIRE(buf.TryPop(val));
    REQUIRE(val == 42);
    REQUIRE(buf.Size() == 0);
}

TEST_CASE("SPSCRingBuffer: returns false when full", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    REQUIRE(buf.TryPush(1));
    REQUIRE(buf.TryPush(2));
    REQUIRE(buf.TryPush(3));
    REQUIRE(buf.TryPush(4));
    REQUIRE_FALSE(buf.TryPush(5));
}

TEST_CASE("SPSCRingBuffer: returns false when empty", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    int val = 0;
    REQUIRE_FALSE(buf.TryPop(val));
}

TEST_CASE("SPSCRingBuffer: FIFO ordering preserved", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 8> buf;
    for (int i = 0; i < 5; i++) {
        REQUIRE(buf.TryPush(i * 10));
    }
    for (int i = 0; i < 5; i++) {
        int val = -1;
        REQUIRE(buf.TryPop(val));
        REQUIRE(val == i * 10);
    }
}

TEST_CASE("SPSCRingBuffer: wraps around correctly", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 4; i++) {
            REQUIRE(buf.TryPush(round * 100 + i));
        }
        for (int i = 0; i < 4; i++) {
            int val = -1;
            REQUIRE(buf.TryPop(val));
            REQUIRE(val == round * 100 + i);
        }
        REQUIRE(buf.Size() == 0);
    }
}

TEST_CASE("SPSCRingBuffer: Clear empties buffer", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 4> buf;
    buf.TryPush(1);
    buf.TryPush(2);
    REQUIRE(buf.Size() == 2);
    buf.Clear();
    REQUIRE(buf.Size() == 0);
    int val = -1;
    REQUIRE_FALSE(buf.TryPop(val));
}

TEST_CASE("SPSCRingBuffer: Size reports correctly", "[SPSCRingBuffer]") {
    sea::SPSCRingBuffer<int, 8> buf;
    REQUIRE(buf.Size() == 0);
    buf.TryPush(1);
    REQUIRE(buf.Size() == 1);
    buf.TryPush(2);
    REQUIRE(buf.Size() == 2);
    int val;
    buf.TryPop(val);
    REQUIRE(buf.Size() == 1);
}
