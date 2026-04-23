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

.PHONY: run-analytics
run-analytics: build
	./build/Release/services/analytics/analytics_service -c ./services/analytics/static_config.yaml

.PHONY: run-all
run-all: build
	@echo "Starting all services..."
	@echo "User service on port 8080, Event service on port 8081, Book service on port 8082, Analytics service on port 8083"
	@./build/Release/services/user/user_service -c ./services/user/static_config.yaml &
	@./build/Release/services/event/event_service -c ./services/event/static_config.yaml &
	@./build/Release/services/book/book_service -c ./services/book/static_config.yaml &
	@./build/Release/services/analytics/analytics_service -c ./services/analytics/static_config.yaml

.PHONY: clangd
clangd:
	cmake -S . -B build/Release -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release && \
	ln -sf build/Release/compile_commands.json compile_commands.json

.PHONY: integration-test
integration-test: build
	@echo "Installing test dependencies..."
	@pip3 install -r tests/integration/requirements.txt
	@echo "Starting databases..."
	@docker-compose up -d postgres mongodb
	@sleep 5
	@echo "Cleaning PostgreSQL database..."
	@docker-compose exec -T postgres psql -U eventbooking -d eventbooking -c "DROP TABLE IF EXISTS bookings CASCADE; DROP TABLE IF EXISTS events CASCADE; DROP TABLE IF EXISTS users CASCADE;"
	@docker-compose exec -T postgres psql -U eventbooking -d eventbooking -f /docker-entrypoint-initdb.d/01-schema.sql
	@sleep 2
	@echo "Starting services..."
	@nohup ./build/Release/services/user/user_service -c ./services/user/static_config.yaml > /tmp/user_service.log 2>&1 & echo $$! > /tmp/user_service.pid
	@nohup ./build/Release/services/event/event_service -c ./services/event/static_config.yaml > /tmp/event_service.log 2>&1 & echo $$! > /tmp/event_service.pid
	@nohup ./build/Release/services/book/book_service -c ./services/book/static_config.yaml > /tmp/book_service.log 2>&1 & echo $$! > /tmp/book_service.pid
	@nohup ./build/Release/services/analytics/analytics_service -c ./services/analytics/static_config.yaml > /tmp/analytics_service.log 2>&1 & echo $$! > /tmp/analytics_service.pid
	@sleep 5
	@echo "Running tests..."
	@python3 -m pytest tests/integration/test_full_flow.py tests/integration/test_analytics.py -v; \
	TEST_EXIT_CODE=$$?; \
	kill $$(cat /tmp/user_service.pid 2>/dev/null) 2>/dev/null || true; \
	kill $$(cat /tmp/event_service.pid 2>/dev/null) 2>/dev/null || true; \
	kill $$(cat /tmp/book_service.pid 2>/dev/null) 2>/dev/null || true; \
	kill $$(cat /tmp/analytics_service.pid 2>/dev/null) 2>/dev/null || true; \
	sleep 1; \
	rm -f /tmp/user_service.pid /tmp/event_service.pid /tmp/book_service.pid /tmp/analytics_service.pid /tmp/nohup.out; \
	if [ $$TEST_EXIT_CODE -ne 0 ]; then \
		echo ""; \
		echo "Service logs (last 20 lines):"; \
		echo "--- User service ---"; \
		tail -20 /tmp/user_service.log 2>/dev/null || true; \
		echo "--- Event service ---"; \
		tail -20 /tmp/event_service.log 2>/dev/null || true; \
		echo "--- Book service ---"; \
		tail -20 /tmp/book_service.log 2>/dev/null || true; \
		echo "--- Analytics service ---"; \
		tail -20 /tmp/analytics_service.log 2>/dev/null || true; \
	fi; \
	exit $$TEST_EXIT_CODE

.PHONY: docker-integration-test
docker-integration-test:
	@echo "Building and starting all services with Docker..."
	@docker-compose up -d --build
	@echo "Waiting for services to be ready..."
	@sleep 30
	@echo "Installing test dependencies..."
	@pip3 install -r tests/integration/requirements.txt
	@echo "Running integration tests..."
	@python3 -m pytest tests/integration/test_full_flow.py tests/integration/test_analytics.py -v; \
	TEST_EXIT_CODE=$$?; \
	if [ $$TEST_EXIT_CODE -ne 0 ]; then \
		echo ""; \
		echo "Service logs:"; \
		echo "--- User service ---"; \
		docker-compose logs --tail=20 user-service 2>/dev/null || true; \
		echo "--- Event service ---"; \
		docker-compose logs --tail=20 event-service 2>/dev/null || true; \
		echo "--- Book service ---"; \
		docker-compose logs --tail=20 book-service 2>/dev/null || true; \
		echo "--- Analytics service ---"; \
		docker-compose logs --tail=20 analytics-service 2>/dev/null || true; \
	fi; \
	exit $$TEST_EXIT_CODE

.PHONY: docker-up
docker-up:
	@echo "Starting all services with Docker..."
	@docker-compose up -d --build

.PHONY: docker-down
docker-down:
	@echo "Stopping all services..."
	@docker-compose down

.PHONY: docker-logs
docker-logs:
	@docker-compose logs -f

.PHONY: db-up
db-up:
	@echo "Starting PostgreSQL..."
	@docker-compose up -d postgres
	@echo "Waiting for PostgreSQL to be ready..."
	@sleep 5
	@docker-compose exec postgres pg_isready -U eventbooking -d eventbooking

.PHONY: db-down
db-down:
	@echo "Stopping PostgreSQL..."
	@docker-compose stop postgres

.PHONY: db-reset
db-reset:
	@echo "Resetting database..."
	@docker-compose down -v postgres
	@docker-compose up -d postgres
	@sleep 5
	@echo "Database reset complete."

.PHONY: db-shell
db-shell:
	@docker-compose exec postgres psql -U eventbooking -d eventbooking

.PHONY: mongo-up
mongo-up:
	@echo "Starting MongoDB..."
	@docker-compose up -d mongodb
	@echo "Waiting for MongoDB to be ready..."
	@sleep 5

.PHONY: mongo-down
mongo-down:
	@echo "Stopping MongoDB..."
	@docker-compose stop mongodb

.PHONY: mongo-reset
mongo-reset:
	@echo "Resetting MongoDB..."
	@docker-compose down -v mongodb
	@docker-compose up -d mongodb
	@sleep 5
	@echo "MongoDB reset complete."

.PHONY: mongo-shell
mongo-shell:
	@docker-compose exec mongodb mongosh analytics
