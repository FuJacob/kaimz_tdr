#!/bin/bash

# JWT Authentication API Test Script
# This script demonstrates the complete authentication flow

BASE_URL="http://localhost:8080"

echo "🚀 Testing JWT Authentication API"
echo "=================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 1. Health Check
echo -e "${BLUE}1. Testing health endpoint...${NC}"
curl -s "$BASE_URL/health" | jq .
echo -e "\n"

# 2. Register a new user
echo -e "${BLUE}2. Registering a new user...${NC}"
REGISTER_RESPONSE=$(curl -s -X POST "$BASE_URL/auth/register" \
  -H "Content-Type: application/json" \
  -d '{
    "email": "testuser@example.com",
    "password": "securepass123",
    "role": "user"
  }')

echo "$REGISTER_RESPONSE" | jq .
TOKEN=$(echo "$REGISTER_RESPONSE" | jq -r '.token')
echo -e "${GREEN}Token saved: ${TOKEN:0:50}...${NC}"
echo -e "\n"

# 3. Try to register same user again (should fail)
echo -e "${BLUE}3. Trying to register same email again (should fail)...${NC}"
curl -s -X POST "$BASE_URL/auth/register" \
  -H "Content-Type: application/json" \
  -d '{
    "email": "testuser@example.com",
    "password": "securepass123"
  }' | jq .
echo -e "\n"

# 4. Login with the created user
echo -e "${BLUE}4. Logging in with registered user...${NC}"
LOGIN_RESPONSE=$(curl -s -X POST "$BASE_URL/auth/login" \
  -H "Content-Type: application/json" \
  -d '{
    "email": "testuser@example.com",
    "password": "securepass123"
  }')

echo "$LOGIN_RESPONSE" | jq .
TOKEN=$(echo "$LOGIN_RESPONSE" | jq -r '.token')
echo -e "${GREEN}New token from login: ${TOKEN:0:50}...${NC}"
echo -e "\n"

# 5. Try login with wrong password (should fail)
echo -e "${BLUE}5. Trying to login with wrong password (should fail)...${NC}"
curl -s -X POST "$BASE_URL/auth/login" \
  -H "Content-Type: application/json" \
  -d '{
    "email": "testuser@example.com",
    "password": "wrongpassword"
  }' | jq .
echo -e "\n"

# 6. Access protected route without token (should fail)
echo -e "${BLUE}6. Accessing protected route without token (should fail)...${NC}"
curl -s "$BASE_URL/api/profile" | jq .
echo -e "\n"

# 7. Access protected route with valid token
echo -e "${BLUE}7. Accessing protected route WITH valid token...${NC}"
curl -s "$BASE_URL/api/profile" \
  -H "Authorization: Bearer $TOKEN" | jq .
echo -e "\n"

# 8. Test another protected endpoint
echo -e "${BLUE}8. Testing another protected endpoint...${NC}"
curl -s "$BASE_URL/api/protected" \
  -H "Authorization: Bearer $TOKEN" | jq .
echo -e "\n"

# 9. Register an admin user
echo -e "${BLUE}9. Registering an admin user...${NC}"
ADMIN_RESPONSE=$(curl -s -X POST "$BASE_URL/auth/register" \
  -H "Content-Type: application/json" \
  -d '{
    "email": "admin@example.com",
    "password": "adminpass123",
    "role": "admin"
  }')

echo "$ADMIN_RESPONSE" | jq .
ADMIN_TOKEN=$(echo "$ADMIN_RESPONSE" | jq -r '.token')
echo -e "${GREEN}Admin token: ${ADMIN_TOKEN:0:50}...${NC}"
echo -e "\n"

# 10. Try to access admin route as regular user (should fail)
echo -e "${BLUE}10. Trying to access admin route as regular user (should fail)...${NC}"
curl -s "$BASE_URL/api/users" \
  -H "Authorization: Bearer $TOKEN" | jq .
echo -e "\n"

# 11. Access admin route as admin
echo -e "${BLUE}11. Accessing admin route as admin user...${NC}"
curl -s "$BASE_URL/api/users" \
  -H "Authorization: Bearer $ADMIN_TOKEN" | jq .
echo -e "\n"

# 12. Update profile
echo -e "${BLUE}12. Updating user profile...${NC}"
curl -s -X PUT "$BASE_URL/api/profile" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "name": "John Doe",
    "bio": "Software developer"
  }' | jq .
echo -e "\n"

echo -e "${GREEN}✅ All tests completed!${NC}"
echo ""
echo "Available tokens for manual testing:"
echo -e "${GREEN}User Token:${NC} $TOKEN"
echo -e "${GREEN}Admin Token:${NC} $ADMIN_TOKEN"
