# Multi-service Dockerfile for event booking platform
# Builds all services in a single image, then creates separate runtime images

FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest AS builder

WORKDIR /build

# Install Conan 2.x
RUN pip3 install conan && conan profile detect --force

# Add userver Conan remote (if available) or use local build
# First, let's try to export userver to local Conan cache
COPY userver/ /userver/

# Export userver to Conan local cache
WORKDIR /userver
RUN conan export . --version=2.16-rc

# Now build the services
WORKDIR /build

COPY hw4/conanfile.txt .

# Install dependencies with Conan (now userver is in local cache)
RUN conan install . -of=build/Release --build=missing -s build_type=Release

COPY hw4/CMakeLists.txt .
COPY hw4/common/ common/
COPY hw4/services/ services/
COPY hw4/configs/ configs/

# Build all services
RUN cmake -S . -B build/Release \
    -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build/Release -j$(nproc)

# ============================================
# Runtime image for user_service
# ============================================
FROM ubuntu:22.04 AS user-service

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libcurl4 \
    libc-ares2 \
    libnghttp2-14 \
    libev4 \
    libyaml-cpp0.7 \
    libfmt8 \
    libcctz2 \
    libcrypto++8 \
    libboost-program-options1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-locale1.74.0 \
    libboost-regex1.74.0 \
    libboost-iostreams1.74.0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/build/Release/services/user/user_service .
COPY hw4/services/user/static_config.yaml .
COPY hw4/configs/ configs/

EXPOSE 8080
CMD ["./user_service", "-c", "static_config.yaml"]

# ============================================
# Runtime image for event_service
# ============================================
FROM ubuntu:22.04 AS event-service

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libcurl4 \
    libc-ares2 \
    libnghttp2-14 \
    libev4 \
    libyaml-cpp0.7 \
    libfmt8 \
    libcctz2 \
    libcrypto++8 \
    libboost-program-options1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-locale1.74.0 \
    libboost-regex1.74.0 \
    libboost-iostreams1.74.0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/build/Release/services/event/event_service .
COPY hw4/services/event/static_config.yaml .
COPY hw4/configs/ configs/

EXPOSE 8081
CMD ["./event_service", "-c", "static_config.yaml"]

# ============================================
# Runtime image for book_service
# ============================================
FROM ubuntu:22.04 AS book-service

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libcurl4 \
    libc-ares2 \
    libnghttp2-14 \
    libev4 \
    libyaml-cpp0.7 \
    libfmt8 \
    libcctz2 \
    libcrypto++8 \
    libboost-program-options1.74.0 \
    libboost-filesystem1.74.0 \
    libboost-locale1.74.0 \
    libboost-regex1.74.0 \
    libboost-iostreams1.74.0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/build/Release/services/book/book_service .
COPY hw4/services/book/static_config.docker.yaml ./static_config.yaml
COPY hw4/configs/ configs/

EXPOSE 8082
CMD ["./book_service", "-c", "static_config.yaml"]
