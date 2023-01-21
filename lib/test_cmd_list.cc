#include "cli.h"
#include <cstring>
#include <gtest/gtest.h>
struct output_buffer {
  char data[1024];
  size_t offset;
};

class TestCli : public ::testing::Test {
protected:
  void SetUp() override {
    cli_init(&_cli, NULL);
    _cli.write = TestCli::write;
    _cli.flush = TestCli::flush;
    _quit_flag = 0;
    clear_output_buffer(_output_buffer);
    cli_register_quit_callback(&_cli, TestCli::quit_handler);
  }
  // void TearDown() override {}
  static void clear_output_buffer(output_buffer &output) {
    memset(output.data, 0, sizeof(output.data));
    output.offset = 0;
  }
  static size_t write(const void *ptr, size_t size) {
    char *s = (char *)ptr;

    for (size_t i = 0; i < size; i++) {
      _output_buffer.data[_output_buffer.offset + i] = s[i];
    }
    _output_buffer.offset += size;
    _output_buffer.offset %= sizeof(_output_buffer.data);

    return size;
  }
  static int flush(void) { return 0; }
  static void quit_handler(void) { _quit_flag = 1; }

  static size_t read(void *ptr, size_t size) {
    size_t n = fread(ptr, size, 1, stdin);
    return n;
  }

public:
  static int cmd_handler(cli_t *cli, int argc, char **argv) {
    (void)cli;
    (void)argc;
    (void)argv;

    cli->write("cmd: ", 5);
    for (int i = 0; i < argc; i++) {
      cli->write("`", 1);
      cli->write(argv[i], strlen(argv[i]));
      cli->write("`, ", 2);
    }
    cli->write("\r\n", 2);

    _handler_flag = 1;

    return 0;
  }

protected:
  cli_t _cli;
  static int _quit_flag;
  static int _handler_flag;
  static output_buffer _output_buffer;
};
output_buffer TestCli::_output_buffer{};
int TestCli::_quit_flag;
int TestCli::_handler_flag;

const cli_cmd_t cli_cmd_mcu_list[] = {

    {
        .name = "reset",
        .desc = "[NUM]. Reset the mcu after NUM seconds",
        .handler = TestCli::cmd_handler,
    },
    {
        .name = "sleep",
        .desc = "[NUM]. Put mcu in sleep mode for NUM seconds",
        .handler = TestCli::cmd_handler,
    },

};

const cli_cmd_group_t cli_cmd_mcu_group = {
    .name = "mcu", .desc = "MCU group", .cmds = NULL, .length = 0};

const cli_cmd_t cli_cmd_gpio_list[] = {
    {
        .name = "input-get",
        .desc = "NAME. Get gpio input NAME value",
        .handler = TestCli::cmd_handler,
    },
    {
        .name = "output-get",
        .desc = "NAME. Get gpio output NAME value",
        .handler = TestCli::cmd_handler,
    },
    {
        .name = "output-set",
        .desc = "NAME (0|1). Set gpio output NAME",
        .handler = TestCli::cmd_handler,
    },
};

const cli_cmd_group_t cli_cmd_gpio_group = {
    .name = "gpio", .desc = "Gpio group", .cmds = NULL, .length = 0};

const cli_cmd_t cli_cmd_adc_list[] = {
    {
        .name = "get",
        .desc = "NAME. Get adc NAME value",
        .handler = TestCli::cmd_handler,
    },
    {
        .name = "start-conv",
        .desc = "NAME. Start acd NAME conversion",
        .handler = TestCli::cmd_handler,
    },
};

const cli_cmd_group_t cli_cmd_adc_group = {
    .name = "adc", .desc = "ADC group", .cmds = NULL, .length = 0};

const cli_cmd_group_t *cli_cmd_group[] = {
    &cli_cmd_mcu_group,
    &cli_cmd_gpio_group,
    &cli_cmd_adc_group,
};

cli_cmd_list_t cli_cmd_list = {
    .groups = NULL,
    .length = 0,
};

TEST_F(TestCli, TestBiildinQuitCmd) {
  cli_puts(&_cli, "quit\n");
  cli_mainloop(&_cli);
  EXPECT_EQ(_quit_flag, 1);
}

