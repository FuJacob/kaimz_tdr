#!/bin/bash

# Load environment variables and run the server

cd "$(dirname "$0")"

# Load .env file
if [ -f .env ]; then
    export $(cat .env | grep -v '^#' | xargs)
    echo "✅ Loaded environment variables from .env"
else
    echo "❌ .env file not found!"
    exit 1
fi

# Check required variables
if [ -z "$JWT_SECRET" ]; then
    echo "❌ JWT_SECRET is not set in .env"
    exit 1
fi

if [ -z "$S3_BUCKET" ]; then
    echo "⚠️  Warning: S3_BUCKET not set. S3 features will not work."
fi

echo "🚀 Starting Kaimz TDR Server..."
echo "   Port: ${PORT:-8080}"
echo "   S3 Bucket: ${S3_BUCKET:-not set}"
echo "   AWS Region: ${AWS_REGION:-us-east-1}"
echo ""

go run cmd/main.go
