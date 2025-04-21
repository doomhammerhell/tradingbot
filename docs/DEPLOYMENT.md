# Deployment Guide

## Overview

This guide covers the deployment process, environment setup, configuration management, and monitoring for the trading bot system in production environments.

## Build Process

### Build Configuration

1. **Release Build**
```bash
# Configure for release
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_TESTING=OFF \
      -DENABLE_DOCS=OFF \
      -DCMAKE_CXX_FLAGS="-O3 -march=native" \
      ..

# Build
make -j$(nproc)
```

2. **Docker Build**
```dockerfile
# Dockerfile
FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libomp-dev \
    nlohmann-json3-dev

# Copy source
COPY . /app
WORKDIR /app

# Build
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Run
CMD ["./build/tradingbot"]
```

### Build Artifacts

1. **Binary Package**
```bash
# Create package
mkdir -p package/bin
cp build/tradingbot package/bin/
cp config.example.json package/config.json
tar -czf tradingbot.tar.gz package/
```

2. **Docker Image**
```bash
# Build image
docker build -t tradingbot:latest .

# Push to registry
docker tag tradingbot:latest registry.example.com/tradingbot:latest
docker push registry.example.com/tradingbot:latest
```

## Environment Setup

### System Requirements

1. **Hardware**
   - CPU: 4+ cores
   - RAM: 8+ GB
   - Storage: 50+ GB SSD
   - Network: 100+ Mbps

2. **Software**
   - OS: Ubuntu 20.04 LTS
   - Docker: 20.10+
   - Docker Compose: 1.29+
   - Nginx: 1.18+

### Network Configuration

1. **Firewall Rules**
```bash
# Allow necessary ports
ufw allow 22/tcp    # SSH
ufw allow 80/tcp    # HTTP
ufw allow 443/tcp   # HTTPS
ufw allow 3000/tcp  # API
ufw enable
```

2. **SSL Configuration**
```bash
# Generate SSL certificate
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/ssl/private/nginx-selfsigned.key \
    -out /etc/ssl/certs/nginx-selfsigned.crt
```

### Database Setup

1. **PostgreSQL**
```bash
# Install PostgreSQL
apt-get install -y postgresql postgresql-contrib

# Create database
sudo -u postgres psql -c "CREATE DATABASE tradingbot;"
sudo -u postgres psql -c "CREATE USER tradingbot WITH PASSWORD 'password';"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE tradingbot TO tradingbot;"
```

2. **Redis**
```bash
# Install Redis
apt-get install -y redis-server

# Configure Redis
sed -i 's/supervised no/supervised systemd/' /etc/redis/redis.conf
systemctl restart redis
```

## Configuration Management

### Environment Variables

1. **Production Environment**
```bash
# .env.production
DB_HOST=localhost
DB_PORT=5432
DB_NAME=tradingbot
DB_USER=tradingbot
DB_PASSWORD=password

REDIS_HOST=localhost
REDIS_PORT=6379

API_KEY=your_api_key
API_SECRET=your_api_secret
```

2. **Docker Environment**
```yaml
# docker-compose.yml
version: '3'
services:
  tradingbot:
    image: tradingbot:latest
    environment:
      - DB_HOST=postgres
      - DB_PORT=5432
      - DB_NAME=tradingbot
      - DB_USER=tradingbot
      - DB_PASSWORD=password
      - REDIS_HOST=redis
      - REDIS_PORT=6379
      - API_KEY=${API_KEY}
      - API_SECRET=${API_SECRET}
    depends_on:
      - postgres
      - redis

  postgres:
    image: postgres:13
    environment:
      - POSTGRES_DB=tradingbot
      - POSTGRES_USER=tradingbot
      - POSTGRES_PASSWORD=password
    volumes:
      - postgres_data:/var/lib/postgresql/data

  redis:
    image: redis:6
    volumes:
      - redis_data:/data

volumes:
  postgres_data:
  redis_data:
```

### Configuration Files

1. **Application Config**
```json
{
    "api": {
        "binance": {
            "api_key": "${API_KEY}",
            "api_secret": "${API_SECRET}"
        }
    },
    "trading": {
        "initial_capital": 10000.0,
        "transaction_cost": 0.001,
        "risk_per_trade": 0.02
    },
    "strategies": {
        "mean_reversion": {
            "enabled": true,
            "parameters": {
                "z_score_threshold": 2.0,
                "window_size": 20,
                "take_profit": 0.5,
                "stop_loss": 0.1
            }
        }
    }
}
```

