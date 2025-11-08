package main

import (
	"context"
	"log"

	"github.com/gin-gonic/gin"
	"github.com/joho/godotenv"

	"backend/config"
	"backend/internal/aws"
	"backend/internal/handlers"
	"backend/internal/logging"
	"backend/internal/routes"
)

func main() {
	// Load .env file (like in Express.js)
	if err := godotenv.Load(); err != nil {
		log.Println("Warning: .env file not found, using system environment variables")
	} else {
		log.Println("✅ Loaded .env file")
	}

	// Load configuration
	cfg := config.Load()

	// Initialize S3 client if bucket is configured
	if cfg.S3Bucket != "" {
		ctx := context.Background()
		s3Client, err := aws.NewS3Client(ctx, cfg.AWSRegion, cfg.S3Bucket)
		if err != nil {
			log.Printf("Warning: Failed to initialize S3 client: %v", err)
		} else {
			handlers.SetS3Client(s3Client)
			log.Printf("S3 client initialized for bucket: %s", cfg.S3Bucket)

			// Fetch and upload logs on startup (once)
			log.Println("🔄 Fetching network logs on startup...")
			logs := logging.FetchLogs(60) // Fetch last 60 minutes of logs
			
			if logs != "" {
				url, err := s3Client.UploadLog(ctx, logs)
				if err != nil {
					log.Printf("❌ Failed to upload startup logs to S3: %v", err)
				} else {
					log.Printf("✅ Startup logs uploaded to S3: %s", url)
				}
			} else {
				log.Println("⚠️  No logs fetched on startup")
			}
		}
	}

	// Initialize Gin router
	r := gin.Default()

	// Setup routes (public + protected)
	routes.SetupRoutes(r, []byte(cfg.JWTSecret))

	// Start server
	log.Printf("Server starting on port %s", cfg.Port)
	if err := r.Run(":" + cfg.Port); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}
