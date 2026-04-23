FROM ghcr.io/userver-framework/ubuntu-userver-build-base:v1 AS builder

WORKDIR /app

COPY hw4/conanfile.txt hw4/CMakeLists.txt hw4/CMakeUserPresets.json ./
COPY hw4/common ./common
COPY hw4/services/analytics ./services/analytics

RUN conan install . --build=missing -s compiler.cppstd=20 -s build_type=Release

RUN cmake --preset=release && \
    cmake --build --preset=release --target analytics_service

FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/release/services/analytics/analytics_service /app/
COPY hw4/services/analytics/static_config.yaml /app/configs/static_config.yaml

EXPOSE 8083

CMD ["/app/analytics_service", "--config", "/app/configs/static_config.yaml"]
