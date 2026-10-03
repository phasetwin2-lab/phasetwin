#pragma once
#include <array>
#include <atomic>
#include <cstdint>
namespace phasetwin {
// Four stereo traces: input A, input B, corrected A, latency-matched B.
constexpr int scopePacketSamples = 256;
struct ScopePacket {
    std::array<std::array<float, scopePacketSamples>, 8> traces{};
    double sampleRate = 48000;
    std::uint64_t firstSample = 0;
    std::uint32_t generation = 0;
};
// Exactly one audio-thread producer and one editor-thread consumer.
// A full queue drops visualization packets; audio processing never waits.
class ScopeQueue {
    static_assert(std::atomic<unsigned>::is_always_lock_free, "Scope transport needs lock-free index atomics");
    static constexpr unsigned capacity = 64;
    std::array<ScopePacket, capacity> packets{};
    std::atomic<unsigned> write{0}, read{0};
public:
    bool push(const ScopePacket& packet) noexcept {
        auto w = write.load(std::memory_order_relaxed);
        auto next = (w + 1) % capacity;
        if (next == read.load(std::memory_order_acquire)) return false;
        packets[w] = packet;
        write.store(next, std::memory_order_release);
        return true;
    }
    bool pop(ScopePacket& packet) noexcept {
        auto r = read.load(std::memory_order_relaxed);
        if (r == write.load(std::memory_order_acquire)) return false;
        packet = packets[r];
        read.store((r + 1) % capacity, std::memory_order_release);
        return true;
    }
};
}
