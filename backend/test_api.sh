#!/bin/bash

# Test script for Kaimz TDR API with JWT authentication
# This script demonstrates how to test protected endpoints without a frontend

set -e  # Exit on error

BASE_URL="http://localhost:8080"
EMAIL="test@example.com"
PASSWORD="testpass123"

echo "🚀 Kaimz TDR API Test Script"
echo "============================"
echo ""

# Check if jq is installed
if ! command -v jq &> /dev/null; then
    echo "⚠️  jq is not installed. Install with: brew install jq"
    echo "Continuing without pretty printing..."
    JQ_CMD="cat"
else
    JQ_CMD="jq ."
fi

# 1. Health Check
echo "📡 Step 1: Health Check"
curl -s "$BASE_URL/health" | eval $JQ_CMD
echo -e "\n"

# 2. Register a new user
echo "📝 Step 2: Register User"
echo "Email: $EMAIL"
REGISTER_RESPONSE=$(curl -s -X POST "$BASE_URL/auth/register" \
  -H "Content-Type: application/json" \
  -d "{\"email\":\"$EMAIL\",\"password\":\"$PASSWORD\"}")

echo "$REGISTER_RESPONSE" | eval $JQ_CMD

# Extract token
TOKEN=$(echo "$REGISTER_RESPONSE" | jq -r '.token' 2>/dev/null || echo "")

if [ -z "$TOKEN" ] || [ "$TOKEN" == "null" ]; then
    echo "❌ Failed to get token from registration. User might already exist."
    echo ""
    echo "📝 Step 2b: Try Login Instead"
    LOGIN_RESPONSE=$(curl -s -X POST "$BASE_URL/auth/login" \
      -H "Content-Type: application/json" \
      -d "{\"email\":\"$EMAIL\",\"password\":\"$PASSWORD\"}")
    
    echo "$LOGIN_RESPONSE" | eval $JQ_CMD
    TOKEN=$(echo "$LOGIN_RESPONSE" | jq -r '.token' 2>/dev/null || echo "")
fi

if [ -z "$TOKEN" ] || [ "$TOKEN" == "null" ]; then
    echo "❌ Failed to obtain JWT token. Exiting."
    exit 1
fi

echo -e "\n✅ Token obtained: ${TOKEN:0:50}...\n"

# Save token to file for manual testing
echo "$TOKEN" > .test_token
echo "💾 Token saved to .test_token"
echo ""

# 3. Test protected endpoint - Get current user
echo "👤 Step 3: Get Current User Info (Protected)"
curl -s "$BASE_URL/api/me" \
  -H "Authorization: Bearer $TOKEN" | eval $JQ_CMD
echo -e "\n"

# 4. Upload a simple log
echo "📤 Step 4: Upload Simple Log to S3"
curl -s -X POST "$BASE_URL/api/logs/upload" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"log":"Test log from script\nLine 2\nLine 3"}' | eval $JQ_CMD
echo -e "\n"

# 5. Fetch and upload network logs
echo "🌐 Step 5: Fetch macOS Network Logs and Upload to S3"
echo "Fetching last 5 minutes of network logs..."
curl -s -X POST "$BASE_URL/api/logs/fetch-network" \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"last_minutes":5}' | eval $JQ_CMD
echo -e "\n"

# 6. List all logs
echo "📋 Step 6: List All Logs in S3"
curl -s "$BASE_URL/api/logs" \
  -H "Authorization: Bearer $TOKEN" | eval $JQ_CMD
echo -e "\n"

# 7. Test without token (should fail)
echo "🔒 Step 7: Test Protected Endpoint WITHOUT Token (should fail)"
curl -s "$BASE_URL/api/me" | eval $JQ_CMD
echo -e "\n"

echo "✅ All tests completed!"
echo ""
echo "📝 Manual Testing Commands:"
echo "-------------------------"
echo "Export token for manual use:"
echo "  export TOKEN='$TOKEN'"
echo ""
echo "Or load from file:"
echo "  export TOKEN=\$(cat .test_token)"
echo ""
echo "Test endpoints manually:"
echo "  curl -H \"Authorization: Bearer \$TOKEN\" $BASE_URL/api/me"
echo "  curl -X POST -H \"Authorization: Bearer \$TOKEN\" -H \"Content-Type: application/json\" \\"
echo "    -d '{\"last_minutes\":10}' $BASE_URL/api/logs/fetch-network"
echo ""
