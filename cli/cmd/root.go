package cmd

import (
	"fmt"
	"os"
	"github.com/spf13/cobra"
)

var (
	version = "0.1.0"
	verbose bool
)

var rootCmd = &cobra.Command{
	Use: "reavix",
	Version: version,
	Short: "Reavix CLI tool",
	Long: "A CLI tool for managing Reavix applications\nComplete documentation at: github.com/Reavix-framework/cli",
	PersistentPreRun: func(cmd *cobra.Command, args []string){
		if verbose {
			fmt.Println("Debug mode enabled")
		}
	},
}

func Execute(){
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}

func init(){
	rootCmd.PersistentFlags().BoolVarP(&verbose, "verbose", "v", false, "Verbose output")
}