# Use a multi-stage build to reduce the final image size
FROM ubuntu:22.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    zlib1g-dev \
    libspdlog-dev \
    nlohmann-json3-dev &&
    rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the project
RUN mkdir build && cd build &&
    cmake .. &&
    make -j$(nproc)

# Create the runtime image
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl1.1 \
    zlib1g \
    libspdlog1 &&
    rm -rf /var/lib/apt/lists/*

# Copy the binary from the builder stage
COPY --from=builder /app/build/tradingbot /usr/local/bin/

# Create a non-root user
RUN useradd -m tradingbot

# Set up the working directory
WORKDIR /home/tradingbot

# Copy configuration files
COPY --from=builder /app/config /home/tradingbot/config

# Set ownership
RUN chown -R tradingbot:tradingbot /home/tradingbot

# Switch to non-root user
USER tradingbot

# Set environment variables
ENV LOG_LEVEL=info
ENV SIMULATION_MODE=true

# Create log directory
RUN mkdir -p /home/tradingbot/logs

# Expose ports if needed
# EXPOSE 8080

# Set the entrypoint
ENTRYPOINT ["/usr/local/bin/tradingbot"]

# Default command
CMD ["--config", "/home/tradingbot/config/config.json"]
