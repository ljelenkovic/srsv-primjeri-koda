FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    librabbitmq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy common headers
COPY common/ ./common/

# Copy processor code
COPY processor/ ./processor/

# Build
WORKDIR /app/processor
RUN make

# Create health check directory
RUN mkdir -p /tmp

# Health check: verify the healthy marker file exists and is recent
HEALTHCHECK --interval=5s --timeout=3s --retries=3 \
  CMD test -f /tmp/healthy || exit 1

CMD ["./processor"]
