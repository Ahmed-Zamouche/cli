# µCLI - Lightweight Command Line Interpreter for Embedded Systems

µCLI (micro-CLI) is a lightweight, configurable command-line interface library designed for embedded systems and resource-constrained environments. It provides a robust framework for adding interactive command-line capabilities to your C/C++ projects with minimal overhead.

## Features

- **Lightweight and Portable**: Written in pure C (C99) with no external dependencies
- **Configurable**: Customizable via preprocessor definitions for memory usage and features
- **Command Groups**: Organize commands into logical groups with hierarchical structure
- **Built-in Commands**: Includes `help`, `echo`, `clear`, `quit`, and `history` (optional)
- **Command History**: Optional history navigation with arrow keys and Ctrl-P/Ctrl-N
- **Line Editing**: Basic line editing with backspace, Ctrl-U (clear line), Ctrl-W (delete word)
- **Case-Insensitive Matching**: Commands are matched case-insensitively
- **Thread-Safe**: Optional lock/unlock callbacks for thread-safe operation
- **Cross-Platform**: Works on Linux, Windows, and embedded platforms
- **Comprehensive Testing**: Includes Google Test-based unit tests

## Quick Start

### Prerequisites

- C compiler (GCC, Clang, MSVC, or any C99-compliant embedded compiler)
- For desktop testing: CMake or Bazel (optional)
- For embedded: No operating system required (bare-metal friendly)

### Embedded-Specific Considerations

1. **Memory Configuration**: Adjust buffer sizes according to your RAM constraints:
   ```c
   #define CLI_LINE_MAX 32      // Reduce for memory-constrained systems
   #define CLI_HISTORY_NUM 4    // Reduce or disable history
   #define CLI_USE_HISTORY      // Comment out to disable
   ```

2. **I/O Implementation**: Override default write/flush functions:
   ```c
   size_t my_write(const void *ptr, size_t size) {
       // Send to UART, USB, etc.
       uart_send_blocking(ptr, size);
       return size;
   }
   
   cli_t cli;
   cli_init(&cli, &cmd_list);
   cli.write = my_write;
   ```

3. **Quit Handler**: The default quit handler enters an infinite loop.
   Override it for your application:
   ```c
   void my_quit_handler(void) {
       // Return to main menu, enter sleep, etc.
   }
   cli_register_quit_callback(&cli, my_quit_handler);
   ```

4. **Minimal Configuration**: For most constrained systems:
   ```c
   #define CLI_LINE_MAX 32
   #define CLI_IN_BUF_MAX 32
   #define CLI_ARGV_NUM 4
   // #define CLI_USE_HISTORY  // Disabled to save 512 bytes
   #include "cli.h"
   ```

### Basic Usage

```c
#include "cli.h"

// Define a command handler
static int my_command_handler(cli_t *cli, int argc, char **argv) {
    (void)cli;
    for (int i = 0; i < argc; i++) {
        printf("Arg %d: %s\n", i, argv[i]);
    }
    return 0;
}

// Define command list
static const cli_cmd_t my_commands[] = {
    {.name = "mycmd", .desc = "My custom command", .handler = my_command_handler},
};

static const cli_cmd_list_t cmd_list = {
    .cmds = my_commands,
    .cmds_length = ARRAY_SIZE(my_commands),
    .groups = NULL,
    .length = 0
};

int main() {
    cli_t cli;
    cli_init(&cli, &cmd_list);
    
    // Main loop
    while (1) {
        // Feed characters from your input source
        // cli_putchar(&cli, ch);
        cli_mainloop(&cli);
    }
    return 0;
}
```

## Configuration

Configure µCLI by defining these macros before including `cli.h`:

| Macro | Default | Description |
|-------|---------|-------------|
| `CLI_PROMPT` | `"ucli"` | Command prompt string |
| `CLI_IN_BUF_MAX` | `128` | Input receive buffer size |
| `CLI_LINE_MAX` | `64` | Maximum command line length |
| `CLI_ARGV_NUM` | `8` | Maximum number of arguments per command |
| `CLI_HISTORY_NUM` | `8` | Number of commands to keep in history |
| `CLI_USE_HISTORY` | *undefined* | Enable history functionality |

Example configuration:
```c
#define CLI_PROMPT "myapp"
#define CLI_LINE_MAX 128
#define CLI_USE_HISTORY
#include "cli.h"
```

## Building

### Using Bazel (Recommended)

```bash
# Build the library
bazel build //lib:cli

# Build with history support
bazel build //lib:cli_history

# Run tests
bazel test //lib:test_cmd_list
bazel test //lib:test_ringbuffer
bazel test //lib:test_history

# Build and run the example
bazel run //example:cli_example
```

### Using CMake

```bash
mkdir build && cd build
cmake ..
make
./tests/test_cmd_list
```

## Project Structure

```
.
├── lib/                    # Core library
│   ├── cli.c              # Main CLI implementation
│   ├── cli.h              # Public API header
│   ├── ringbuffer.c       | Ring buffer implementation
│   └── ringbuffer.h       | (internal dependency)
├── example/               # Example applications
│   ├── main.c             | Example main program
│   ├── uart.c             | UART/serial interface example
│   └── cmd_list.c         | Example command definitions
├── tests/                 # Unit tests
│   ├── test_cmd_list.cc   | Command list tests
│   ├── test_ringbuffer.cc | Ring buffer tests
│   └── test_history.cc    | History functionality tests
└── third-party/           # Third-party dependencies
```

## API Overview

### Initialization
```c
void cli_init(cli_t *cli, const cli_cmd_list_t *cmd_list);
```

### Input Handling
```c
int cli_putchar(cli_t *cli, int ch);
int cli_puts(cli_t *cli, const char *str);
```

### Main Loop
```c
void cli_mainloop(cli_t *cli);
```

### Customization
```c
void cli_register_quit_callback(cli_t *cli, void (*quit_cb)(void));
```

### Command Structure
```c
typedef struct cli_cmd_s {
    const char *name;
    const char *desc;
    int (*handler)(cli_t *cli, int argc, char **argv);
} cli_cmd_t;

typedef struct cli_cmd_group_s {
    const char *name;
    const char *desc;
    const cli_cmd_t *cmds;
    size_t length;
} cli_cmd_group_t;

typedef struct cli_cmd_list_s {
    const cli_cmd_group_t **groups;
    size_t length;
    const cli_cmd_t *cmds;
    size_t cmds_length;
} cli_cmd_list_t;
```

## Examples

### Command Groups

```c
static const cli_cmd_t gpio_commands[] = {
    {.name = "read", .desc = "Read GPIO pin", .handler = gpio_read_handler},
    {.name = "write", .desc = "Write GPIO pin", .handler = gpio_write_handler},
};

static const cli_cmd_group_t gpio_group = {
    .name = "gpio",
    .desc = "GPIO operations",
    .cmds = gpio_commands,
    .length = ARRAY_SIZE(gpio_commands)
};

static const cli_cmd_group_t *groups[] = {&gpio_group};
static const cli_cmd_list_t cmd_list = {
    .groups = groups,
    .length = ARRAY_SIZE(groups),
    .cmds = NULL,
    .cmds_length = 0
};
```

Usage: `gpio read <pin>` or `gpio write <pin> <value>`

## Testing

The library includes comprehensive unit tests:

```bash
# Run all tests
bazel test //lib:all

# Run specific test suites
bazel test //lib:test_cmd_list
bazel test //lib:test_history --test_output=all
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Designed for embedded systems with minimal resource usage
- Inspired by traditional UNIX command-line interfaces
- Built with portability and reliability in mind