TEST_F(TestCli, TestBuildinEchoCmd) {
  cli_puts(&_cli, "echo off\r\n");
  cli_mainloop(&_cli);
  EXPECT_STREQ(_output_buffer.data, "echo off\r\nOk\r\n" CLI_PROMPT ">");

  clear_output_buffer(_output_buffer);
  cli_puts(&_cli, "echo on\r\n");
  cli_mainloop(&_cli);
  EXPECT_STREQ(_output_buffer.data, "\r\nOk\r\n" CLI_PROMPT ">");
}

TEST_F(TestCli, TestBuildinHelpCmd) {
  cli_puts(&_cli, "help\r\n");
  cli_mainloop(&_cli);
  EXPECT_EQ(memcmp(_output_buffer.data, "help\r\nhelp\t", 11), 0);
}

TEST_F(TestCli, TestNullCmdGroup) {
  cli_puts(&_cli, "mcu reset\r\n");
  cli_mainloop(&_cli);
  EXPECT_EQ(_quit_flag, 0);
}

static void add_groups(cli_cmd_list_t *cmd_list) {
  static cli_cmd_group_t mcu_group = {0};
  static cli_cmd_group_t gpio_group = {0};
  static cli_cmd_group_t adc_group = {0};

  // attach the empty groups
  static cli_cmd_group_t *cmd_group[3] = {&mcu_group, &gpio_group, &adc_group};

  cmd_list->groups = (const cli_cmd_group_t **)cmd_group;
  cmd_list->length = 3;
  for (size_t i = 0; i < 3; i++) {
    cmd_group[i]->name = cli_cmd_group[i]->name;
    cmd_group[i]->desc = cli_cmd_group[i]->desc;
  }
}

static void add_commands(const cli_cmd_t *cmd_list, size_t cmds_size,
                         cli_cmd_group_t **cmd_group, size_t index) {
  cmd_group[index]->cmds = cmd_list;
  cmd_group[index]->length = cmds_size;
}

static void add_adc_commands(cli_cmd_group_t **cmd_group,
                             int(handler)(cli_t *, int, char **)) {
  static cli_cmd_t cmd_adc_list[ARRAY_SIZE(cli_cmd_adc_list)] = {};
  for (size_t i = 0; i < ARRAY_SIZE(cli_cmd_adc_list); i++) {
    cmd_adc_list[i].name = cli_cmd_adc_list[i].name;
    cmd_adc_list[i].desc = cli_cmd_adc_list[i].desc;
    cmd_adc_list[i].handler = handler;
  }
  add_commands(cmd_adc_list, ARRAY_SIZE(cli_cmd_adc_list), cmd_group, 2);
}
static void add_gpio_commands(cli_cmd_group_t **cmd_group,
                              int(handler)(cli_t *, int, char **)) {
  static cli_cmd_t cmd_gpio_list[ARRAY_SIZE(cli_cmd_gpio_list)] = {};
  for (size_t i = 0; i < ARRAY_SIZE(cli_cmd_gpio_list); i++) {
    cmd_gpio_list[i].name = cli_cmd_gpio_list[i].name;
    cmd_gpio_list[i].desc = cli_cmd_gpio_list[i].desc;
    cmd_gpio_list[i].handler = handler;
  }
  add_commands(cmd_gpio_list, ARRAY_SIZE(cli_cmd_gpio_list), cmd_group, 1);
}
static void add_mcu_commands(cli_cmd_group_t **cmd_group,
                             int(handler)(cli_t *, int, char **)) {
  static cli_cmd_t cmd_mcu_list[ARRAY_SIZE(cli_cmd_mcu_list)];
  for (size_t i = 0; i < ARRAY_SIZE(cli_cmd_mcu_list); i++) {
    cmd_mcu_list[i].name = cli_cmd_mcu_list[i].name;
    cmd_mcu_list[i].desc = cli_cmd_mcu_list[i].desc;
    cmd_mcu_list[i].handler = handler;
  }
  add_commands(cmd_mcu_list, ARRAY_SIZE(cli_cmd_mcu_list), cmd_group, 0);
}

TEST_F(TestCli, TestNullCmdGroups) {

  cli_cmd_list_t *cmd_list = &cli_cmd_list;
  _cli.cmd_list = cmd_list;

  add_groups(cmd_list);

  cli_puts(&_cli, "mcu reset\r\n");
  cli_mainloop(&_cli);
  EXPECT_EQ(_handler_flag, 0);
}

