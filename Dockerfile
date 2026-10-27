# ==============================================================================
# Stage 1: Build the C++ Backend with CMake, GCC, and Ninja
# ==============================================================================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy CMakeLists, backend source files, and third_party vendor dependencies
COPY CMakeLists.txt .
COPY backend/ backend/
COPY third_party/ third_party/

# Configure and compile with Release optimization using Ninja
RUN cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

# ==============================================================================
# Stage 2: Minimal Production Runtime
# ==============================================================================
FROM ubuntu:22.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy compiled executable from builder stage
COPY --from=builder /app/build/student_management_server /app/student_management_server
RUN chmod +x /app/student_management_server

# Copy frontend assets (served as fallback or in single-container mode)
COPY frontend/ /app/frontend/

# Create persistent data directory for SQLite database
RUN mkdir -p /app/data && chmod 777 /app/data

# Default production environment variables (Render automatically sets $PORT)
ENV PORT=18080
ENV BIND_ADDRESS=0.0.0.0
ENV DB_PATH=/app/data/student_management.db
ENV FRONTEND_DIR=/app/frontend

EXPOSE 18080

# Execute server without CLI arguments so it reads PORT and DB_PATH from environment
CMD ["./student_management_server"]
