package startup

import (
	"context"
	"log"

	"backend/config"
	"backend/internal/aws"
	"backend/internal/handlers"
	"backend/internal/scheduler"
	"time"
)

// InitializeS3AndScheduler initializes S3 client and starts log scheduler
func InitializeS3AndScheduler(cfg *config.Config, interval time.Duration, runOnce bool) {
	if cfg.S3Bucket == "" {
		return
	}

	ctx := context.Background()
	s3Client, err := aws.NewS3Client(ctx, cfg.AWSRegion, cfg.S3Bucket)
	if err != nil {
		log.Printf("Warning: Failed to initialize S3 client: %v", err)
		return
	}

	// Set S3 client for API handlers
	handlers.SetS3Client(s3Client)
	log.Printf("S3 client initialized for bucket: %s", cfg.S3Bucket)

	// Start log fetching scheduler
	scheduler.StartLogScheduler(ctx, s3Client, interval, runOnce)
}
