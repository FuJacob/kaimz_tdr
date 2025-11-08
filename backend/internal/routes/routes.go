package routes

import (
	"github.com/gin-gonic/gin"

	// "backend/internal/auth"
	"backend/internal/handlers"
)

// SetupRoutes configures all application routes
func SetupRoutes(r *gin.Engine, secret []byte) {
	// Health check
	r.GET("/health", func(c *gin.Context) {
		c.JSON(200, gin.H{"status": "ok"})
	})

	// Public routes - Authentication
	authGroup := r.Group("/auth")
	{
		authGroup.POST("/register", handlers.Register(secret))
		authGroup.POST("/login", handlers.Login(secret))
	}

	// Protected routes - require valid JWT
	api := r.Group("/api")
	// api.Use(auth.AuthMiddleware(secret)) // Apply authentication middleware
	{
		// Example protected endpoint - returns user info from JWT
		api.GET("/me", handlers.GetCurrentUser())

		// S3 Log upload endpoints
		api.POST("/logs/upload", handlers.UploadLog())
		api.GET("/logs", handlers.ListLogs())
		
		// Network logs - fetch macOS network logs and upload to S3
		api.POST("/logs/fetch-network", handlers.FetchAndUploadLogs())

		// Any other protected routes go here
		// api.GET("/data", handlers.GetData())
		// api.POST("/action", handlers.DoAction())
	}
}
