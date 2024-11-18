LLVM_HASH := ab51eccf88f5321e7c60591c5546b254b6afab99 # llvmorg-19.1.3

llvm/src-vanilla:
	rm -rf $@
	git clone https://github.com/llvm/llvm-project.git $@
	cd $@ && git checkout $(LLVM_HASH)

llvm/build-vanilla: llvm/src-vanilla
	cmake $</llvm -B $@ -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DLLVM_ENABLE_PROJECTS="clang"
	ninja -C $@ -j64

llvm/vanilla: llvm/build-vanilla
	cmake --install $< --prefix $@


llvm/src-shim: llvm/shim.diff
	rm -rf $@
	git clone https://github.com/llvm/llvm-project.git $@
	cd $@ && git checkout $(LLVM_HASH)
	cd $@ && git apply ../shim.diff

llvm/build-shim: llvm/src-shim
	cmake $</llvm -B $@ -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DLLVM_ENABLE_PROJECTS="clang"
	ninja -C $@ -j64

llvm/shim: llvm/build-shim
	cmake --install $< --prefix $@


llvm/sqlite3.c:
	wget https://www.sqlite.org/2024/sqlite-amalgamation-3470000.zip
	unzip sqlite-amalgamation-3470000.zip
	mv sqlite-amalgamation-3470000/sqlite3.c llvm
	rm -rf sqlite-amalgamation-3470000.zip sqlite-amalgamation-3470000
