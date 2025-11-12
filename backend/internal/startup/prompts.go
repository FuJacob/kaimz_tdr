package startup

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
)

// PromptForPort prompts the user for a server port or uses default
func PromptForPort(defaultPort string) string {
	fmt.Println("📌 SERVER CONFIGURATION")
	fmt.Println("───────────────────────────────────────────────────────────────")
	reader := bufio.NewReader(os.Stdin)
	fmt.Printf("  Server Port [default: %s]: ", defaultPort)
	portInput, _ := reader.ReadString('\n')
	port := strings.TrimSpace(portInput)
	if port == "" {
		port = defaultPort
	}
	fmt.Printf("  ✓ Using port: %s\n", port)
	fmt.Println()
	return port
}

// PromptForLogInterval prompts the user for log fetch interval
// Returns (interval, runOnce) where runOnce=true means fetch only once on startup
func PromptForLogInterval() (time.Duration, bool) {
	fmt.Println("📊 LOG COLLECTION SCHEDULE")
	fmt.Println("───────────────────────────────────────────────────────────────")
	reader := bufio.NewReader(os.Stdin)
	fmt.Print("  Fetch Interval (hours) [default: Once on load]: ")
	intervalInput, _ := reader.ReadString('\n')
	intervalStr := strings.TrimSpace(intervalInput)

	var fetchInterval time.Duration
	runOnce := true

	if intervalStr != "" {
		hours, err := strconv.ParseFloat(intervalStr, 64)
		if err != nil || hours <= 0 {
			fmt.Printf("  ⚠ Invalid interval '%s', defaulting to once on load\n", intervalStr)
		} else {
			fetchInterval = time.Duration(hours * float64(time.Hour))
			runOnce = false
			fmt.Printf("  ✓ Logs will be fetched every %.2f hours\n", hours)
		}
	} else {
		fmt.Println("  ✓ Logs will be fetched once on server startup")
	}

	fmt.Println()
	return fetchInterval, runOnce
}
