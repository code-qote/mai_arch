# Standalone Dockerfile for book_service
# For use with docker-compose, use the root hw2/Dockerfile instead

FROM ghcr.io/userver-framework/ubuntu-22.04-userver-base:latest AS builder

WORKDIR /build

# Install Conan 2.x
RUN pip3 install conan && conan profile detect --force

# Copy and export userver to local Conan cache
COPY userver/ /userver/
WORKDIR /userver
RUN conan export . --version=2.16-rc

WORKDIR /build

# Copy conan files and install dependencies
COPY hw2/conanfile.txt .
RUN conan install . -of=build/Release --build=missing -s build_type=Release

# Copy source code
COPY hw2/CMakeLists.txt .
COPY hw2/common/ common/
COPY hw2/services/book/ services/book/
COPY hw2/configs/ configs/

# Build the service
RUN cmake -S . -B build/Release \
    -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build/Release --target book_service -j$(nproc)

# Runtime stage
FROM ubuntu:22.04 AS runtime

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
COPY hw2/services/book/static_config.docker.yaml ./static_config.yaml
COPY hw2/configs/ configs/

EXPOSE 8082
CMD ["./book_service", "-c", "static_config.yaml"]
