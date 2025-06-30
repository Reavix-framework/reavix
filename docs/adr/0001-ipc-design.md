# ADR 0001: IPC Mechanism Selection

## Status

Accepted (2023-11-20)

## Context

Need zero-copy, high-speed communication between:

- Javascript (renderer process)
- C (main process)

## Decision

use `SharedArrayBuffer` with atomic operations because:

1. Nanosecond latency (vs ms for JSON serialization)
2. Zero-copy semantics
3. Memory efficiency

## Tradeoffs

- Requires careful synchronization (Rust-like discipline)
- Browser security policies require COOP/COEP headers
- More complex debugging

## Mitigations

- Implement arena allocator with bounds checking
- Use Typescript to enforce thread safety constraints
- Comprehensive IPC fuzz testing
