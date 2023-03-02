CXX := clang++
CXXFLAGS := --std=c++17 -fPIC -O3 -fvisibility=hidden -Wall -Wextra -Werror
LDFLAGS := -fPIC -shared
ARFLAGS := -crs

BUILD_DIRECTORY := build

.PHONY: all
all: $(BUILD_DIRECTORY)/libdetector.so $(BUILD_DIRECTORY)/liblitterer.so $(BUILD_DIRECTORY)/liblitterer.a

.PHONY: clean
clean:
	rm -rf $(BUILD_DIRECTORY)

$(BUILD_DIRECTORY)/libdetector.so: detector.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) -o $@ -ldl

$(BUILD_DIRECTORY)/litterer.o: litterer.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) -c -o $@
$(BUILD_DIRECTORY)/liblitterer.so: $(BUILD_DIRECTORY)/litterer.o
	$(CXX) $< $(LDFLAGS) -o $@
$(BUILD_DIRECTORY)/liblitterer.a: $(BUILD_DIRECTORY)/litterer.o
	$(AR) $(ARFLAGS) $@ $<

.PHONY: format
format:
	clang-format -i constants.hpp detector.cpp litterer.cpp
