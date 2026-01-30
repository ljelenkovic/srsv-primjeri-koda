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

# Copy stats code
COPY stats/ ./stats/

# Build
WORKDIR /app/stats
RUN make

CMD ["./stats"]
