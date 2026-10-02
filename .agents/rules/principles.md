# Agent Principles for Posix Socket

## Role and Purpose

This repository contains a concurrent HTTP server implemented in C/POSIX on FreeBSD. Agents assist in maintaining the C23 codebase, FreeBSD kqueue implementations, and Capsicum sandboxing.

## Key Technical Constraints and Invariants

- Code MUST follow C23 and POSIX.1-2024 standards.
- Concurrency model strictly relies on FreeBSD kqueue.
- Capsicum must be used for robust process sandboxing.
- Follow existing `.clang-format` rules in the `Project/` directory.

## References

- Follow constitutional rules in `/AGENTS.md`.
