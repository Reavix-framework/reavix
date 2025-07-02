
# Reavix

**Reavix** is a native-first desktop application framework built in **C** with a modern **React-based frontend** (via Vite/TS). Reavix enables secure, high-performance desktop applications. It is designed to give developers powerful tools, native speed, and a seamless full-stack experience.

---

## Table of Contents

- [Features](#features)
- [Getting Started](#getting-started)
- [Project Structure](#project-structure)
- [CLI Commands](#cli-commands)
- [Architecture Overview](#architecture-overview)
- [Security Model](#security-model)
- [Performance & Memory Safety](#performance--memory-safety)
- [Plugin System](#plugin-system)
- [Roadmap](#roadmap)
- [License](#license)


## Features

### Core Capabilities

- **React/Vite Frontend** — Use modern React (TS/JS) to build UI.
- **Native C Backend** — Fast HTTP/WebSocket engine with zero runtime dependencies.
- **Modular Architecture** — Core + optional native and WASM plugins.
- **Security-first Design** — Sandboxing, permission control, CSP enforcement.
- **Pre-execution Analysis** — Catch memory errors, race conditions at compile time.
- **Single-Binary Packaging** — Bundle your entire app into one native executable.
- **Cross-Platform** — Supports Windows, macOS, and Linux natively.
- **Instant Hot Reload** — Lightning-fast frontend + backend rebuilds.
- **Built-in SQLite & KV Store** — Native storage + encrypted secrets.
- **Ultra-Fast IPC** — Lock-free shared memory queues and zero-copy JSON.



## Getting Started

### Prerequisites

- Node.js (for Vite/React only)
- C Compiler (GCC/Clang/MSVC)
- CMake ≥ 3.20
- Git

### Installation

```bash
# Install the CLI globally (compiled binary or build from source)
curl -sSL https://reavix.dev/install.sh | bash
````

### Create a New Project

```bash
reavix create my-app
cd my-app
reavix dev
```

This will start both the backend and React frontend in development mode with hot reload.


## Project Structure

```
my-app/
├── frontend/           # Vite + React (JS/TS) frontend
│   └── src/
├── backend/            # C backend source code
│   └── routes/
├── plugins/            # Optional native/WASM plugins
├── assets/             # Static files
├── reavix.config.json  # App manifest and permissions
├── build/              # Final build output
└── package.json        # Frontend dependencies
```


## CLI Commands

```bash
reavix create <app-name>      # Scaffold a new app
reavix dev                    # Start dev server (frontend + backend)
reavix build                  # Compile and package app
reavix run                    # Run built app
reavix audit                  # Security and permission scan
reavix stats                  # Live resource monitor
```


## Architecture Overview

```text
         ┌─────────────────────────────┐
         │         React (TS/JS)       │
         │       via Vite + HMR        │
         └────────────┬────────────────┘
                      │
                      ▼
         ┌─────────────────────────────┐
         │    Reavix Core (C engine)   │
         │ HTTP/WebSocket + JSON + FS  │
         │ SQLite + KV Store + Plugins │
         │ Secure Sandboxing + IPC     │
         └────────────┬────────────────┘
                      ▼
         ┌─────────────────────────────┐
         │    Cross-platform Runtime   │
         │  (Windows / macOS / Linux)  │
         └─────────────────────────────┘
```


## Security Model

* **Permission Manifest**
  Declarative JSON config to control access to files, net, system, etc.

* **Sandboxing (Capsicum/Seccomp)**
  Restricts backend execution to only allowed capabilities.

* **Secure IPC**
  Shared memory model prevents injection, overwriting, or spoofing.

* **No `eval`, no dynamic `require`**
  Frontend is sandboxed via CSP and build-time static analysis.

* **Crypto-first**
  Built-in `libsodium` APIs for encryption, hashing, and key management.


## Performance & Memory Safety

* **Compile-time Analysis**
  Escape analysis, use-after-free detection, bounds checking.

* **Custom Arena Allocators**
  For short-lived memory to minimize fragmentation.

* **Fail-fast Debug Mode**
  Guard pages, stack canaries, and memory overwrite detectors.

* **Thread-safe by Default**
  Prevents data races in concurrent backend logic.



## Plugin System

| Type             | Supported Formats       | Isolation       | Use Cases                          |
| ---------------- | ----------------------- | --------------- | ---------------------------------- |
| Native Plugins   | `.dll`, `.so`, `.dylib` | Dynamic linking | Extended routing, native APIs      |
| WASM Plugins     | `.wasm` + WASI          | Sandboxed       | Safe 3rd-party extensions          |
| Internal Modules | Static compiled         | Full access     | DB drivers, GPU layers, auth, etc. |

You can create reusable plugins to extend Reavix apps in a controlled, sandboxed manner.


## 📜 License

Reavix is licensed under the **GDLV3 License**. See [LICENSE](./LICENSE) for details.


## 🤝 Contributing

We're looking for contributors, plugin authors, and developers to help shape the future of Reavix.

```bash
# Clone the monorepo and build locally
git clone https://github.com/Reavix-framework/reavix.git
cd reavix
make setup && make dev
```

> Join the community: [Discord](https://discord.gg/reavix) • [Docs](https://docs.reavix.dev) • [X](https://x.com/reavixframework)


##### Reavix — Build Native. Ship Fast. Stay Secure.
