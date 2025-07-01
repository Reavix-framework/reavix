package cmd

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	
	"github.com/spf13/cobra"

)

var devCmd = &cobra.Command{
	Use: "dev",
	Short: "Start development server",
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("Starting development server...")

		go func(){
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
				if err := c.Run(); err != nil{
					fmt.Println("Server error: %v\n", err)
					return
				}
			}
		}()

		frontendCmd := exec.Command("npm", "run", "dev")
		frontendCmd.Dir = "app"
		frontendCmd.Stdout = os.Stdout
		frontendCmd.Stderr = os.Stderr

		if err := frontendCmd.Run(); err != nil{
			fmt.Println("App error: %v\n", err)
			return
		}
	
	
},
}

func init(){
	rootCmd.AddCommand(devCmd)
}