2. **Nginx Config**
```nginx
# /etc/nginx/sites-available/tradingbot
server {
    listen 80;
    server_name tradingbot.example.com;
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl;
    server_name tradingbot.example.com;

    ssl_certificate /etc/ssl/certs/nginx-selfsigned.crt;
    ssl_certificate_key /etc/ssl/private/nginx-selfsigned.key;

    location / {
        proxy_pass http://localhost:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
    }
}
```

## Deployment Process

### Manual Deployment

1. **Prepare Environment**
```bash
# Update system
apt-get update && apt-get upgrade -y

# Install dependencies
apt-get install -y nginx postgresql redis-server

# Configure services
systemctl enable nginx postgresql redis-server
systemctl start nginx postgresql redis-server
```

2. **Deploy Application**
```bash
# Extract package
tar -xzf tradingbot.tar.gz -C /opt/

# Set permissions
chown -R tradingbot:tradingbot /opt/tradingbot

# Create systemd service
cat > /etc/systemd/system/tradingbot.service << EOF
[Unit]
Description=Trading Bot
After=network.target postgresql.service redis-server.service

[Service]
User=tradingbot
Group=tradingbot
WorkingDirectory=/opt/tradingbot
ExecStart=/opt/tradingbot/bin/tradingbot
Restart=always

[Install]
WantedBy=multi-user.target
EOF

# Start service
systemctl daemon-reload
systemctl enable tradingbot
systemctl start tradingbot
```

### Automated Deployment

1. **Ansible Playbook**
```yaml
# deploy.yml
- hosts: tradingbot
  become: yes
  tasks:
    - name: Update system
      apt:
        update_cache: yes
        upgrade: dist

    - name: Install dependencies
      apt:
        name:
          - nginx
          - postgresql
          - redis-server
        state: present

    - name: Configure services
      systemd:
        name: "{{ item }}"
        state: started
        enabled: yes
      with_items:
        - nginx
        - postgresql
        - redis-server

    - name: Deploy application
      unarchive:
        src: tradingbot.tar.gz
        dest: /opt/
        remote_src: yes

    - name: Set permissions
      file:
        path: /opt/tradingbot
        owner: tradingbot
        group: tradingbot
        recurse: yes

    - name: Start service
      systemd:
        name: tradingbot
        state: started
        enabled: yes
```

2. **CI/CD Pipeline**
```yaml
# .gitlab-ci.yml
stages:
  - build
  - test
  - deploy

build:
  stage: build
  script:
    - mkdir build && cd build
    - cmake -DCMAKE_BUILD_TYPE=Release ..
    - make -j$(nproc)
  artifacts:
    paths:
      - build/tradingbot

test:
  stage: test
  script:
    - cd build
    - ctest

deploy:
  stage: deploy
  script:
    - ansible-playbook deploy.yml
  only:
    - master
```

## Monitoring

### System Monitoring

1. **Prometheus Configuration**
```yaml
# prometheus.yml
global:
  scrape_interval: 15s

scrape_configs:
  - job_name: 'tradingbot'
    static_configs:
      - targets: ['localhost:9090']
  - job_name: 'node'
    static_configs:
      - targets: ['localhost:9100']
```

2. **Grafana Dashboard**
```json
{
    "dashboard": {
        "panels": [
            {
                "title": "CPU Usage",
                "type": "graph",
                "datasource": "Prometheus",
                "targets": [
                    {
                        "expr": "rate(process_cpu_seconds_total[5m])"
                    }
                ]
            },
            {
                "title": "Memory Usage",
                "type": "graph",
                "datasource": "Prometheus",
                "targets": [
                    {
                        "expr": "process_resident_memory_bytes"
                    }
                ]
            }
        ]
    }
}
```

### Application Monitoring

1. **Logging Configuration**
```cpp
class Logger {
public:
    void setup() {
        // Configure logging
        spdlog::set_level(spdlog::level::info);
        auto file_logger = spdlog::basic_logger_mt(
            "file_logger",
            "logs/tradingbot.log"
        );
        spdlog::set_default_logger(file_logger);
    }
};
```

2. **Metrics Collection**
```cpp
class MetricsCollector {
public:
    void collect() {
        // Collect metrics
        metrics_.cpu_usage = getCPUUsage();
        metrics_.memory_usage = getMemoryUsage();
        metrics_.trades_count = getTradesCount();
        metrics_.profit = getTotalProfit();
        
        // Export to Prometheus
        exporter_.export(metrics_);
    }

private:
    Metrics metrics_;
    PrometheusExporter exporter_;
};
```

