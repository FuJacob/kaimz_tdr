package handlers

import (
	"time"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"

	"backend/internal/auth"
	"backend/internal/models"
)

// In-memory user store (replace with database in production)
var users = make(map[string]*models.User)

// Register creates a new user account
func Register(secret []byte) gin.HandlerFunc {
	return func(c *gin.Context) {
		var req models.RegisterRequest
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(400, gin.H{"error": err.Error()})
			return
		}

		// Check if user already exists
		for _, user := range users {
			if user.Email == req.Email {
				c.JSON(400, gin.H{"error": "user with this email already exists"})
				return
			}
		}

		// Hash the password
		hashedPassword, err := auth.HashPassword(req.Password)
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to hash password"})
			return
		}

		// Set default role if not provided
		role := req.Role
		if role == "" {
			role = "user"
		}

		// Create new user
		user := &models.User{
			ID:           uuid.New().String(),
			Email:        req.Email,
			PasswordHash: hashedPassword,
			Role:         role,
			CreatedAt:    time.Now(),
		}

		// Store user (in production, save to database)
		users[user.ID] = user

		// Generate JWT token
		token, err := auth.GenerateJWT(user.ID, user.Email, user.Role, secret)
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to generate token"})
			return
		}

		c.JSON(201, models.AuthResponse{
			Token: token,
			User:  *user,
		})
	}
}

// Login authenticates a user and returns a JWT token
func Login(secret []byte) gin.HandlerFunc {
	return func(c *gin.Context) {
		var req models.LoginRequest
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(400, gin.H{"error": err.Error()})
			return
		}

		// Find user by email (in production, query database)
		var foundUser *models.User
		for _, user := range users {
			if user.Email == req.Email {
				foundUser = user
				break
			}
		}

		if foundUser == nil {
			c.JSON(401, gin.H{"error": "invalid email or password"})
			return
		}

		// Verify password
		if !auth.CheckPasswordHash(req.Password, foundUser.PasswordHash) {
			c.JSON(401, gin.H{"error": "invalid email or password"})
			return
		}

		// Generate JWT token
		token, err := auth.GenerateJWT(foundUser.ID, foundUser.Email, foundUser.Role, secret)
		if err != nil {
			c.JSON(500, gin.H{"error": "failed to generate token"})
			return
		}

		c.JSON(200, models.AuthResponse{
			Token: token,
			User:  *foundUser,
		})
	}
}
