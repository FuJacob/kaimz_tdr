package main

import (
	"fmt"
	"log"

	"backend/config"
	"backend/internal/routes"
	"backend/internal/startup"

	"github.com/gin-gonic/gin"
	"github.com/joho/godotenv"
)

func main() {
	// Display startup banner
	startup.PrintBanner()

	// Load .env file
	if err := godotenv.Load(); err != nil {
		fmt.Println("⚠ .env file not found, using system environment variables")
	} else {
		fmt.Println("✓ Loaded environment configuration")
	}
	fmt.Println()

	// Load configuration
	cfg := config.Load()

	// Prompt for server port
	port := startup.PromptForPort(cfg.Port)

	// Prompt for log fetch interval
	interval, runOnce := startup.PromptForLogInterval()

	// Initialize S3 and start log scheduler
	startup.InitializeS3AndScheduler(cfg, interval, runOnce)

	// Initialize Gin router
	r := gin.Default()

	// Setup routes (public + protected)
	routes.SetupRoutes(r, []byte(cfg.JWTSecret))

	// Start server
	fmt.Println("═══════════════════════════════════════════════════════════════")
	log.Printf("🚀 Server running on port %s", port)
	fmt.Println("═══════════════════════════════════════════════════════════════")
	if err := r.Run(":" + port); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}