TEST_F(TestCli, TestMcuCmdGroup) {

  cli_cmd_list_t *cmd_list = &cli_cmd_list;
  _cli.cmd_list = cmd_list;

  add_groups(cmd_list);
  add_mcu_commands((cli_cmd_group_t **)cmd_list->groups, NULL);
  add_gpio_commands((cli_cmd_group_t **)cmd_list->groups, NULL);
  add_adc_commands((cli_cmd_group_t **)cmd_list->groups, NULL);
  // 2.1.1 Test default handler
  _handler_flag = 0;
  cli_puts(&_cli, "mcu reset\r\n");
  cli_mainloop(&_cli);
  EXPECT_EQ(_handler_flag, 0);

  // 2.1.1 Test handler
  add_mcu_commands((cli_cmd_group_t **)cmd_list->groups, TestCli::cmd_handler);
  add_gpio_commands((cli_cmd_group_t **)cmd_list->groups, TestCli::cmd_handler);
  add_adc_commands((cli_cmd_group_t **)cmd_list->groups, TestCli::cmd_handler);
  _handler_flag = 0;
  cli_puts(&_cli, "mcu reset\r\n");
  cli_mainloop(&_cli);
  EXPECT_EQ(_handler_flag, 1);
}

TEST_F(TestCli, TestLimits) {
  // 3.1 Test empty line
  cli_cmd_list_t *cmd_list = &cli_cmd_list;
  _cli.cmd_list = cmd_list;

  add_groups(cmd_list);
  add_mcu_commands((cli_cmd_group_t **)cmd_list->groups, TestCli::cmd_handler);
  add_gpio_commands((cli_cmd_group_t **)cmd_list->groups, TestCli::cmd_handler);
  add_adc_commands((cli_cmd_group_t **)cmd_list->groups, TestCli::cmd_handler);

  _handler_flag = 0;
  cli_puts(&_cli, "\r\n");
  cli_mainloop(&_cli);
  EXPECT_EQ(_handler_flag, 0);

  // 3.2 Test argv max limit

  {
    char buf[CLI_LINE_MAX * 2] = {0};
    int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "mcu reset");
    for (size_t i = 0; i < (CLI_ARGV_NUM - 2) + 1; i++) {
      n += snprintf(buf + n, sizeof(buf) - n, " arg%ld", i);
    }
    n += snprintf(buf + n, sizeof(buf) - n, "\r\n");
    _handler_flag = 0;

    cli_puts(&_cli, buf);
    clear_output_buffer(_output_buffer);
    cli_mainloop(&_cli);
    EXPECT_EQ(_handler_flag, 0);
    EXPECT_EQ(strcmp(_output_buffer.data + strlen(buf),
                     "Error: The number of arguments exceeds maximum of "
                     "CLI_ARGV_NUM\r\n" CLI_PROMPT ">"),
              0);
  }

  {
    char buf[CLI_LINE_MAX * 2] = {0};
    int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "mcu reset");
    for (size_t i = 0; i < (CLI_ARGV_NUM - 2); i++) {
      n += snprintf(buf + n, sizeof(buf) - n, " arg%ld", i);
    }
    n += snprintf(buf + n, sizeof(buf) - n, "\r\n");

    _handler_flag = 0;
    cli_puts(&_cli, buf);
    clear_output_buffer(_output_buffer);
    cli_mainloop(&_cli);
    EXPECT_EQ(_handler_flag, 1);
  }

  // 3.2 Test line max limit
  {
    char buf[CLI_LINE_MAX * 2] = {0};
    int n = 0;
    n += snprintf(
        buf + n, sizeof(buf) - n,
        "mcu reset "
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\r\n");
    _handler_flag = 0;
    clear_output_buffer(_output_buffer);
    cli_puts(&_cli, buf);
    cli_mainloop(&_cli);
    EXPECT_EQ(_handler_flag, 0);
    // anything over \link CLI_LINE_MAX \endlink will be truncated
    EXPECT_EQ(strcmp(_cli.argv[2],
                     "0123456789abcdef0123456789abcdef0123456789abcdef01234"),
              0);
  }
}
