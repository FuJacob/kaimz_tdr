package constants

// macOS log command constants
const (
	// MacOSLogStreamCommand is the base command for streaming logs on macOS
	MacOSLogStreamCommand = "log"
	MacOSLogStreamAction  = "stream"
	MacOSLogShowAction    = "show"
	
	// MacOSLogPredicate filters for network-related logs
	MacOSLogPredicate = `subsystem CONTAINS "network"`
	
	// MacOSLogStyle specifies the output format (compact, json, syslog)
	MacOSLogStyleCompact = "compact"
	MacOSLogStyleJSON    = "json"
)

// Windows PowerShell command templates
const (
	// WindowsStreamLogsScript gets recent network events for streaming
	WindowsStreamLogsScript = `Get-WinEvent -FilterHashtable @{LogName='Microsoft-Windows-NetworkProfile/Operational'; StartTime=(Get-Date).AddMinutes(-5)} -MaxEvents 50 | Format-List TimeCreated, Message`
	
	// WindowsFetchLogsScriptTemplate is a template for fetching logs with time parameter
	// Usage: fmt.Sprintf(WindowsFetchLogsScriptTemplate, startTime)
	WindowsFetchLogsScriptTemplate = `
		$startTime = [DateTime]::Parse('%s')
		Get-WinEvent -FilterHashtable @{
			LogName='System';
			StartTime=$startTime
		} -MaxEvents 100 -ErrorAction SilentlyContinue | 
		Where-Object { $_.Message -match 'network|ethernet|wifi|adapter' } |
		Select-Object -First 50 |
		Format-Table TimeCreated, Id, LevelDisplayName, Message -AutoSize |
		Out-String -Width 200
	`
)

// Log fetching limits
const (
	// MaxLogLines is the maximum number of log lines to return for macOS
	MaxLogLines = 100
	
	// WindowsMaxEvents is the maximum number of events to fetch from Windows Event Log
	WindowsMaxEvents = 100
	
	// WindowsMaxResults is the maximum number of filtered results to return
	WindowsMaxResults = 50
)
