package logging

import (
	"bufio"
	"fmt"
	"os/exec"
	"runtime"
	"time"
)

// StreamLogs streams network logs in real-time based on the OS
func StreamLogs() {
	var cmd *exec.Cmd

	if runtime.GOOS == "windows" {
		// Windows: Use PowerShell to get network events from Event Log
		cmd = exec.Command("powershell", "-Command",
			`Get-WinEvent -FilterHashtable @{LogName='Microsoft-Windows-NetworkProfile/Operational'; StartTime=(Get-Date).AddMinutes(-5)} | Format-List`,
		)
	} else {
		// macOS/Linux: Use log stream command
		cmd = exec.Command(
			"log", "stream",
			"--predicate", `subsystem CONTAINS "network"`,
			"--style", "json",
		)
	}

	stdout, _ := cmd.StdoutPipe()
	cmd.Start()

	scanner := bufio.NewScanner(stdout)
	for scanner.Scan() {
		fmt.Println(scanner.Text()) // each line is a JSON log entry
	}

	cmd.Wait()
}

// FetchLogs fetches network logs for the specified time period based on the OS
func FetchLogs(lastPeriodLength int) string {
	var cmd *exec.Cmd
	var output []byte
	var err error

	if runtime.GOOS == "windows" {
		// Windows: Use PowerShell to fetch network-related events
		// Get events from the last N minutes
		minutesAgo := time.Now().Add(-time.Duration(lastPeriodLength) * time.Minute).Format("2006-01-02T15:04:05")
		
		psScript := fmt.Sprintf(`
			$startTime = [DateTime]::Parse('%s')
			Get-WinEvent -FilterHashtable @{
				LogName='Microsoft-Windows-NetworkProfile/Operational','System';
				StartTime=$startTime
			} -ErrorAction SilentlyContinue | 
			Where-Object { $_.Message -match 'network|ethernet|wifi|adapter' } |
			Format-List TimeCreated, Id, LevelDisplayName, Message |
			Out-String
		`, minutesAgo)

		cmd = exec.Command("powershell", "-Command", psScript)
		output, err = cmd.CombinedOutput()
		
		if err != nil {
			fmt.Printf("Error fetching Windows logs: %v\n", err)
			// Fallback: Try getting recent System events
			fallbackCmd := exec.Command("powershell", "-Command",
				fmt.Sprintf(`Get-EventLog -LogName System -Newest 100 -After (Get-Date).AddMinutes(-%d) | Format-List`, lastPeriodLength),
			)
			output, err = fallbackCmd.CombinedOutput()
			if err != nil {
				fmt.Printf("Error fetching fallback logs: %v\n", err)
				return ""
			}
		}
	} else {
		// macOS/Linux: Use log show command
		period := fmt.Sprintf("%dm", lastPeriodLength)
		cmd = exec.Command("log", "show", 
			"--predicate", `subsystem CONTAINS "network"`,
			"--info", "--last", period)
		output, err = cmd.CombinedOutput()
		
		if err != nil {
			fmt.Printf("Error fetching macOS logs: %v\n", err)
			return ""
		}
	}

	fmt.Printf("Fetched logs from %s (last %d minutes)\n", runtime.GOOS, lastPeriodLength)
	return string(output)
}
