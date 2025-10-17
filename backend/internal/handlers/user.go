package handlers

import (
	"github.com/gin-gonic/gin"
)

// GetCurrentUser returns the current user info from JWT token
func GetCurrentUser() gin.HandlerFunc {
	return func(c *gin.Context) {
		// Extract user info from context (set by AuthMiddleware)
		userID, _ := c.Get("user_id")
		email, _ := c.Get("email")
		role, _ := c.Get("role")

		c.JSON(200, gin.H{
			"user_id": userID,
			"email":   email,
			"role":    role,
		})
	}
}
