#include "cli.h"
#include <gtest/gtest.h>
#include <string.h>
#include <vector>
#include <string>

// Mock write function to capture output
static std::vector<std::string> output_lines;
static std::string current_output;

static size_t mock_write(const void *ptr, size_t size) {
    const char *str = (const char *)ptr;
    for (size_t i = 0; i < size; ++i) {
        if (str[i] == '\n') {
            output_lines.push_back(current_output);
            current_output.clear();
        } else if (str[i] != '\r') {
            current_output += str[i];
        }
    }
    return size;
}

static int mock_flush(void) {
    return 0;
}

static int test_handler(cli_t *cli, int argc, char **argv) {
    (void)cli; (void)argc; (void)argv;
    return 0;
}

static const cli_cmd_t mock_cmds[] = {
    {"test", "test cmd", test_handler},
    {"cmd1", "cmd1", test_handler},
    {"cmd2", "cmd2", test_handler},
};

static const cli_cmd_list_t mock_cmd_list = {
    NULL, 0, mock_cmds, 3
};

class CliHistoryTest : public ::testing::Test {
protected:
    cli_t cli;
    
    void SetUp() override {
        output_lines.clear();
        current_output.clear();
        cli_init(&cli, &mock_cmd_list);
        cli.write = mock_write;
        cli.flush = mock_flush;
    }
};

TEST_F(CliHistoryTest, PushAndList) {
    // Manually push some commands to history since we want to test the history command first
    // Note: in actual use, cli_mainloop calls cli_history_push
    
    // We need to simulate how cli_mainloop calls it
    cli_puts(&cli, "help\n");
    cli_mainloop(&cli);
    
    cli_puts(&cli, "echo on\n");
    cli_mainloop(&cli);
    
    output_lines.clear();
    cli_puts(&cli, "history\n");
    cli_mainloop(&cli);
    
    // Check if "help" and "echo on" are in the output
    bool found_help = false;
    bool found_echo = false;
    for (const auto& line : output_lines) {
        if (line.find("help") != std::string::npos) found_help = true;
        if (line.find("echo on") != std::string::npos) found_echo = true;
    }
    
    EXPECT_TRUE(found_help);
    EXPECT_TRUE(found_echo);
}

TEST_F(CliHistoryTest, HistoryClear) {
    cli_puts(&cli, "help\n");
    cli_mainloop(&cli);
    
    cli_puts(&cli, "history clear\n");
    cli_mainloop(&cli);
    
    output_lines.clear();
    cli_puts(&cli, "history\n");
    cli_mainloop(&cli);
    
    // History should be empty (except for "history" itself if it was pushed, 
    // but usually we check if the previous ones are gone)
    for (const auto& line : output_lines) {
        EXPECT_EQ(line.find("help"), std::string::npos);
    }
}

TEST_F(CliHistoryTest, DuplicateHistory) {
    cli_puts(&cli, "test\n");
    cli_mainloop(&cli);
    
    cli_puts(&cli, "test\n");
    cli_mainloop(&cli);
    
    output_lines.clear();
    cli_puts(&cli, "history\n");
    cli_mainloop(&cli);
    
    int test_count = 0;
    for (const auto& line : output_lines) {
        if (line.find(" test") != std::string::npos) test_count++;
    }
    
    // Should only appear once in history (the second "test" is a duplicate)
    // Plus the "history" command itself
    EXPECT_EQ(test_count, 1);
}

TEST_F(CliHistoryTest, NoSaveOnFailure) {
    // A command that doesn't exist
    cli_puts(&cli, "unknown_cmd\n");
    cli_mainloop(&cli);
    
    // Valid command
    cli_puts(&cli, "help\n");
    cli_mainloop(&cli);
    
    output_lines.clear();
    cli_puts(&cli, "history\n");
    cli_mainloop(&cli);
    
    bool found_unknown = false;
    for (const auto& line : output_lines) {
        if (line.find("unknown_cmd") != std::string::npos) found_unknown = true;
    }
    
    EXPECT_FALSE(found_unknown);
}

TEST_F(CliHistoryTest, Navigation) {
    cli_puts(&cli, "cmd1\n");
    cli_mainloop(&cli);
    cli_puts(&cli, "cmd2\n");
    cli_mainloop(&cli);
    
    // Now simulate UP arrow: ESC [ A
    cli_putchar(&cli, 0x1b);
    cli_putchar(&cli, '[');
    cli_putchar(&cli, 'A');
    cli_mainloop(&cli);
    
    // This should have populated the line with "cmd2"
    EXPECT_STREQ(cli.line, "cmd2");
    
    // UP arrow again
    cli_putchar(&cli, 0x1b);
    cli_putchar(&cli, '[');
    cli_putchar(&cli, 'A');
    cli_mainloop(&cli);
    EXPECT_STREQ(cli.line, "cmd1");
    
    // DOWN arrow: ESC [ B
    cli_putchar(&cli, 0x1b);
    cli_putchar(&cli, '[');
    cli_putchar(&cli, 'B');
    cli_mainloop(&cli);
    EXPECT_STREQ(cli.line, "cmd2");

    // DOWN arrow again (back to empty)
    cli_putchar(&cli, 0x1b);
    cli_putchar(&cli, '[');
    cli_putchar(&cli, 'B');
    cli_mainloop(&cli);
    EXPECT_STREQ(cli.line, "");
}
