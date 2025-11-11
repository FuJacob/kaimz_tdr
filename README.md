# Kaimz TDR

Cross-platform network log monitoring and S3 storage system with JWT authentication.

## Features

- Cross-platform log collection (Windows/macOS)
- Automatic S3 upload with configurable scheduling
- JWT authentication and RESTful API
- Interactive configuration on startup

## Stack

- **Backend:** Go 1.21+ | Gin Framework
- **Auth:** JWT v5 | bcrypt
- **Cloud:** AWS S3 (SDK v2)
- **Logging:** Native OS commands

## Setup

**1. Environment Configuration**

Create `backend/.env`:
```env
AWS_ACCESS_KEY_ID=your_key
AWS_SECRET_ACCESS_KEY=your_secret
AWS_REGION=us-east-1
S3_BUCKET=kaimz-tdr
JWT_SECRET=your_secret
PORT=1514
```

**2. Run**

```bash
cd backend
go mod download
go run cmd/main.go
```

**3. Configure on Startup**

```
Enter server port (default '1514'): [Enter for default]
How often should logs be fetched (in hours, default 'Once on load'): [Enter once, or 1, 2, 24, etc.]
```

## API

### Authentication
```bash
POST /auth/register  # {"email": "user@example.com", "password": "pass"}
POST /auth/login     # {"email": "user@example.com", "password": "pass"}
```

### Logs
```bash
GET  /api/me                      # Get current user
POST /api/logs/upload             # Upload custom log
GET  /api/logs                    # List all logs
POST /api/logs/fetch-network      # Trigger network log fetch
```

## Architecture

```
backend/
├── cmd/main.go                   # Entry point
├── config/                       # Environment config
├── internal/
│   ├── aws/s3.go                # S3 operations
│   ├── constants/               # OS commands & limits
│   ├── handlers/                # API handlers
│   ├── logging/                 # Log collection
│   ├── routes/                  # Route definitions
│   ├── scheduler/               # Recurring tasks
│   └── startup/                 # Initialization
└── .env
```

## Configuration

| Variable                | Required | Default     | Description              |
| ----------------------- | -------- | ----------- | ------------------------ |
| `AWS_ACCESS_KEY_ID`     | Yes      | -           | AWS S3 access key        |
| `AWS_SECRET_ACCESS_KEY` | Yes      | -           | AWS S3 secret key        |
| `AWS_REGION`            | No       | `us-east-1` | AWS region               |
| `S3_BUCKET`             | Yes      | `kaimz-tdr` | S3 bucket name           |
| `JWT_SECRET`            | Yes      | `TEST`      | JWT signing key          |
| `PORT`                  | No       | `1514`      | Server port              |

## Development

```bash
# Build
go build -o kaimz_agent ./cmd/main.go

# Test endpoints
curl -X POST http://localhost:1514/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"test@kaimz.com","password":"test123"}'

curl -X POST http://localhost:1514/api/logs/upload \
  -H "Content-Type: application/json" \
  -d '{"log":"Test log entry"}'
```

## License

Proprietary - Kaimz Agent Development Project
