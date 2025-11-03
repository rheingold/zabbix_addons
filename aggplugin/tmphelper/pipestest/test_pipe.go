package main

import (
	"fmt"
	"time"

	"github.com/natefinch/npipe"
)

func main() {
	pipePath := `\\.\pipe\agent.plugin.sock`
	fmt.Printf("Attempting to connect to: %s\n", pipePath)

	conn, err := npipe.DialTimeout(pipePath, 5*time.Second)
	if err != nil {
		fmt.Printf("ERROR: %v\n", err)
		fmt.Printf("Error type: %T\n", err)
		return
	}

	fmt.Printf("SUCCESS! Connected to pipe\n")
	conn.Close()
}
