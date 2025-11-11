package main

import (
	"log"

	"backend/config"
	"backend/internal/routes"
	"backend/internal/startup"

	"github.com/gin-gonic/gin"
)

func main() {
	// Load configuration
	cfg := config.Load()

	// Prompt for server port
	port := startup.PromptForPort(cfg.Port)
	log.Printf("Using port: %s", port)

	// Prompt for log fetch interval
	interval, runOnce := startup.PromptForLogInterval()

	// Initialize S3 and start log scheduler
	startup.InitializeS3AndScheduler(cfg, interval, runOnce)

	// Initialize Gin router
	r := gin.Default()

	// Setup routes (public + protected)
	routes.SetupRoutes(r, []byte(cfg.JWTSecret))

	// Start server
	log.Printf("Server starting on port %s", port)
	if err := r.Run(":" + port); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}
