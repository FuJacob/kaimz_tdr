package handlers

import (
	"github.com/gin-gonic/gin"

	"backend/internal/aws"
)

// S3 client instance (initialize in main.go)
var s3Client *aws.S3Client

// SetS3Client sets the S3 client for handlers to use
func SetS3Client(client *aws.S3Client) {
	s3Client = client
}

// UploadLogRequest represents the request body for uploading logs
type UploadLogRequest struct {
	Log      string `json:"log" binding:"required"`
	Filename string `json:"filename"` // Optional custom filename
}

// UploadLog handles uploading log strings to S3
func UploadLog() gin.HandlerFunc {
	return func(c *gin.Context) {
		var req UploadLogRequest
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(400, gin.H{"error": err.Error()})
			return
		}

		if s3Client == nil {
			c.JSON(500, gin.H{"error": "S3 client not initialized"})
			return
		}

		var url string
		var err error

		// Use custom filename if provided, otherwise generate timestamped one
		if req.Filename != "" {
			url, err = s3Client.UploadLogWithKey(c.Request.Context(), req.Filename, req.Log)
		} else {
			url, err = s3Client.UploadLog(c.Request.Context(), req.Log)
		}

		if err != nil {
			c.JSON(500, gin.H{"error": err.Error()})
			return
		}

		c.JSON(200, gin.H{
			"message": "log uploaded successfully",
			"url":     url,
		})
	}
}

// ListLogs returns all log files from S3
func ListLogs() gin.HandlerFunc {
	return func(c *gin.Context) {
		if s3Client == nil {
			c.JSON(500, gin.H{"error": "S3 client not initialized"})
			return
		}

		logs, err := s3Client.ListLogs(c.Request.Context())
		if err != nil {
			c.JSON(500, gin.H{"error": err.Error()})
			return
		}

		c.JSON(200, gin.H{
			"logs":  logs,
			"count": len(logs),
		})
	}
}
