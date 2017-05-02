

RABBITMQ_C_SRC_DIR := rabbitmq-c-0.8.0

rabbitmq-c:
	test -d $(RABBITMQ_C_SRC_DIR) || unzip rabbitmq-c-0.8.0.zip
	cd $(RABBITMQ_C_SRC_DIR) && (mkdir -p build; cd build; \
		cmake -D CMAKE_C_COMPILER=/usr/local/arm_linux_4.8/bin/arm-linux-gcc \
		-D CMAKE_CXX_COMPILER=/usr/local/arm_linux_4.8/bin/arm-linux-g++ ..; \
		cmake -DENABLE_SSL_SUPPORT=OFF ..; \
		cmake --build .; \
		cmake -DCMAKE_INSTALL_PREFIX=$(TOP_DIR)/tmp ..;\
		cmake --build . --target install;)
	@echo "rabbitmq-c build finished!"

clean:
	test ! -d $(RABBITMQ_C_SRC_DIR)/build || (cd $(RABBITMQ_C_SRC_DIR)/build; make clean;)
	#make -C $(RABBITMQ_C_SRC_DIR)/build clean
	@echo "clean rabbitmq-c!"



