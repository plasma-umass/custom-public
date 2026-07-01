## 1. Get the sources

```
git clone https://github.com/rust-lang/rust.git --branch 1.96.0 rust-arena
cp bootstrap.toml rust-arena

git clone https://github.com/rust-lang/rust.git --branch 1.96.0 rust-malloc
cp bootstrap.toml rust-malloc
cd rust-malloc && patch -p1 -i ../patch && cd ..
```

## 2. Build the toolchain

```bash
cd rust-arena
python3 x.py build --stage 2
cd ..

cd rust-malloc
python3 x.py build --stage 2
cd ..
```

The optimized compiler ends up at `build/host/stage2/bin/rustc`.

## 3. Generate a large Rust file

```bash
cargo install bindgen-cli

mkdir bench && cd bench

# Alternative 1: SQLite
wget https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip \
    && echo "e7eb4cfb2d95626e782cfa748f534c74482f2c3c93f13ee828b9187ce05b2da7  sqlite-amalgamation-3490100.zip" | sha3sum -a 256 -c - \
    && unzip -p sqlite-amalgamation-3490100.zip sqlite-amalgamation-3490100/sqlite3.h > sqlite3.h \
    && rm sqlite-amalgamation-3490100.zip
bindgen sqlite3.h --use-core -o b_sqlite.rs
sed -i '1s/^/#![allow(non_upper_case_globals, non_camel_case_types, non_snake_case, dead_code, improper_ctypes, unsafe_op_in_unsafe_fn)]\n/' b_sqlite.rs

# Alternative 2: Vulkan
bindgen /usr/include/vulkan/vulkan_core.h --use-core -o b_vulkan.rs
sed -i '1s/^/#![allow(non_upper_case_globals, non_camel_case_types, non_snake_case, dead_code, improper_ctypes, unsafe_op_in_unsafe_fn)]\n/' b_vulkan.rs
```

## 4. Benchmark

```bash
./rust-arena/build/host/stage2/bin/rustc b_sqlite.rs \
  --edition 2021 --crate-type lib -Cdebuginfo=0 -Ccodegen-units=1

./rust-malloc/build/host/stage2/bin/rustc b_vulkan.rs \
  --edition 2021 --crate-type lib -Cdebuginfo=0 -Ccodegen-units=1
```