## Backup and Recovery

### Database Backup

1. **Automated Backup**
```bash
# backup.sh
#!/bin/bash
BACKUP_DIR="/backups"
DATE=$(date +%Y%m%d_%H%M%S)

# Backup PostgreSQL
pg_dump -U tradingbot tradingbot > $BACKUP_DIR/tradingbot_$DATE.sql

# Backup Redis
redis-cli SAVE
cp /var/lib/redis/dump.rdb $BACKUP_DIR/redis_$DATE.rdb

# Cleanup old backups
find $BACKUP_DIR -type f -mtime +7 -delete
```

2. **Restore Database**
```bash
# restore.sh
#!/bin/bash
BACKUP_FILE=$1

# Restore PostgreSQL
psql -U tradingbot tradingbot < $BACKUP_FILE

# Restore Redis
systemctl stop redis-server
cp $BACKUP_FILE /var/lib/redis/dump.rdb
systemctl start redis-server
```

### Configuration Backup

1. **Backup Configuration**
```bash
# backup_config.sh
#!/bin/bash
BACKUP_DIR="/backups/config"
DATE=$(date +%Y%m%d_%H%M%S)

# Backup configuration files
cp /opt/tradingbot/config.json $BACKUP_DIR/config_$DATE.json
cp /etc/nginx/sites-available/tradingbot $BACKUP_DIR/nginx_$DATE.conf

# Cleanup old backups
find $BACKUP_DIR -type f -mtime +30 -delete
```

2. **Restore Configuration**
```bash
# restore_config.sh
#!/bin/bash
BACKUP_FILE=$1

# Restore configuration
cp $BACKUP_FILE /opt/tradingbot/config.json
systemctl restart tradingbot
```

## Security

### Access Control

1. **User Management**
```bash
# Create tradingbot user
useradd -m -s /bin/bash tradingbot
usermod -aG sudo tradingbot

# Set up SSH access
mkdir -p /home/tradingbot/.ssh
cp authorized_keys /home/tradingbot/.ssh/
chown -R tradingbot:tradingbot /home/tradingbot/.ssh
chmod 700 /home/tradingbot/.ssh
chmod 600 /home/tradingbot/.ssh/authorized_keys
```

2. **Firewall Rules**
```bash
# Configure UFW
ufw default deny incoming
ufw default allow outgoing
ufw allow ssh
ufw allow http
ufw allow https
ufw enable
```

### SSL/TLS

1. **Certificate Management**
```bash
# Install Certbot
apt-get install -y certbot python3-certbot-nginx

# Obtain certificate
certbot --nginx -d tradingbot.example.com

# Auto-renewal
certbot renew --dry-run
```

2. **Security Headers**
```nginx
# Nginx security headers
add_header X-Frame-Options "SAMEORIGIN";
add_header X-XSS-Protection "1; mode=block";
add_header X-Content-Type-Options "nosniff";
add_header Strict-Transport-Security "max-age=31536000; includeSubDomains";
```

## Troubleshooting

### Common Issues

1. **Service Not Starting**
```bash
# Check status
systemctl status tradingbot

# Check logs
journalctl -u tradingbot

# Check configuration
tradingbot --check-config
```

2. **Database Issues**
```bash
# Check PostgreSQL
systemctl status postgresql
psql -U tradingbot -c "\l"

# Check Redis
systemctl status redis-server
redis-cli ping
```

3. **Network Issues**
```bash
# Check ports
netstat -tulpn | grep LISTEN

# Check firewall
ufw status

# Check SSL
openssl s_client -connect tradingbot.example.com:443
```

### Recovery Procedures

1. **Service Recovery**
```bash
# Stop service
systemctl stop tradingbot

# Backup data
backup.sh

# Restore from backup
restore.sh backup_file

# Start service
systemctl start tradingbot
```

2. **Database Recovery**
```bash
# Stop services
systemctl stop tradingbot postgresql redis-server

# Restore databases
restore.sh db_backup.sql
restore.sh redis_backup.rdb

# Start services
systemctl start postgresql redis-server tradingbot
```

## Best Practices

1. **Deployment**
   - Use version control
   - Implement CI/CD
   - Test in staging
   - Rollback plan

2. **Monitoring**
   - Set up alerts
   - Monitor metrics
   - Log everything
   - Regular backups

3. **Security**
   - Regular updates
   - Strong passwords
   - Limited access
   - SSL/TLS

4. **Maintenance**
   - Regular backups
   - Log rotation
   - Performance tuning
   - Security patches 