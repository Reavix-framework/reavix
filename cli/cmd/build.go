package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	
	"github.com/spf13/cobra"

	
	utils "github.com/Reavix-framework/cli/internal/utils"
	
)

var buildCmd = &cobra.Command{
	Use: "build",
	Short: "Build production version",
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("Building production version...")

		frontendCmd := exec.Command("npm","run","build")
		frontendCmd.Dir = "app"
		frontendCmd.Stdout = os.Stdout
		frontendCmd.Stderr = os.Stderr

		if err := frontendCmd.Run(); err != nil {
			fmt.Println("App build error: %v\n", err)
			return
		}

		backendDir := filepath.Join("server","build")
		os.MkdirAll(backendDir, 0755)

		cmds := []*exec.Cmd{
			exec.Command("cmake", ".."),
			exec.Command("make"),
			exec.Command("./server"),
		}

		for _, c := range cmds{
			c.Dir = backendDir
			c.Stdout = os.Stdout
			c.Stderr = os.Stderr

			if err := c.Run(); err != nil {
				fmt.Println("Server build error: %v\n", err)
				return
			}
		}

		if err := os.MkdirAll("build", 0755); err != nil {
			fmt.Println("Error creating build directory: %v\n", err)
			return
		}

		if err := utils.CopyFile(
			filepath.Join(backendDir,"server"),
			filepath.Join("build","reavix-app"),
		); err != nil {
			fmt.Println("Error copying server: %v\n", err)
		}

		if err := utils.CopyDir(
			filepath.Join("app","dist"),
			filepath.Join("build","static"),
		); err != nil {
			fmt.Println("Error copying frontend: %v\n", err)
		}

		fmt.Println("Build complete! Run with: reavix run")
	},
}

func init(){
	rootCmd.AddCommand(buildCmd)
}