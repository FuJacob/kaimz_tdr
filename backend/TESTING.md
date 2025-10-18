# Manual Testing Guide - No Frontend Required

## Quick Start

### 1. Start the Server

```bash
cd backend
export $(cat .env | xargs)
go run cmd/main.go
```

### 2. Run Automated Test Script

```bash
./test_api.sh
```

This will:

- Register/login a user
- Save JWT token to `.test_token`
- Test all endpoints
- Show you manual testing commands

---

## Manual Testing with cURL

### Step 1: Get JWT Token

**Register a new user:**

```bash
curl -X POST http://localhost:8080/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com","password":"pass123"}' | jq .
```

**Or login:**

```bash
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"test@example.com","password":"pass123"}' | jq .
```

**Copy the token from response and export it:**

```bash
export TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
```

Or use the saved token:

```bash
export TOKEN=$(cat .test_token)
```

### Step 2: Test Endpoints

**Get current user info:**

```bash
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:8080/api/me | jq .
```

**Upload custom log:**

```bash
curl -X POST http://localhost:8080/api/logs/upload \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"log":"My custom log content"}' | jq .
```

**Fetch macOS network logs (5 minutes):**

```bash
curl -X POST http://localhost:8080/api/logs/fetch-network \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"last_minutes":5}' | jq .
```

**Fetch network logs (custom time + filename):**

```bash
curl -X POST http://localhost:8080/api/logs/fetch-network \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"last_minutes":10,"filename":"logs/network/my-test.log"}' | jq .
```

**List all logs in S3:**

```bash
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:8080/api/logs | jq .
```

---

## Testing with Postman or Thunder Client

### Setup

1. **Import as Collection** or test individually
2. **Set Authorization:**
   - Type: `Bearer Token`
   - Token: (paste your JWT from login/register response)

### Endpoints

| Method | URL                       | Body (JSON)                                         | Description                  |
| ------ | ------------------------- | --------------------------------------------------- | ---------------------------- |
| POST   | `/auth/register`          | `{"email":"test@example.com","password":"pass123"}` | Register user                |
| POST   | `/auth/login`             | `{"email":"test@example.com","password":"pass123"}` | Login user                   |
| GET    | `/api/me`                 | -                                                   | Get current user (protected) |
| POST   | `/api/logs/upload`        | `{"log":"content"}`                                 | Upload custom log            |
| POST   | `/api/logs/fetch-network` | `{"last_minutes":5}`                                | Fetch & upload network logs  |
| GET    | `/api/logs`               | -                                                   | List all logs                |

---

## Using HTTPie (Alternative to cURL)

Install: `brew install httpie`

```bash
# Login and save token
http POST localhost:8080/auth/login email=test@example.com password=pass123 | jq -r '.token' > .test_token
TOKEN=$(cat .test_token)

# Test endpoints
http localhost:8080/api/me "Authorization:Bearer $TOKEN"
http POST localhost:8080/api/logs/fetch-network "Authorization:Bearer $TOKEN" last_minutes:=5
```

---

## Debugging

**Check if server is running:**

```bash
curl http://localhost:8080/health
```

**Test without token (should fail with 401):**

```bash
curl http://localhost:8080/api/me
# Response: {"error":"missing authorization header"}
```

**Verify token format:**

```bash
echo $TOKEN | cut -d'.' -f1 | base64 -d
# Should show header like: {"alg":"HS256","typ":"JWT"}
```

---

## Common Issues

**"missing authorization header"**

- Make sure you're using: `Authorization: Bearer <token>`
- Check that TOKEN variable is set: `echo $TOKEN`

**"invalid or expired token"**

- Token expires after 24 hours
- Login again to get a new token

**"S3 client not initialized"**

- Check `.env` has `S3_BUCKET` set
- Verify AWS credentials in `~/.aws/credentials` with `[kaimz-tdr]` profile

**"failed to fetch logs"**

- macOS `log` command requires proper permissions
- Run server with appropriate permissions or use `sudo` if needed
