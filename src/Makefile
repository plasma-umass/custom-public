CXX := clang++
CXXFLAGS := --std=c++17 -fPIC -O3 -DNDEBUG -fvisibility=hidden -Wall -Wextra -Werror
LDFLAGS := -fPIC -shared

BUILD_DIRECTORY := build

.PHONY: all
all: $(BUILD_DIRECTORY)/libdetector.so $(BUILD_DIRECTORY)/liblitterer.so

.PHONY: clean
clean:
	rm -rf $(BUILD_DIRECTORY)

$(BUILD_DIRECTORY)/libdetector.so: detector.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) -o $@ -ldl

ifdef OUTPUT_PERF_DATA
$(BUILD_DIRECTORY)/liblitterer.so: litterer.cpp vendor/libpfm4
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) -Ivendor/libpfm4/include -DOUTPUT_PERF_DATA $(EXTRAFLAGS) $(LDFLAGS) -Lvendor/libpfm4/lib -o $@ -ldl -lpfm
else
$(BUILD_DIRECTORY)/liblitterer.so: litterer.cpp
	mkdir -p $(BUILD_DIRECTORY)
	$(CXX) $< $(CXXFLAGS) $(EXTRAFLAGS) $(LDFLAGS) -o $@ -ldl
endif

vendor/libpfm4:
	mkdir vendor
	git clone https://github.com/wcohen/libpfm4 $@
	cd $@ && make

.PHONY: format
format:
	clang-format -i constants.hpp detector.cpp litterer.cpp PFMWrapper.hpp
