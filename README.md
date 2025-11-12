# Kaimz Agent

Cross-platform network log monitoring and S3 storage system with JWT authentication.

## Features

- Cross-platform log collection (Windows/macOS)
- Automatic S3 upload with configurable scheduling
- JWT authentication and RESTful API
- Interactive configuration on startup
- Standalone executable - no external dependencies

## Stack

- **Backend:** Go 1.21+ | Gin Framework
- **Auth:** JWT v5 | bcrypt
- **Cloud:** AWS S3 (SDK v2)
- **Logging:** Native OS commands

## Quick Start (Using Pre-built Executable)

**1. Download the Executable**

Get the latest release for your platform:

- **Windows:** `kaimz_agent.exe`
- **macOS/Linux:** `kaimz_agent`

**2. Create Configuration File**

In the **same directory** as the executable, create a file named `.env`:

```env
# Required: AWS S3 Configuration
AWS_ACCESS_KEY_ID=your_aws_access_key_here
AWS_SECRET_ACCESS_KEY=your_aws_secret_key_here
AWS_REGION=us-east-1
S3_BUCKET=kaimz-tdr

# Optional: Server Configuration
JWT_SECRET=your-super-secret-key
PORT=1514
```

> ⚠️ **Important:** The `.env` file must be in the same directory as the executable!

**3. Run the Executable**

**Windows:**

```cmd
.\kaimz_agent.exe
```

**macOS/Linux:**

```bash
chmod +x kaimz_agent
./kaimz_agent
```

**4. Configure on Startup**

You'll see the Kaimz banner and be prompted:

```
╔═══════════════════════════════════════════════════════════════╗
║                    KAIMZ INC. - AGENT                         ║
╚═══════════════════════════════════════════════════════════════╝

📌 SERVER CONFIGURATION
Server Port [default: 1514]: 8080

📊 LOG COLLECTION SCHEDULE
Fetch Interval (hours) [default: Once on load]: 2
```

- **Port:** Press Enter for default (1514) or type custom port
- **Interval:** Press Enter for one-time fetch, or type hours (e.g., 1, 2, 24)

## Building from Source

**1. Prerequisites**

- Go 1.21 or higher
- Git

**2. Clone and Build**

```bash
# Clone repository
git clone https://github.com/FuJacob/kaimz_tdr.git
cd kaimz_tdr/backend

# Download dependencies
go mod download

# Build for your platform
go build -o kaimz_agent ./cmd/main.go
```

**3. Build for Specific Platforms**

**Windows (from any OS):**

```bash
GOOS=windows GOARCH=amd64 go build -o kaimz_agent.exe ./cmd/main.go
```

**macOS (Intel):**

```bash
GOOS=darwin GOARCH=amd64 go build -o kaimz_agent ./cmd/main.go
```

**macOS (Apple Silicon):**

```bash
GOOS=darwin GOARCH=arm64 go build -o kaimz_agent ./cmd/main.go
```

**Linux:**

```bash
GOOS=linux GOARCH=amd64 go build -o kaimz_agent ./cmd/main.go
```

**4. Create .env Configuration**

Create `.env` file in the **same directory as the executable**:

```bash
# Copy example to .env
cp .env.example .env

# Edit with your AWS credentials
nano .env
```

**5. Run the Executable**

```bash
./kaimz_agent
```

## Directory Structure for Deployment

When deploying the agent, ensure this structure:

```
deployment_folder/
├── kaimz_agent(.exe)    # The executable
└── .env                 # Configuration file (REQUIRED)
```

The agent will:

1. Load configuration from `.env` in the current directory
2. Display startup banner and system info
3. Prompt for port and log schedule
4. Initialize S3 connection
5. Fetch and upload network logs based on schedule
6. Start API server on specified port

## API Endpoints

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

### Example Usage

```bash
# Register user
curl -X POST http://localhost:1514/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"admin@kaimz.com","password":"secure123"}'

# Upload custom log
curl -X POST http://localhost:1514/api/logs/upload \
  -H "Content-Type: application/json" \
  -d '{"log":"Application started successfully"}'

# List all logs in S3
curl http://localhost:1514/api/logs
```

## Configuration Reference

| Variable                | Required | Default     | Description                                      |
| ----------------------- | -------- | ----------- | ------------------------------------------------ |
| `AWS_ACCESS_KEY_ID`     | Yes      | -           | AWS IAM access key for S3                        |
| `AWS_SECRET_ACCESS_KEY` | Yes      | -           | AWS IAM secret key for S3                        |
| `AWS_REGION`            | No       | `us-east-1` | AWS region where S3 bucket is located            |
| `S3_BUCKET`             | Yes      | `kaimz-tdr` | S3 bucket name for log storage                   |
| `JWT_SECRET`            | Yes      | `TEST`      | Secret key for JWT token signing                 |
| `PORT`                  | No       | `1514`      | Server port (Wazuh default, requires sudo <1024) |

## AWS S3 Setup

**1. Create S3 Bucket**

```bash
aws s3 mb s3://kaimz-tdr --region us-east-1
```

**2. Create IAM User with S3 Permissions**

Required permissions:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": ["s3:PutObject", "s3:GetObject", "s3:ListBucket"],
      "Resource": ["arn:aws:s3:::kaimz-tdr/*", "arn:aws:s3:::kaimz-tdr"]
    }
  ]
}
```

**3. Get Access Keys**

Save the `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY` to your `.env` file.

## Troubleshooting

**"Failed to load .env file"**

- Ensure `.env` file is in the same directory as the executable
- Check file name is exactly `.env` (not `env.txt` or `.env.txt`)

**"Failed to initialize S3 client"**

- Verify AWS credentials in `.env` are correct
- Check S3 bucket exists and region is correct
- Ensure IAM user has required S3 permissions

**"Permission denied" (macOS/Linux)**

- Make executable: `chmod +x kaimz_agent`
- For ports < 1024: Run with `sudo ./kaimz_agent`

**"No logs fetched"**

- **macOS:** Ensure Full Disk Access is granted in System Preferences
- **Windows:** Run as Administrator to access Event Logs

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
│   └── startup/                 # Initialization & banner
└── .env
```

## License

Proprietary - Kaimz Inc.
