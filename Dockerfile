# ==============================================================================
# Domain-Aware JSON Validation and Pattern Matching System for Banking Data
# Multi-stage / Lightweight Production Container
# ==============================================================================

FROM python:3.11-slim

# Install system compiler and parser-generator toolchain
RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    flex \
    bison \
    make \
    libc6-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy all project source files, assets, and configurations
COPY . /app

# Clean and compile the C/Flex/Bison pipeline into the banking_validator binary
RUN make clean && make && chmod +x banking_validator

# Run automated integration tests during build to guarantee pipeline integrity
RUN python server.py --test

# Default networking configuration
ENV HOST=0.0.0.0
ENV PORT=8080
EXPOSE 8080

# Launch the native HTTP server
CMD ["python", "server.py"]
