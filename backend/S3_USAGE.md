# S3 Log Upload API

## Quick Start

### 1. Setup Environment

```bash
# Copy and edit .env file
cp .env.example .env

# Edit .env and set:
# - JWT_SECRET (generate with: openssl rand -base64 32)
# - S3_BUCKET (your S3 bucket name)
# - AWS_REGION (e.g., us-east-1)
```

### 2. Configure AWS Credentials

Option A - AWS CLI (recommended):
```bash
aws configure
```

Option B - Environment variables:
```bash
export AWS_ACCESS_KEY_ID=your-key
export AWS_SECRET_ACCESS_KEY=your-secret
export AWS_REGION=us-east-1
```

### 3. Run Server

```bash
# Load environment variables
export $(cat .env | xargs)

# Run server
go run cmd/main.go
```

## API Endpoints

### Register & Login

```bash
# Register
curl -X POST http://localhost:8080/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "email": "user@example.com",
    "password": "password123"
  }'

# Save the token from response
TOKEN="eyJhbGc..."
```

### Upload Log to S3

```bash
# Upload log with auto-generated timestamp filename
curl -X POST http://localhost:8080/api/logs/upload \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "log": "This is my log message\nLine 2 of log\nLine 3 of log"
  }'

# Response:
# {
#   "message": "log uploaded successfully",
#   "url": "s3://your-bucket/logs/2025-10-17_14-30-45.log"
# }
```

```bash
# Upload log with custom filename
curl -X POST http://localhost:8080/api/logs/upload \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "log": "Error occurred in service X",
    "filename": "logs/errors/service-x-error.log"
  }'
```

### List All Logs

```bash
curl http://localhost:8080/api/logs \
  -H "Authorization: Bearer $TOKEN"

# Response:
# {
#   "count": 3,
#   "logs": [
#     "logs/2025-10-17_14-30-45.log",
#     "logs/2025-10-17_14-31-22.log",
#     "logs/errors/service-x-error.log"
#   ]
# }
```

## Complete Example

```bash
#!/bin/bash

# 1. Register user
RESPONSE=$(curl -s -X POST http://localhost:8080/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com","password":"pass123"}')

# 2. Extract token
TOKEN=$(echo $RESPONSE | jq -r '.token')
echo "Token: $TOKEN"

# 3. Upload a log
LOG_CONTENT="[2025-10-17 14:30:45] INFO: Application started
[2025-10-17 14:30:46] INFO: Connected to database
[2025-10-17 14:30:47] ERROR: Failed to connect to external API"

curl -X POST http://localhost:8080/api/logs/upload \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"log\": \"$LOG_CONTENT\"}" | jq .

# 4. List all logs
curl -s http://localhost:8080/api/logs \
  -H "Authorization: Bearer $TOKEN" | jq .
```

## S3 Bucket Setup

Make sure your S3 bucket exists and your AWS credentials have permissions:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "s3:PutObject",
        "s3:GetObject",
        "s3:ListBucket"
      ],
      "Resource": [
        "arn:aws:s3:::your-bucket-name/*",
        "arn:aws:s3:::your-bucket-name"
      ]
    }
  ]
}
```

## Code Usage Example

```go
// In your handler or service
ctx := context.Background()
s3Client, _ := aws.NewS3Client(ctx, "us-east-1", "my-bucket")

// Upload log
logContent := "This is a log message"
url, err := s3Client.UploadLog(ctx, logContent)
// Returns: "s3://my-bucket/logs/2025-10-17_14-30-45.log"

// Upload with custom key
url, err := s3Client.UploadLogWithKey(ctx, "custom/path/mylog.log", logContent)
// Returns: "s3://my-bucket/custom/path/mylog.log"
```

## Notes

- Logs are uploaded to `logs/` prefix by default
- Filename format: `YYYY-MM-DD_HH-MM-SS.log`
- All endpoints except `/auth/*` require JWT authentication
- S3 client gracefully handles missing bucket configuration (logs warning)
