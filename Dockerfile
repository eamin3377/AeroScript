# Use GCC image with Flex and Bison pre-installed
FROM gcc:latest

# Install flex, bison, nodejs, npm
RUN apt-get update && apt-get install -y \
    flex \
    bison \
    make \
    curl \
    && curl -fsSL https://deb.nodesource.com/setup_18.x | bash - \
    && apt-get install -y nodejs \
    && rm -rf /var/lib/apt-get/lists/*

# Set working directory
WORKDIR /app

# Copy project source files
COPY . .

# Build C compiler binary for Linux target
RUN flex lexer.l && \
    bison -d parser.y && \
    gcc -Wall -Wextra -I. -o dronec lex.yy.c parser.tab.c ast.c interpreter.c main.c -lm

# Expose port 3000
EXPOSE 3000

# Start server
CMD ["node", "server.js"]
