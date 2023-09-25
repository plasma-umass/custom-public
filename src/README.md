## Microbenchmark

Here are instructions on running and tuning the `microbenchmark-pages` target.

```bash
# On Windows
% cmake src -B build
% cmake --build build --config Release
% .\build\Release\microbenchmark-pages.exe

# On other platforms
% cmake src -B build -DCMAKE_BUILD_TYPE=Release
% cmake --build build
% .\build\microbenchmark-pages.exe

# TODO: Maybe these can be merged into one, just needs testing.
```

Settings can be set using CMake at configuration time (the first step).

```bash
% cmake src -B build -DMBP_OBJECT_SIZE=32 -DMBP_OBJECT_DISTANCE=4096
```