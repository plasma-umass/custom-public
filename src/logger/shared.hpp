#ifndef LOGGER_DETAIL_SHARED_HPP
#define LOGGER_DETAIL_SHARED_HPP

#include <cstdint>

enum class EventType : std::uint8_t {
    Null,
    Allocation,
    Reallocation,
    Free,
};

struct Event {
    EventType type = EventType::Null;
    std::uint64_t size = 0;
    std::uint64_t pointer = 0;
    std::uint64_t result = 0;
};

#endif // LOGGER_DETAIL_SHARED_HPP
