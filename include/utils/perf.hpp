#ifndef UTILS_PERF_HPP
#define UTILS_PERF_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace utils::perf {
std::string toString(std::uint32_t type, std::uint64_t config);
std::string toString(std::pair<std::uint32_t, std::uint64_t> event);

struct Group {
    Group(const std::vector<std::pair<std::uint32_t, std::uint64_t>>& events);
    ~Group();

    Group(const Group&) = delete;
    Group& operator=(const Group&) = delete;

    Group(Group&&) = default;
    Group& operator=(Group&&) = default;

    void enable();
    void disable();
    void reset();

    [[nodiscard]] bool isEnabled() const;

    [[nodiscard]] std::vector<std::uint64_t> read() const;

  private:
    std::vector<int> descriptors;
    bool enabled = false;
};
} // namespace utils::perf

#endif
