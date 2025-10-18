package config

import (
	"log"
	"os"
)

type Config struct {
	JWTSecret string
	Port      string
	AWSRegion string
	S3Bucket  string
}

// Load reads configuration from environment variables
func Load() *Config {
	jwtSecret := os.Getenv("JWT_SECRET")
	if jwtSecret == "" {
		log.Fatal("JWT_SECRET environment variable is required")
	}

	port := os.Getenv("PORT")
	if port == "" {
		port = "8080" // default port
	}

	awsRegion := os.Getenv("AWS_REGION")
	if awsRegion == "" {
		awsRegion = "us-east-1" // default region
	}

	s3Bucket := os.Getenv("S3_BUCKET")
	if s3Bucket == "" {
		s3Bucket = "kaimz-tdr"
	}

	return &Config{
		JWTSecret: jwtSecret,
		Port:      port,
		AWSRegion: awsRegion,
		S3Bucket:  s3Bucket,
	}
}
