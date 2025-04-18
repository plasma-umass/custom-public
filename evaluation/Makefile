NGINX_HASH := e24f7ccc161f1a2a759eb27263ec9af4fc7c8e96 # release-1.27.2

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
