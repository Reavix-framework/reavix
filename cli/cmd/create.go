package cmd

import (
    "fmt"
    "os"
    "os/exec"
    "path/filepath"
    "text/template"

    "github.com/spf13/cobra"
)

var (
	readmeTmpl = readFile("readme.tmpl")
	gitignoreTmpl = readFile("gitignore.tmpl")
	indexCssTmpl = readFile("index.css.tmpl")
	viteConfigTmpl = readFile("vite.config.tmpl")
	tailwindConfigTmpl = readFile("tailwind.config.tmpl")
	postcssConfigTmpl = readFile("postcss.config.tmpl")
	mainTsxTmpl = readFile("main.tsx.tmpl")
	appTsxTmpl = readFile("app.tsx.tmpl")
	connectionStatusTmpl = readFile("connection_status.tmpl")
	routerCTmpl = readFile("router.c.tmpl")
	mainCTmpl = readFile("main.c.tmpl")
	routerHTmpl = readFile("router.h.tmpl")
	utilsCTmpl = readFile("utils.c.tmpl")
	cmakeTmpl = readFile("CMakeLists.txt.tmpl")
)

func readFile(filename string) string {
	templateDir := "/home/kasimlyee/Reavix/cli/templates"
	absPath := filepath.Join(templateDir, filename)

	data, err := os.ReadFile(absPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading file %s: %v\n", absPath, err)
		return ""
	}

	return string(data)
}

var createCMD = &cobra.Command{
    Use:   "create <app-name>",
    Short: "Create a new Reavix application",
    Args:  cobra.ExactArgs(1),
    Run: func(cmd *cobra.Command, args []string) {
        appName := args[0]
        if err := createProject(appName); err != nil {
            fmt.Printf("Error creating project: %v\n", err)
            os.Exit(1)
        }

        fmt.Printf("Project %s created successfully\n", appName)
        fmt.Printf("Run `reavix build` to build the project\n")
    },
}

func init() {
    rootCmd.AddCommand(createCMD)
}

func createProject(name string) error {
    dirs := []string{
        "app/src/components",
        "app/src/hooks",
        "server/src",
        "server/include",
        "build",
        "script",
    }

    for _, dir := range dirs {
        fullPath := filepath.Join(name, dir)
        if err := os.MkdirAll(fullPath, 0755); err != nil {
            fmt.Printf("Error creating directory %s: %v\n", fullPath, err)
            return err
        }
    }

    files := map[string]struct {
        content string
        data    interface{}
    }{
        "app/vite.config.ts":                  {content: viteConfigTmpl},
        "app/tailwind.config.js":              {content: tailwindConfigTmpl},
        "app/postcss.config.js":               {content: postcssConfigTmpl},
        "app/src/main.tsx":                    {content: mainTsxTmpl},
        "app/src/App.tsx":                     {content: appTsxTmpl},
        "app/src/index.css":                   {content: indexCssTmpl},
        "app/src/components/ConnectionStatus.tsx": {content: connectionStatusTmpl},
        "server/src/main.c":                   {content: mainCTmpl},
        "server/src/router.c":                 {content: routerCTmpl},
        "server/src/utils.c":                  {content: utilsCTmpl},
        "server/include/router.h":             {content: routerHTmpl},
        //"scripts/build.sh":                    {content: buildScriptTmpl, data: map[string]string{"AppName": name}},
        "server/CMakeLists.txt":               {content: cmakeTmpl},
        "README.md":                           {content: readmeTmpl, data: map[string]string{"AppName": name}},
        ".gitignore":                          {content: gitignoreTmpl},
    }

    for file, templateInfo := range files {
        if err := createFileFromTemplate(
            filepath.Join(name, file),
            templateInfo.content,
            templateInfo.data,
        ); err != nil {
            return fmt.Errorf("failed to create file %s: %w", file, err)
        }
    }

    if err := initFrontendDeps(name); err != nil {
        return fmt.Errorf("failed to initialize frontend dependencies: %w", err)
    }

    return nil
}

func initFrontendDeps(projectDir string) error {
    cmd := exec.Command("npm", "install", "-D", "vite", "@vitejs/plugin-react", "tailwindcss", "postcss", "autoprefixer", "typescript", "@types/react", "@types/react-dom")
    cmd.Dir = filepath.Join(projectDir, "app")
    cmd.Stdout = os.Stdout
    cmd.Stderr = os.Stderr

    if err := cmd.Run(); err != nil {
        return fmt.Errorf("failed to install frontend dependencies: %w", err)
    }

    cmd = exec.Command("npx", "tailwindcss", "init", "-p")
    cmd.Dir = filepath.Join(projectDir, "app")
    cmd.Stdout = os.Stdout
    cmd.Stderr = os.Stderr
    return cmd.Run()
}

func createFileFromTemplate(path, content string, data interface{}) error {
    if data == nil {
        data = map[string]interface{}{}
    }
    tmpl, err := template.New("file").Parse(content)
    if err != nil {
        return err
    }

    if err := os.MkdirAll(filepath.Dir(path), 0755); err != nil {
        return err
    }

    file, err := os.Create(path)
    if err != nil {
        return err
    }
	defer file.Close()

    return tmpl.Execute(file, data)
}

