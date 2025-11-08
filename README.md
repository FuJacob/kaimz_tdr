# Kaimz TDR (Technical Design Report)

**Riipen Project** - AI Agent Development for Kaimz

This repository contains the backend infrastructure and development work for the **Kaimz Agent**, an intelligent automation system currently under active development. This project is part of our Riipen collaboration focused on building production-ready AI agent capabilities.

## Project Overview

Kaimz TDR serves as the technical backbone for the Kaimz Agent, providing:

- **Authentication & Authorization** - JWT-based user management
- **Cloud Integration** - AWS S3 for log storage and data persistence
- **API Infrastructure** - RESTful endpoints for agent operations
- **Scalable Architecture** - Foundation for future AI agent features

## Repository Structure

```
kaimz_tdr/
├── backend/          # Go API server with JWT auth + S3
├── agent.c           # Agent development artifacts
└── README.md         # This file
```

## Tech Stack

- **Backend:** Go + Gin framework
- **Auth:** JWT with bcrypt password hashing
- **Cloud:** AWS S3 for storage
- **Deployment:** Docker-ready

## Quick Start

### Prerequisites

- Go 1.21+
- AWS Account with S3 bucket
- AWS CLI configured

### Setup

1. **Configure AWS Credentials**

```bash
mkdir -p ~/.aws
nano ~/.aws/credentials
```

Add your credentials:

```ini
[kaimz_tdr]
aws_access_key_id = YOUR_ACCESS_KEY_ID
aws_secret_access_key = YOUR_SECRET_ACCESS_KEY
```

Set region (optional):

```bash
nano ~/.aws/config
```

```ini
[kaimz_tdr]
region = us-east-1
```

2. **Configure Backend**

```bash
cd backend
cp .env.example .env
# Edit .env:
# - JWT_SECRET (generate: openssl rand -base64 32)
# - S3_BUCKET (your bucket name)
# - AWS_REGION (default: us-east-1)
```

3. **Run Server**

```bash
cd backend
go mod download
export $(cat .env | xargs)
go run cmd/main.go
```

Server starts at `http://localhost:8080`

## API Endpoints

### Public Routes

```bash
# Register user
POST /auth/register
{"email": "user@example.com", "password": "pass123"}

# Login
POST /auth/login
{"email": "user@example.com", "password": "pass123"}
# Returns: {"token": "eyJhbG...", "user": {...}}
```

### Protected Routes (Require JWT)

Add header: `Authorization: Bearer <token>`

```bash
# Get current user info
GET /api/me

# Upload log to S3
POST /api/logs/upload
{"log": "your log content"}

# List all logs
GET /api/logs
```

## Development Workflow

```bash
# Quick test
TOKEN=$(curl -s -X POST http://localhost:8080/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com","password":"pass123"}' | jq -r '.token')

curl -X POST http://localhost:8080/api/logs/upload \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"log":"Test log from Kaimz agent"}' | jq .
```

## Docker Deployment

```bash
cd backend
docker build -t kaimz-backend .
docker run -p 8080:8080 \
  -e JWT_SECRET="your-secret" \
  -e S3_BUCKET="your-bucket" \
  -e AWS_REGION="us-east-1" \
  -v ~/.aws:/root/.aws:ro \
  kaimz-backend
```

## Project Status

🚧 **Active Development** - This is an ongoing Riipen project

Current Phase:

- ✅ JWT Authentication system
- ✅ AWS S3 integration for logs
- ✅ RESTful API foundation
- 🚧 AI Agent capabilities (in progress)
- 🚧 Advanced automation features (planned)

## Environment Variables

| Variable     | Required | Description                     |
| ------------ | -------- | ------------------------------- |
| `JWT_SECRET` | Yes      | JWT token signing secret        |
| `S3_BUCKET`  | Yes      | AWS S3 bucket name              |
| `AWS_REGION` | No       | AWS region (default: us-east-1) |
| `PORT`       | No       | Server port (default: 8080)     |

## Riipen Collaboration

This project is developed as part of a Riipen educational partnership, focused on:

- Real-world AI agent development
- Production-grade backend architecture
- Cloud infrastructure integration
- Secure authentication systems

## Contributing

This is an active development repository. For questions or collaboration:

- Review the backend README for detailed API documentation
- Check open issues for current development tasks
- Follow Go best practices and project conventions

## License

Proprietary - Kaimz Agent Development Project
