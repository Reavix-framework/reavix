package cmd

import (
	"fmt"
	"os"
	"path/filepath"
	"text/template"

	"github.com/spf13/cobra"

)

var createCMD = &cobra.Command{
	Use: "create <app-name>",
	Short: "Create a new Reavix application",
	Args: cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		appName := args[0]
		createProject(appName); err != nil{
			fmt.Printf("Error creating project: %v\n",err)
			os.Exit(1)
		}

		fmt.Printf("Project %s created successfully\n",appName)
		fmt.Printf("Run `reavix build` to build the project")
	},
}

func init() {
	rootCmd.AddCommand(createCMD)
}

func createProject(name string){

	dirs := []string{
		"app/src/components",
		"app/src/hooks",
		"server/src",
		"server/include",
		"build",
		"script",
	}

	for _, dir := range dirs{
		fullPath := filepath.Join(name,dir)
		if err := os.MkdirAll(fullPath, 0755); err != nil {
			fmt.Printf("Error creating directory %s: %v\n",fullPath,err)
			return
		}
	
	}

	files := map[string]struct{
		content string
		data interface {}
	}{
		"app/vite.config.ts":{content:viteConfigTmpl},
		"app/tailwind.config.js":{content:tailwindConfigTmpl},
		"app/postcss.config.js":{content:postcssConfigTmpl},
		"app/src/main.tsx":{content:mainTsxTmpl},
		"app/src/App.tsx":{content:appTsxTmpl},
		"app/src/index.css":{content:indexCssTmpl},
		"app/src/components/ConnectionStatus.tsx":{content:connectionStatusTmpl},
		"server/src/main.c":{content:mainCTmpl},
		"server/src/router.c":{content:routerCTmpl},
		"server/src/utils.c":{content:utilsCTmpl},
		"server/include/router.h":{content:routerHTmpl},
		"scripts/build.sh":{content:buildScriptTmpl, data: map[string]string{"AppName":name}},
		"server/CMakeLists.txt":cmakeTmpl,
		"README.md":{content:readmeTmpl,date: map[string]string{"AppName":name}},
		".gitignore":{content:gitignoreTmpl},
	}

	for file, content := range files{
		fullPath := filepath.Join(name,file)
		if err := os.WriteFile(fullPath,[]byte(content),0644); err !=nil{
			fmt.Printf("Error creating file %s: %v\n",fullPath,err)
			return
		}
	}

	fmt.Printf("Created new Reavix Project: %s\n",name)
	fmt.Printf("Next steps:\n")
	fmt.Printf("	cd %s\n",name)
	fmt.Printf("	reavix dev\n")
}

const viteConfigTmpl = `import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      "/api": {
        target: "http://localhost:8081",
        changeOrigin: true,
      },
    },
  },
});
`

const cmakeTmpl = `Cmake_minimum_required(VERSION 3.10)
project(ReavixBackend)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUV REQUIRED libuv)
include_directories(${LIBUV_INCLUDE_DIRS})
link_directories(${LIBUV_LIBRARY_DIRS})

include_directories(include)

add_executable(server
    src/main.c
    src/router.c
    src/utils.c)

target_link_libraries(server uv pthread dl rt)`

const readmeTmpl = `# {{.AppName}}

This is a Reavix application.

## Development

\'\'\'bash
reavix dev
\'\'\'

# Build

\'\'\'bash
reavix build
\'\'\'
`