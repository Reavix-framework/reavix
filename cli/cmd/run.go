package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	
	"github.com/spf13/cobra"
)

var runCmd = &cobra.Command{
	Use: "run",
	Short: "Run Reavix application",
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("Starting production server...")

		cmdRun := exec.Command(filepath.Join(".","reavix-app"))
		cmdRun.Dir = "build"
		cmdRun.Stdout = os.Stdout
		cmdRun.Stderr = os.Stderr

		if err := cmdRun.Run(); err != nil {
			fmt.Println("Error running application: %v\n", err)
		}
	
	},
}

func init(){
	rootCmd.AddCommand(runCmd)
}