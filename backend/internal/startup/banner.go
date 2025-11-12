package startup

import (
	"fmt"
	"runtime"
)

// PrintBanner displays the startup banner with system information
func PrintBanner() {
	banner := `
╔════════════════════════════════════════════════════════════════╗
║                                                                ║
║ ██╗  ██╗ █████╗ ██╗███╗   ███╗███████╗   ██╗███╗   ██╗ ██████╗ ║
║ ██║ ██╔╝██╔══██╗██║████╗ ████║╚══███╔╝   ██║████╗  ██║██╔════╝ ║
║ █████╔╝ ███████║██║██╔████╔██║  ███╔╝    ██║██╔██╗ ██║██║      ║
║ ██╔═██╗ ██╔══██║██║██║╚██╔╝██║ ███╔╝     ██║██║╚██╗██║██║      ║
║ ██║  ██╗██║  ██║██║██║ ╚═╝ ██║███████╗   ██║██║ ╚████║╚██████╗ ║
║ ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚═╝     ╚═╝╚══════╝   ╚═╝╚═╝  ╚═══╝ ╚═════╝ ║
║                                                                ║
║                                                                ║
║                                                                ║
║                                                                ║
║              A G E N T   -   L o g   M o n i t o r             ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝
`
	fmt.Println(banner)

	// System information
	fmt.Printf("  System: %s/%s\n", runtime.GOOS, runtime.GOARCH)
	fmt.Printf("  Go Version: %s\n", runtime.Version())
	fmt.Println("  Cross-platform network log monitoring with AWS S3")
	fmt.Println()
	fmt.Println("═══════════════════════════════════════════════════════════════")
	fmt.Println()
}
