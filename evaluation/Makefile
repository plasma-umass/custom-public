LLVM_HASH := ab51eccf88f5321e7c60591c5546b254b6afab99 # llvmorg-19.1.3
NGINX_HASH := e24f7ccc161f1a2a759eb27263ec9af4fc7c8e96 # release-1.27.2

llvm/src-vanilla:
	rm -rf $@
	git clone https://github.com/llvm/llvm-project.git $@
	cd $@ && git checkout $(LLVM_HASH)

llvm/build-vanilla: llvm/src-vanilla
	rm -rf $@
	cmake $</llvm -B $@ -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DLLVM_ENABLE_PROJECTS="clang"
	ninja -C $@ -j64

llvm/vanilla: llvm/build-vanilla
	rm -rf $@
	cmake --install $< --prefix $@


llvm/src-shim: llvm/shim.diff
	rm -rf $@
	git clone https://github.com/llvm/llvm-project.git $@
	cd $@ && git checkout $(LLVM_HASH)
	cd $@ && git apply ../shim.diff

llvm/build-shim: llvm/src-shim
	rm -rf $@
	cmake $</llvm -B $@ -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DLLVM_ENABLE_PROJECTS="clang"
	ninja -C $@ -j64

llvm/shim: llvm/build-shim
	rm -rf $@
	cmake --install $< --prefix $@


llvm/sqlite3.c:
	wget https://www.sqlite.org/2024/sqlite-amalgamation-3470000.zip
	unzip sqlite-amalgamation-3470000.zip
	mv sqlite-amalgamation-3470000/sqlite3.c llvm
	rm -rf sqlite-amalgamation-3470000.zip sqlite-amalgamation-3470000



nginx/src-vanilla:
	rm -rf $@
	git clone https://github.com/nginx/nginx.git $@
	cd $@ && git checkout $(NGINX_HASH)

nginx/vanilla: nginx/src-vanilla
	rm -rf $@
	cd $< && ./auto/configure --prefix=$(abspath $@) && make -j && make install

.PHONY: nginx/www
nginx/www:
	rm -rf $@
	mkdir $@
	for i in $$(seq 1 1000); do \
		HASH=$$(openssl rand -hex 16); \
		SIZE=$$(shuf -i 262144-1048576 -n 1); \
		head -c $$SIZE </dev/urandom > $@/$$HASH.blob; \
		echo $$HASH.blob >> $@/file.list; \
		printf "%4s %s\n" $$i $$HASH.blob; \
	done
