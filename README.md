# 🔧 Simple C Shell
This is a custom shell built in C that replicates many core features of a UNIX shell, including support for:

- ✅ Command parsing with pipes (|)
- 🧠 Command history with timing, process ID, and execution duration
- 📦 Background execution (&)
- 🚫 Signal handling for graceful exits (Ctrl+C)
- 🔁 Input/output redirection (planned or extendable)
- 🧵 Multithreaded-safe command parsing (manages memory allocations carefully)

# ✨ Features
| Feature           | Description                                                                 |
|-------------------|-----------------------------------------------------------------------------|
| Piped commands     | Supports multiple commands linked with `\|`                                |
| Command history    | Tracks each command's start/end time, PID, and execution duration          |
| Background jobs    | Use `&` to run processes in the background                                  |
| Signal handling    | Gracefully exits and prints history on `SIGINT` (Ctrl+C)                   |
| Modular parsing    | Uses dynamic memory and safe string handling for flexible command input    |

# 🛠️ How It Works
- User input is parsed using custom functions `parse()` and `makecmd()` for command separation and argument extraction.
- Commands are executed using `fork()` and `execvp()`.
- Background tasks redirect `stdout` and `stderr` to `/dev/null`.
- `SIGINT` is handled using `sigaction` to print command history before exit.

# 🧪 Example

```bash
$ ls -l | grep ".c" &

Started background process with PID: 12345

$ history

-------------------------------
Command: ls -l | grep ".c"
PID: 12345
Start Time: 1712848000
End Time: 1712848001
Duration: 1.00 seconds
-------------------------------
📦 Build and R
