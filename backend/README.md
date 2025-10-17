# Backend API with JWT Authentication

A Go backend API built with Gin framework featuring JWT-based authentication.

## Features

- ✅ User registration and login
- ✅ JWT token generation and validation
- ✅ Password hashing with bcrypt
- ✅ Protected routes with middleware
- ✅ Role-based access control example
- ✅ In-memory user storage (replace with database in production)

## Project Structure

```
backend/
├── cmd/
│   └── main.go              # Application entry point
├── config/
│   └── config.go            # Configuration management
├── internal/
│   ├── auth/
│   │   ├── jwt.go           # JWT token generation
│   │   ├── middleware.go    # Authentication middleware
│   │   └── password.go      # Password hashing utilities
│   ├── handlers/
│   │   ├── auth.go          # Auth handlers (register, login)
│   │   └── user.go          # User handlers (profile, etc.)
│   ├── models/
│   │   └── user.go          # Data models
│   └── routes/
│       └── routes.go        # Route definitions
├── .env.example             # Example environment variables
└── go.mod                   # Go module dependencies
```

## Setup

### 1. Install Dependencies

```bash
cd backend
go mod download
```

### 2. Configure Environment

```bash
# Copy example env file
cp .env.example .env

# Edit .env and set a strong JWT_SECRET
# You can generate a random secret with:
openssl rand -base64 32
```

### 3. Run the Server

```bash
# Export environment variables
export $(cat .env | xargs)

# Run the server
go run cmd/main.go
```

Server will start on `http://localhost:8080`

## API Endpoints

### Public Endpoints (No Authentication Required)

#### Health Check
```bash
GET /health
```

#### Register
```bash
POST /auth/register
Content-Type: application/json

{
  "email": "user@example.com",
  "password": "password123",
  "role": "user"  // optional, defaults to "user"
}
```

**Response:**
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "user": {
    "id": "uuid",
    "email": "user@example.com",
    "role": "user",
    "created_at": "2025-10-16T..."
  }
}
```

#### Login
```bash
POST /auth/login
Content-Type: application/json

{
  "email": "user@example.com",
  "password": "password123"
}
```

**Response:**
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "user": {
    "id": "uuid",
    "email": "user@example.com",
    "role": "user",
    "created_at": "2025-10-16T..."
  }
}
```

### Protected Endpoints (Require JWT Token)

All protected endpoints require the `Authorization` header:
```
Authorization: Bearer <your-jwt-token>
```

#### Get Profile
```bash
GET /api/profile
Authorization: Bearer <token>
```

**Response:**
```json
{
  "user_id": "uuid",
  "email": "user@example.com",
  "role": "user"
}
```

#### Update Profile
```bash
PUT /api/profile
Authorization: Bearer <token>
Content-Type: application/json

{
  "name": "John Doe"
}
```

#### Protected Example
```bash
GET /api/protected
Authorization: Bearer <token>
```

#### Get All Users (Admin Only)
```bash
GET /api/users
Authorization: Bearer <token>
```
*Note: Only works if the JWT role is "admin"*

## Testing with cURL

### 1. Register a new user
```bash
curl -X POST http://localhost:8080/auth/register \
  -H "Content-Type: application/json" \
  -d '{
    "email": "test@example.com",
    "password": "password123"
  }'
```

### 2. Save the token
```bash
# Copy the token from response and save it
TOKEN="eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
```

### 3. Access protected route
```bash
curl http://localhost:8080/api/profile \
  -H "Authorization: Bearer $TOKEN"
```

### 4. Test without token (should fail)
```bash
curl http://localhost:8080/api/profile
# Response: {"error":"missing authorization header"}
```

## Testing with Postman/Thunder Client

1. **Register:** POST to `http://localhost:8080/auth/register` with JSON body
2. **Copy token** from response
3. **Add Authorization header** to protected requests:
   - Type: Bearer Token
   - Token: (paste your JWT)
4. **Make requests** to `/api/*` endpoints

## Production Considerations

⚠️ **Current implementation uses in-memory storage** - users are lost when server restarts.

### Before deploying to production:

1. **Add a database:**
   - PostgreSQL, MySQL, or MongoDB
   - Implement proper user repository pattern
   - Add database migrations

2. **Security enhancements:**
   - Use environment variables for secrets (never commit `.env`)
   - Implement refresh tokens (short-lived access + long-lived refresh)
   - Add token blacklist for logout
   - Rate limiting on auth endpoints
   - HTTPS only in production
   - CORS configuration
   - Input validation and sanitization

3. **Additional features:**
   - Email verification
   - Password reset flow
   - Two-factor authentication
   - Account lockout after failed attempts
   - Audit logging

4. **Monitoring:**
   - Add structured logging
   - Metrics and monitoring
   - Error tracking

## Environment Variables

| Variable | Description | Default | Required |
|----------|-------------|---------|----------|
| `JWT_SECRET` | Secret key for signing JWT tokens | - | ✅ Yes |
| `PORT` | Server port | 8080 | No |

## Dependencies

- `github.com/gin-gonic/gin` - Web framework
- `github.com/golang-jwt/jwt/v5` - JWT implementation
- `golang.org/x/crypto/bcrypt` - Password hashing
- `github.com/google/uuid` - UUID generation

## License

MIT
