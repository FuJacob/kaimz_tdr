package logging

import (
	"bufio"
	"fmt"
	"os/exec"
	"runtime"
	"strings"
	"time"

	"backend/internal/constants"
)

func StreamLogs() {
	var cmd *exec.Cmd

	if runtime.GOOS == "windows" {
		// Windows: Get recent network events (last 5 minutes only)
		cmd = exec.Command("powershell", "-Command", constants.WindowsStreamLogsScript)
	} else {
		// macOS/Linux: Stream network logs
		cmd = exec.Command(
			constants.MacOSLogStreamCommand,
			constants.MacOSLogStreamAction,
			"--predicate", constants.MacOSLogPredicate,
			"--style", constants.MacOSLogStyleCompact,
		)
	}

	stdout, _ := cmd.StdoutPipe()
	cmd.Start()

	scanner := bufio.NewScanner(stdout)
	for scanner.Scan() {
		fmt.Println(scanner.Text())
	}

	cmd.Wait()
}

func FetchLogs(lastPeriodLength int) string {
	var cmd *exec.Cmd
	var output []byte
	var err error

	if runtime.GOOS == "windows" {
		// Windows: Fetch limited network events
		minutesAgo := time.Now().Add(-time.Duration(lastPeriodLength) * time.Minute).Format("2006-01-02T15:04:05")
		psScript := fmt.Sprintf(constants.WindowsFetchLogsScriptTemplate, minutesAgo)

		cmd = exec.Command("powershell", "-Command", psScript)
		output, err = cmd.CombinedOutput()
		
		if err != nil {
			fmt.Printf("Error fetching Windows logs: %v\n", err)
			return ""
		}
	} else {
		// macOS: Fetch logs with compact format and limit output
		period := fmt.Sprintf("%dm", lastPeriodLength)
		cmd = exec.Command(
			constants.MacOSLogStreamCommand,
			constants.MacOSLogShowAction,
			"--predicate", constants.MacOSLogPredicate,
			"--style", constants.MacOSLogStyleCompact,
			"--last", period,
		)
		output, err = cmd.CombinedOutput()
		
		if err != nil {
			fmt.Printf("Error fetching macOS logs: %v\n", err)
			return ""
		}
		
		// Limit macOS logs to configured max lines
		lines := strings.Split(string(output), "\n")
		if len(lines) > constants.MaxLogLines {
			lines = lines[len(lines)-constants.MaxLogLines:]
		}
		output = []byte(strings.Join(lines, "\n"))
	}

	fmt.Printf("Fetched %d bytes of logs from %s (last %d minutes)\n", len(output), runtime.GOOS, lastPeriodLength)
	return string(output)
}
