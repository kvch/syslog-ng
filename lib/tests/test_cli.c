 /*
 * Copyright (c) 2016 Noemi Vanyi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "cli.h"
#include "testutils.h"
#include "messages.h"

#define CLI_TESTCASE(testfunc, ...)  { testcase_begin("%s(%s)", #testfunc, #__VA_ARGS__); testfunc(__VA_ARGS__); testcase_end(); }


void
test_is_cli_or_command_line_drivers(void)
{
  Cli *cli = cli_new(NULL, FALSE);
  assert_false(cli->is_cli, "Not command line mode: --cli is not set");
  assert_false(cli->is_command_line_drivers, "Not command line drivers: raw params are NULL");

  cli = cli_new(NULL, TRUE);
  assert_true(cli->is_cli, "Command line mode: --cli is set");
  assert_false(cli->is_command_line_drivers, "Not command line drivers: raw params are NULL");

  gchar *parser_raw_param[] = {"parser csvparser();"};
  cli = cli_new(parser_raw_param, FALSE);
  assert_true(cli->is_cli, "Command line mode: command line drivers are passed");
  assert_true(cli->is_command_line_drivers, "Command line drivers: raw param string is passed");

  cli = cli_new(parser_raw_param, TRUE);
  assert_true(cli->is_cli, "Command line mode: --cli is set && command line drivers are passed");
  assert_true(cli->is_command_line_drivers, "Command line drivers: raw param string is passed");
}

void
test_cli_setup_params(void)
{
  gboolean command_line_mode = TRUE;

  gchar **parser_raw_param = (char *[]){ "parser csvparser()", NULL };
  Cli *cli = cli_new(parser_raw_param, command_line_mode);
  assert_false(cli_setup_params(cli), "Invalid raw params: missing driver separator");
  assert_true(cli->params == NULL, "No parsed drivers");

  parser_raw_param = (char *[]){ "csvparser();", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_false(cli_setup_params(cli), "Invalid raw params: missing driver type");
  assert_true(cli->params == NULL, "No parsed drivers");

  parser_raw_param = (char *[]){ "parser;", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_false(cli_setup_params(cli), "Invalid raw params: missing driver config");
  assert_true(cli->params == NULL, "No parsed drivers");
  
  parser_raw_param = (char *[]){ "parser json-parser();", "parser json-parser()", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_false(cli_setup_params(cli), "Invalid raw params: missing driver separator in 2nd item");
  assert_true(cli->params == NULL, "No parsed drivers");
  
  parser_raw_param = (char *[]){ "parser csvparser();", "parser;", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_false(cli_setup_params(cli), "Invalid raw params: missing driver config in 2nd item");
  assert_true(cli->params == NULL, "No parsed drivers");
  
  parser_raw_param = (char *[]){ "parser csvparser();", "csvparser();", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_false(cli_setup_params(cli), "Invalid raw params: missing driver config in 2nd item");
  assert_true(cli->params == NULL, "No parsed drivers");
  
  parser_raw_param = (char *[]){ "parser csvparser();", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_true(cli_setup_params(cli), "Valid raw params: csvparser");
  assert_true(g_list_length(cli->params) == 1, "1 parsed driver: csvparser");
  
  parser_raw_param = (char *[]){ "parser csvparser(columns(column));", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_true(cli_setup_params(cli), "Valid raw params: csvparser");
  assert_true(g_list_length(cli->params) == 1, "1 parsed driver: csvparser");

  parser_raw_param = (char *[]){ "parser csvparser(columns(column)); parser json-parser();", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_true(cli_setup_params(cli), "Valid raw params: csvparser, json-parser");
  assert_true(g_list_length(cli->params) == 2, "2 parsed driver: csvparser, json-parser");
  
  parser_raw_param = (char *[]){ "parser csvparser(columns(column)); parser json-parser();", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_true(cli_setup_params(cli), "Valid raw params: csvparser, json-parser");
  assert_true(g_list_length(cli->params) == 2, "2 parsed driver: csvparser, json-parser");
  
  parser_raw_param = (char *[]){ "parser csvparser(columns(column)); parser json-parser();", "parser csvparser();", NULL };
  cli = cli_new(parser_raw_param, command_line_mode);
  assert_true(cli_setup_params(cli), "Valid raw params: csvparser, json-parser, csvparser");
  assert_true(g_list_length(cli->params) == 3, "3 parsed driver: csvparser, json-parser, csvparser");
}

void
test_cli_init_cfg(void)
{
  GlobalConfig *global_config = cfg_new(0);
  GList *params = NULL;
  CliParam *cli_param = cli_param_new("parser", "csvparser();");
  cli_param->name = "my-name";
  params = g_list_append(params, cli_param);
  Cli *cli = cli_new(NULL, TRUE);
  cli->params = params;
  gchar *expected_cfg = "@version: 3.8.0alpha0\n"
                        "@include scl.conf\n"
                        "source s_stdin { stdin(); };\n"
                        "destination d_stdout { stdout(); };\n"
                        "parser my-name {  csvparser();};\n"
                        "log {source(s_stdin); destination(d_stdout); parser(my-name); };";

  assert_true(cli_init_cfg(cli, global_config), "Succesfully initialized config");
  assert_true(strcmp(cli->generated_config, expected_cfg) == 0, "Expected config file");
}

int
main(int argc G_GNUC_UNUSED, char *argv[] G_GNUC_UNUSED)
{
  CLI_TESTCASE(test_is_cli_or_command_line_drivers);
  CLI_TESTCASE(test_cli_setup_params);
  CLI_TESTCASE(test_cli_init_cfg);
  return 0;
}
