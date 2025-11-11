# Kaimz TDR (Technical Design Report)

**Riipen Project** - AI Agent Infrastructure for Kaimz

Backend infrastructure for the **Kaimz Agent**, an intelligent automation system that monitors and uploads network logs to AWS S3. Built as part of our Riipen collaboration.

## Features

- 🔒 **JWT Authentication** - Secure user management with bcrypt
- ☁️ **AWS S3 Integration** - Automatic log uploads to S3
- 📊 **Network Log Monitoring** - Cross-platform (Windows/macOS) log collection
- ⏰ **Flexible Scheduling** - One-time or recurring log fetching
- 🚀 **RESTful API** - Clean endpoints for all operations

## Tech Stack

- **Backend:** Go 1.21+ with Gin framework
- **Auth:** JWT v5 with bcrypt password hashing
- **Cloud:** AWS SDK v2 for S3
- **Logging:** Native OS commands (macOS `log`, Windows PowerShell)

## Quick Start

### 1. Prerequisites

- Go 1.21 or higher
- AWS account with S3 bucket access
- macOS or Windows machine

### 2. Configure Environment

Create `/backend/.env`:

```bash
# AWS Configuration
AWS_ACCESS_KEY_ID=your_access_key
AWS_SECRET_ACCESS_KEY=your_secret_key
AWS_REGION=us-east-1
S3_BUCKET=kaimz-tdr

# Server Configuration
JWT_SECRET=your-super-secret-key-change-this
PORT=1514
```

### 3. Run Server

```bash
cd backend
go mod download
go run cmd/main.go
```

You'll be prompted:

```
Enter server port (default '1514'): [press enter or type custom port]
How often should logs be fetched (in hours, default 'Once on load'): [press enter or type 1, 2, 24, etc.]
```

**Examples:**

- Press Enter twice → Uses port 1514, fetches logs once on startup
- Type `8080` then `2` → Uses port 8080, fetches logs every 2 hours
- Type `3000` then `0.5` → Uses port 3000, fetches logs every 30 minutes

## API Endpoints

### Authentication (Public)

```bash
# Register new user
POST /auth/register
Content-Type: application/json
{"email": "user@example.com", "password": "password123"}

# Login
POST /auth/login
Content-Type: application/json
{"email": "user@example.com", "password": "password123"}
# Returns: {"token": "eyJhbG...", "user": {...}}
```

### Logs (Public - Auth Commented Out)

```bash
# Get current user info
GET /api/me

# Upload custom log to S3
POST /api/logs/upload
Content-Type: application/json
{"log": "your custom log content"}

# List all logs in S3
GET /api/logs

# Manually trigger network log fetch and upload
POST /api/logs/fetch-network
```

## How It Works

### Automatic Log Collection

1. **On Startup:** Server automatically fetches network logs and uploads to S3
2. **Recurring (Optional):** If you set an interval, logs are fetched every N hours
3. **Cross-Platform:**
   - **macOS:** Uses `log show --predicate 'subsystem CONTAINS "network"'`
   - **Windows:** Uses PowerShell to query Windows Event Log

### Log Storage

All logs are stored in your S3 bucket with timestamped filenames:

```
s3://kaimz-tdr/logs-2025-11-11T14-30-45.txt
```

## Project Structure

```
kaimz_tdr/
├── backend/
│   ├── cmd/
│   │   └── main.go              # Entry point (clean and simple)
│   ├── config/
│   │   └── config.go            # Environment configuration
│   ├── internal/
│   │   ├── aws/
│   │   │   └── s3.go            # S3 client and operations
│   │   ├── handlers/
│   │   │   ├── auth.go          # Auth handlers
│   │   │   ├── logs.go          # Log upload handlers
│   │   │   └── network_logs.go  # Network log handlers
│   │   ├── logging/
│   │   │   └── logging.go       # OS-specific log collection
│   │   ├── routes/
│   │   │   └── routes.go        # API route definitions
│   │   ├── scheduler/
│   │   │   └── log_scheduler.go # Recurring log fetch scheduler
│   │   └── startup/
│   │       ├── init.go          # S3 and scheduler initialization
│   │       └── prompts.go       # Interactive user prompts
│   ├── .env                     # Environment variables
│   └── go.mod
└── README.md
```

## Development

### Test Endpoints

```bash
# Register and get token
curl -X POST http://localhost:8080/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"test@kaimz.com","password":"test123"}'

# Upload custom log
curl -X POST http://localhost:8080/api/logs/upload \
  -H "Content-Type: application/json" \
  -d '{"log":"Custom log entry from API"}'

# Trigger manual network log fetch
curl -X POST http://localhost:8080/api/logs/fetch-network

# List all logs
curl http://localhost:8080/api/logs
```

## Configuration Options

| Variable                | Required | Default     | Description                      |
| ----------------------- | -------- | ----------- | -------------------------------- |
| `AWS_ACCESS_KEY_ID`     | Yes      | -           | AWS access key for S3            |
| `AWS_SECRET_ACCESS_KEY` | Yes      | -           | AWS secret key for S3            |
| `AWS_REGION`            | No       | `us-east-1` | AWS region for S3 bucket         |
| `S3_BUCKET`             | Yes      | `kaimz-tdr` | S3 bucket name for log storage   |
| `JWT_SECRET`            | Yes      | `TEST`      | Secret key for JWT token signing |
| `PORT`                  | No       | `1514`      | Server port (Wazuh default)      |

## Project Status

🚀 **Active Development** - Riipen Project

**Completed:**

- ✅ Cross-platform network log collection (Windows/macOS)
- ✅ Automatic S3 upload on startup
- ✅ Flexible scheduling (one-time or recurring)
- ✅ Interactive configuration prompts
- ✅ Clean, modular architecture
- ✅ JWT authentication system
- ✅ RESTful API with log management

**In Progress:**

- 🚧 AI agent capabilities
- 🚧 Advanced log analysis
- 🚧 Real-time log streaming

## License

Proprietary - Kaimz Agent Development Project
