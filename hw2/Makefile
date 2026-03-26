.PHONY: build
build:
	cmake -S . -B build/Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
	cmake --build build/Release

.PHONY: test
test: build
	cd build/Release && ctest -V --output-on-failure

.PHONY: test-user
test-user: build
	./build/Release/services/user/user_service_unittest

.PHONY: run-user
run-user: build
	./build/Release/services/user/user_service -c ./services/user/static_config.yaml

.PHONY: run-event
run-event: build
	./build/Release/services/event/event_service -c ./services/event/static_config.yaml

.PHONY: run-book
run-book: build
	./build/Release/services/book/book_service -c ./services/book/static_config.yaml

.PHONY: run-all
run-all: build
	@echo "Starting all services..."
	@echo "User service on port 8080, Event service on port 8081, Book service on port 8082"
	@./build/Release/services/user/user_service -c ./services/user/static_config.yaml &
	@./build/Release/services/event/event_service -c ./services/event/static_config.yaml &
	@./build/Release/services/book/book_service -c ./services/book/static_config.yaml

.PHONY: clangd
clangd:
	cmake -S . -B build/Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
	ln -sf build/Release/compile_commands.json compile_commands.json

.PHONY: integration-test
integration-test: build
	@echo "Installing test dependencies..."
	@pip3 install -r tests/integration/requirements.txt
	@echo "Starting services..."
	@nohup ./build/Release/services/user/user_service -c ./services/user/static_config.yaml > /tmp/user_service.log 2>&1 & echo $$! > /tmp/user_service.pid
	@nohup ./build/Release/services/event/event_service -c ./services/event/static_config.yaml > /tmp/event_service.log 2>&1 & echo $$! > /tmp/event_service.pid
	@nohup ./build/Release/services/book/book_service -c ./services/book/static_config.yaml > /tmp/book_service.log 2>&1 & echo $$! > /tmp/book_service.pid
	@sleep 5
	@echo "Running tests..."
	@python3 -m pytest tests/integration/test_full_flow.py -v; \
	TEST_EXIT_CODE=$$?; \
	kill $$(cat /tmp/user_service.pid 2>/dev/null) 2>/dev/null || true; \
	kill $$(cat /tmp/event_service.pid 2>/dev/null) 2>/dev/null || true; \
	kill $$(cat /tmp/book_service.pid 2>/dev/null) 2>/dev/null || true; \
	sleep 1; \
	rm -f /tmp/user_service.pid /tmp/event_service.pid /tmp/book_service.pid /tmp/nohup.out; \
	if [ $$TEST_EXIT_CODE -ne 0 ]; then \
		echo ""; \
		echo "Service logs (last 20 lines):"; \
		echo "--- User service ---"; \
		tail -20 /tmp/user_service.log 2>/dev/null || true; \
		echo "--- Event service ---"; \
		tail -20 /tmp/event_service.log 2>/dev/null || true; \
		echo "--- Book service ---"; \
		tail -20 /tmp/book_service.log 2>/dev/null || true; \
	fi; \
	exit $$TEST_EXIT_CODE