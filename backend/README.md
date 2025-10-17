# Backend API

> Part of the Kaimz TDR project - See [root README](../README.md) for full project overview

Go backend API for the Kaimz AI Agent with JWT authentication and S3 log storage.

## Quick Setup

### 1. Install Dependencies
```bash
go mod download
```

### 2. Configure AWS Credentials

**Option A: AWS CLI (Recommended)**
```bash
aws configure
```

**Option B: Manual Setup**

Create `~/.aws/credentials`:
```bash
mkdir -p ~/.aws
nano ~/.aws/credentials
```
```ini
[kaimz_tdr]
aws_access_key_id = YOUR_ACCESS_KEY_ID
aws_secret_access_key = YOUR_SECRET_ACCESS_KEY
```

Create `~/.aws/config`:
```bash
nano ~/.aws/config
```
```ini
[kaimz_tdr]
region = us-east-1
```

### 3. Environment Variables

```bash
cp .env.example .env
# Edit .env and set:
# - JWT_SECRET (generate: openssl rand -base64 32)
# - S3_BUCKET (your S3 bucket name)
# - AWS_REGION (default: us-east-1)
```

### 4. Run

```bash
export $(cat .env | xargs)
go run cmd/main.go
```

Server: `http://localhost:8080`

## API Reference

### Authentication

**Register:**
```bash
POST /auth/register
{"email": "user@example.com", "password": "pass123"}
```

**Login:**
```bash
POST /auth/login
{"email": "user@example.com", "password": "pass123"}
```
Returns: `{"token": "...", "user": {...}}`

### Protected Endpoints

Requires header: `Authorization: Bearer <token>`

**Get user info:**
```bash
GET /api/me
```

**Upload log:**
```bash
POST /api/logs/upload
{"log": "your log content", "filename": "optional/custom/path.log"}
```

**List logs:**
```bash
GET /api/logs
```

## Project Structure

```
backend/
├── cmd/main.go              # Entry point
├── config/config.go         # Environment config
├── internal/
│   ├── auth/               # JWT + password hashing
│   ├── aws/                # S3 client
│   ├── handlers/           # API handlers
│   ├── models/             # Data models
│   └── routes/             # Route setup
├── Dockerfile              # Container config
└── .env.example            # Environment template
```

## Environment Variables

| Variable | Required | Description |
|----------|----------|-------------|
| `JWT_SECRET` | Yes | JWT signing secret |
| `S3_BUCKET` | Yes | S3 bucket for logs |
| `AWS_REGION` | No | Default: us-east-1 |
| `PORT` | No | Default: 8080 |

## Quick Test

```bash
# Register & get token
TOKEN=$(curl -s -X POST http://localhost:8080/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com","password":"pass123"}' | jq -r '.token')

# Upload log
curl -X POST http://localhost:8080/api/logs/upload \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"log":"Test log message"}' | jq .

# List logs
curl http://localhost:8080/api/logs \
  -H "Authorization: Bearer $TOKEN" | jq .
```

## Docker

```bash
docker build -t kaimz-backend .
docker run -p 8080:8080 \
  -e JWT_SECRET="your-secret" \
  -e S3_BUCKET="your-bucket" \
  -v ~/.aws:/root/.aws:ro \
  kaimz-backend
```

## Notes

- Users stored in-memory (restart clears data)
- Logs stored at `s3://bucket/logs/YYYY-MM-DD_HH-MM-SS.log`
- AWS credentials from `~/.aws/credentials`

## Tech Stack

- **Framework:** Gin
- **Auth:** JWT v5 + bcrypt
- **Cloud:** AWS SDK v2 (S3)
- **UUID:** google/uuid


