package handlers

import (
	"github.com/gin-gonic/gin"

	"backend/internal/logging"
)

// FetchAndUploadLogsRequest represents the request to fetch and upload logs
type FetchAndUploadLogsRequest struct {
	LastMinutes int    `json:"last_minutes"` // How many minutes of logs to fetch (default: 5)
	Filename    string `json:"filename"`     // Optional custom S3 filename
}

// FetchAndUploadLogs fetches macOS network logs and uploads them to S3
func FetchAndUploadLogs() gin.HandlerFunc {
	return func(c *gin.Context) {
		var req FetchAndUploadLogsRequest
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(400, gin.H{"error": err.Error()})
			return
		}

		// Default to 5 minutes if not specified
		if req.LastMinutes == 0 {
			req.LastMinutes = 5
		}

		// Fetch the logs from macOS
		logContent := logging.FetchLogs(req.LastMinutes)
		if logContent == "" {
			c.JSON(500, gin.H{"error": "failed to fetch logs or no logs available"})
			return
		}

		if s3Client == nil {
			c.JSON(500, gin.H{"error": "S3 client not initialized"})
			return
		}

		var url string
		var err error

		// Upload to S3 with custom filename or auto-generated one
		if req.Filename != "" {
			url, err = s3Client.UploadLogWithKey(c.Request.Context(), req.Filename, logContent)
		} else {
			url, err = s3Client.UploadLog(c.Request.Context(), logContent)
		}

		if err != nil {
			c.JSON(500, gin.H{"error": err.Error()})
			return
		}

		c.JSON(200, gin.H{
			"message":      "network logs fetched and uploaded successfully",
			"url":          url,
			"last_minutes": req.LastMinutes,
			"log_size":     len(logContent),
		})
	}
}

// StreamLogsSSE streams macOS network logs in real-time using Server-Sent Events
func StreamLogsSSE() gin.HandlerFunc {
	return func(c *gin.Context) {
		c.Header("Content-Type", "text/event-stream")
		c.Header("Cache-Control", "no-cache")
		c.Header("Connection", "keep-alive")

		// Note: This is a simple implementation. In production, you'd want:
		// - Proper goroutine management
		// - Client disconnect handling
		// - Rate limiting

		c.JSON(200, gin.H{
			"message": "Streaming not yet implemented - use POST /api/logs/fetch-network instead",
			"note":    "For streaming, consider WebSocket implementation",
		})
	}
}
