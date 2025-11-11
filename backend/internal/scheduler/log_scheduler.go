package scheduler

import (
	"context"
	"log"
	"time"

	"backend/internal/aws"
	"backend/internal/logging"
)

// FetchAndUploadLogs fetches network logs and uploads them to S3
func FetchAndUploadLogs(ctx context.Context, s3Client *aws.S3Client) {
	log.Println("🔄 Fetching network logs...")
	logs := logging.FetchLogs(60) // Fetch last 60 minutes of logs

	if logs != "" {
		url, err := s3Client.UploadLog(ctx, logs)
		if err != nil {
			log.Printf("❌ Failed to upload logs to S3: %v", err)
		} else {
			log.Printf("✅ Logs uploaded to S3: %s", url)
		}
	} else {
		log.Println("⚠️  No logs fetched")
	}
}

// StartLogScheduler starts the log fetching routine
// If runOnce is true, fetches logs once and stops
// If runOnce is false, fetches logs immediately then every interval
func StartLogScheduler(ctx context.Context, s3Client *aws.S3Client, interval time.Duration, runOnce bool) {
	// Fetch logs immediately on startup
	go FetchAndUploadLogs(ctx, s3Client)

	// If recurring, start a ticker
	if !runOnce {
		go func() {
			ticker := time.NewTicker(interval)
			defer ticker.Stop()

			for range ticker.C {
				FetchAndUploadLogs(ctx, s3Client)
			}
		}()
	}
}
