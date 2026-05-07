#!/bin/sh
# Start Redis server on port 16380
redis-server --port 16380 --daemonize yes

# Wait for Redis to be ready
until redis-cli -p 16380 ping 2>/dev/null | grep -q PONG; do
  sleep 0.1
done
echo "Redis server is ready on port 16380"

# Copy sentinel config (sentinel modifies it at runtime)
cp /etc/redis/sentinel-template.conf /tmp/sentinel.conf

# Start sentinel in foreground
exec redis-sentinel /tmp/sentinel.conf
