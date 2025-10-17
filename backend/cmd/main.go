package main

import (
	"context"
	"log"

	"github.com/gin-gonic/gin"

	"backend/config"
	"backend/internal/aws"
	"backend/internal/handlers"
	"backend/internal/routes"
)

func main() {
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

