package logging

import (
	"bufio"
	"fmt"
	"os/exec"
)

func StreamLogs() {
	cmd := exec.Command(
		"log", "stream",
		"--predicate", `subsystem CONTAINS "network"`,
		"--style", "json",
	)

	stdout, _ := cmd.StdoutPipe()
	cmd.Start()

	scanner := bufio.NewScanner(stdout)
	for scanner.Scan() {
		fmt.Println(scanner.Text()) // each line is a JSON log entry
	}

	cmd.Wait()
}

func FetchLogs(lastPeriodLength int) string {
	period := fmt.Sprintf("%dm", lastPeriodLength)
	cmd := exec.Command("log", "show", "--predicate", `subsystem CONTAINS "network"`,
		"--info", "--last", period)
	output, err := cmd.CombinedOutput()
	if err != nil {
		fmt.Println("error fetching logs", err)
		return ``
	}
	fmt.Println("LOGS:", output)
	return string(output)
}
