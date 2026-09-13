# OSN Mini Project

This repository contains two related components:

1. `c-shell`: a POSIX C shell with built-in commands, external command
	 execution, redirection, pipelines, background jobs, and job control.
2. `xv6-riscv`: an xv6/RISC-V kernel modified to compare FIFO, round-robin,
	 and multilevel-feedback-queue scheduling.

The files in `plot/` are optional Python utilities for processing scheduler
output and generating plots.

## 1. Running the C Shell

The shell requires a POSIX-like environment with `gcc`, `make`, and the usual
Unix process APIs (`fork`, `exec`, `pipe`, `waitpid`, process groups, and
signals). On Windows, run it through WSL, MSYS2, or another compatible
environment.

```bash
cd c-shell
make
./a
```

Clean the generated object files and executable with:

```bash
make clean
```

The shell supports these built-ins:

`echo`, `hop`, `reveal`, `locate`, `peek`, `activities`, `resume`, `ping`,
`spy`, and `snoop`.

The lexer recognizes:

- Pipes: `|`
- Background execution: `&`
- Sequential commands: `;`
- Input redirection: `<`
- Output redirection: `>` and `>>`
- Single- and double-quoted words

External commands are searched in the current directory and then in `PATH`.
An executable path may also be supplied directly. The shell groups foreground
jobs into a process group and supports stopping/resuming jobs using the job
control commands.

## 2. Running xv6

xv6 requires a RISC-V cross-compiler and QEMU with the `riscv64-softmmu`
target. The Makefile automatically detects common toolchain prefixes such as
`riscv64-unknown-elf-` and `riscv64-linux-gnu-`.

From the repository root:

```bash
cd xv6-riscv
make qemu
```

The default scheduler is round robin. Select another scheduler at build time:

```bash
make clean
make qemu SCHEDULER=FIFO

make clean
make qemu SCHEDULER=MLFQ
```

The supported values are `RR` (the default), `FIFO`, and `MLFQ`. The `FIFO`
option implements first-come, first-served behavior; the code uses the name
`SCHEDULER_FIFO` internally.

Useful scheduler programs included in the xv6 image are:

```text
schedtest   fixed mixed CPU-bound/bursty workload
mlfqtest    continuously CPU-bound stress workload
```

The benchmark output can be redirected from the QEMU session and processed by
the scripts in `plot/`. For example, inspect `plot/README.md` for the plotting
workflow and Python dependencies.

## 3. Repository Structure

```text
.
|-- README.md                 project documentation
|-- FCFS.txt, rr.txt, mlfq.txt
|                              scheduler experiment output
|-- c-shell/
|   |-- Makefile               host-shell build rules
|   |-- include/               public headers
|   `-- src/                   lexer, parser, built-ins, jobs, and main
|-- plot/
|   |-- main.py, plot.py       output processing and plotting
|   `-- pyproject.toml         Python project configuration
`-- xv6-riscv/
		|-- kernel/                kernel and scheduler implementation
		|-- user/                  user programs and scheduler tests
		|-- Makefile                cross-build and QEMU targets
		`-- test-xv6.py             xv6 test helper
```

## 4. C Shell Design Choices

- **Lexer/parser:** input is first converted into tokens and then validated by
	a small state-machine parser before execution.
- **Built-ins vs. external commands:** built-ins run in the shell process when
	possible so commands such as `hop` can change shell state. Pipeline and
	background stages run in child processes.
- **Pipelines:** each pipeline stage is forked into the same process group, and
	adjacent stages are connected with Unix pipes.
- **Redirection:** input is copied into temporary files where needed so the
	same redirection path works with built-ins. `>` truncates and `>>` appends;
	created output files use mode `0644`.
- **Job control:** foreground jobs receive the terminal, while the shell
	ignores interactive `SIGINT`, `SIGTSTP`, and `SIGTTOU`. `SIGCHLD` updates
	background-job state.
- **Command lookup:** normal names use the current directory first and then
	`PATH`; names beginning with `%` explicitly trigger `PATH` lookup after the
	prefix is removed.

## 5. xv6 Design Changes

- Added build-time scheduler selection through `SCHEDULER=RR`,
	`SCHEDULER=FIFO`, or `SCHEDULER=MLFQ`.
- Implemented FIFO scheduling without timer preemption, so a process runs
	until it blocks, yields, or exits.
- Kept round-robin scheduling as the default timer-driven scheduler.
- Implemented MLFQ with four queues. Its time quanta are `1`, `4`, `8`, and
	`16` timer ticks, with strict priority between queues and a global priority
	boost every `48` ticks.
- Added per-process scheduling state and timing metrics, including arrival,
	first-run, completion, waiting, queue, and tick counters.
- Added `schedtest` and `mlfqtest` to exercise mixed workloads and sustained
	CPU contention. `schedtest` creates 10 jobs, including CPU-bound and bursty
	processes; its burst lengths are fixed in `user/schedtest.c`.

No project-specific syscall was added. The syscall lookup table in
`kernel/syscall.c` maps these 22 xv6 syscalls:

`fork`, `exit`, `wait`, `pipe`, `read`, `kill`, `exec`, `fstat`, `chdir`,
`dup`, `getpid`, `sbrk`, `pause`, `uptime`, `open`, `write`, `mknod`,
`unlink`, `link`, `mkdir`, `close`, and `sync`.

## 6. Assumptions and Implementation Limits

### C shell

- Each `fgets` call reads at most 998 input characters, excluding the null
	terminator. A longer physical line is consumed over multiple prompt reads;
	it is not treated as one unbounded command.
- The lexer allocates a fixed array of 1,000 tokens per input line.
- There is no separate semantic maximum for pipeline stages. In practice,
	pipelines are limited by the 1,000-token array, available memory, file
	descriptors, and the operating system's process limit. Background job
	bookkeeping has space for at most 100 processes per job.
- Stored command text is bounded: individual process descriptions hold 64
	characters and job command text holds 256 characters. Longer descriptions
	are truncated for display/bookkeeping, although execution still uses the
	parsed token text.
- Quoting groups words but does not implement shell expansion such as `$VAR`,
	command substitution, globbing, or escape processing beyond the implemented
	`\\n` conversion.
- Syntax errors, missing redirection targets, missing files, and failed process
	operations are reported to standard output/error according to the existing
	implementation; this is a teaching shell rather than a complete POSIX shell.

### xv6 and experiments

- xv6 uses the standard process-table limit defined by its `NPROC` constant;
	the scheduler test itself intentionally creates 10 benchmark jobs.
- Scheduler measurements are in xv6 timer ticks, not host wall-clock time.
- FIFO is cooperative/non-preemptive, so a CPU-bound process can delay other
	runnable processes until it blocks, yields, or exits.
- The MLFQ policy assumes four queues, the fixed quanta above, and periodic
	boosting every 48 ticks. These are compile-time implementation choices, not
	user-configurable runtime settings.
- Results depend on the selected scheduler, QEMU version, toolchain, and the
	exact workload. The experiment output files should therefore be treated as
	measurements of this configuration rather than universal performance claims.

## 7. Cleaning Generated Files

For the host shell:

```bash
cd c-shell
make clean
```

For xv6, from `xv6-riscv/`, use the upstream clean target before rebuilding:

```bash
make clean
```