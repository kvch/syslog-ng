/*
 * Copyright (c) 2010-2016 Balabit
 * Copyright (c) 2010-2015 Balázs Scheidler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "template_lib.h"
#include "apphook.h"
#include "plugin.h"
#include "cfg.h"
#include <criterion/criterion.h>

static void
add_dummy_template_to_configuration(void)
{
  LogTemplate *dummy = log_template_new(configuration, "dummy");
  assert_true(log_template_compile(dummy, "dummy template expanded $HOST", NULL),
              "Unexpected error compiling dummy template");
  cfg_tree_add_template(&configuration->tree, dummy);
}

__attribute__((constructor))
static void global_test_init(void)
{
  app_startup();
  init_template_tests();
  add_dummy_template_to_configuration();
  plugin_load_module("basicfuncs", configuration, NULL);
}

__attribute__((destructor))
static void global_test_deinit(void)
{
  deinit_template_tests();
  app_shutdown();
}

static void
_log_msg_free(gpointer data, gpointer user_data)
{
  log_msg_unref((LogMessage *) data);
}

static GPtrArray *
create_log_messages_with_values(const gchar *name, const gchar **values)
{
  LogMessage *message;
  GPtrArray *messages = g_ptr_array_new();

  const gchar **value;
  for (value = values; *value != NULL; ++value)
    {
      message = create_empty_message();
      log_msg_set_value_by_name(message, name, *value, -1);
      g_ptr_array_add(messages, message);
    }

  return messages;
}

static void
free_log_message_array(GPtrArray *messages)
{
  g_ptr_array_foreach(messages, _log_msg_free, NULL);
  g_ptr_array_free(messages, TRUE);
}

Test(basicfuncs, test_cond_funcs)
{
  TemplateFormatTestCase test_cases[] =
  {
      {"$(grep 'facility(local3)' $PID)", "23323,23323"},
      {"$(grep -m 1 'facility(local3)' $PID)", "23323"},
      {"$(grep 'facility(local3)' $PID $PROGRAM)", "23323,syslog-ng,23323,syslog-ng"},
      {"$(grep 'facility(local4)' $PID)", ""},
      {"$(grep ('$FACILITY' eq 'local4') $PID)", ""},
      {"$(grep ('$FACILITY(' eq 'local3(') $PID)", "23323,23323"},
      {"$(grep ('$FACILITY(' eq 'local4)') $PID)", ""},
      {"$(grep \\'$FACILITY\\'\\ eq\\ \\'local4\\' $PID)", ""},
      {"$(if 'facility(local4)' alma korte)", "korte"},
      {"$(if 'facility(local3)' alma korte)", "alma"},
      {"$(if '\"$FACILITY\" lt \"local3\"' alma korte)", "korte"},
      {"$(if '\"$FACILITY\" le \"local3\"' alma korte)", "alma"},
      {"$(if '\"$FACILITY\" eq \"local3\"' alma korte)", "alma"},
      {"$(if '\"$FACILITY\" ne \"local3\"' alma korte)", "korte"},
      {"$(if '\"$FACILITY\" gt \"local3\"' alma korte)", "korte"},
      {"$(if '\"$FACILITY\" ge \"local3\"' alma korte)", "alma"},
      {"$(if '\"$FACILITY_NUM\" < \"19\"' alma korte)", "korte"},
      {"$(if '\"$FACILITY_NUM\" <= \"19\"' alma korte)", "alma"},
      {"$(if '\"$FACILITY_NUM\" == \"19\"' alma korte)", "alma"},
      {"$(if '\"$FACILITY_NUM\" != \"19\"' alma korte)", "korte"},
      {"$(if '\"$FACILITY_NUM\" > \"19\"' alma korte)", "korte"},
      {"$(if '\"$FACILITY_NUM\" >= \"19\"' alma korte)", "alma"},
      {"$(if '\"$FACILITY_NUM\" >= \"19\" and \"kicsi\" eq \"nagy\"' alma korte)", "korte"},
      {"$(if '\"$FACILITY_NUM\" >= \"19\" or \"kicsi\" eq \"nagy\"' alma korte)", "alma"},
      {"$(grep 'facility(local3)' $PID)@0", "23323"},
      {"$(grep 'facility(local3)' $PID)@1", "23323"},
      {"$(grep 'facility(local3)' $PID)@2", ""},
      {"$(or 1 \"\" 2)", "1"},
      {"$(or \"\" 2)", "2"},
      {"$(or \"\" \"\")", ""},
      {"$(or)", ""},
  };
  gint i, nr_of_cases;

  nr_of_cases = sizeof(test_cases) / sizeof(test_cases[0]);
  for (i = 0; i < nr_of_cases; i++)
      assert_template_format_with_context(test_cases[i].template, test_cases[i].expected);
}

void
test_str_funcs(void)
{
  TemplateFormatTestCase test_cases[] =
  {
    {"$(ipv4-to-int $SOURCEIP)", "168496141"},

    {"$(length $HOST $PID)", "5 5"},
    {"$(length $HOST)", "5"},
    {"$(length)", ""},

    {"$(substr $HOST 1 3)", "zor"},
    {"$(substr $HOST 1)", "zorp"},
    {"$(substr $HOST -1)", "p"},
    {"$(substr $HOST -2 1)", "r"},

    {"$(strip ${APP.STRIP1})", "value"},
    {"$(strip ${APP.STRIP2})", "value"},
    {"$(strip ${APP.STRIP3})", "value"},
    {"$(strip ${APP.STRIP4})", "value"},
    {"$(strip ${APP.STRIP5})", ""},

    {"$(strip ${APP.STRIP1} ${APP.STRIP2} ${APP.STRIP3} ${APP.STRIP4} ${APP.STRIP5})", "value value value value "},

    {"$(sanitize alma/bela)", "alma_bela"},
    {"$(sanitize -r @ alma/bela)", "alma@bela"},
    {"$(sanitize -i @ alma@bela)", "alma_bela"},
    {"$(sanitize -i '@/l ' alma@/bela)", "a_ma__be_a"},
    {"$(sanitize alma\x1b_bela)", "alma__bela"},
    {"$(sanitize -C alma\x1b_bela)", "alma\x1b_bela"},

    {"$(sanitize $HOST $PROGRAM)", "bzorp/syslog-ng"},

    {"$(indent-multi-line 'foo\nbar')", "foo\n\tbar"},

    {"$(lowercase ŐRÜLT ÍRÓ)", "őrült író"},
    {"$(uppercase őrült író)", "ŐRÜLT ÍRÓ"},

    {"$(replace-delimiter \"\t\" \",\" \"hello\tworld\")", "hello,world"},

    {"$(padding foo 10)", "       foo"},
    {"$(padding foo 10 x)", "xxxxxxxfoo"},
    {"$(padding foo 10 abc)", "abcabcafoo"},
  };

  assert_templates_format(test_cases, sizeof(test_cases) / sizeof(test_cases[0]));
}

Test(basicsfuncs, test_numeric_funcs)
{
  TemplateFormatTestCase test_cases[] =
  {
    {"$(+ $FACILITY_NUM 1)", "20"},
    {"$(+ -1 -1)", "-2"},
    {"$(- $FACILITY_NUM 1)", "18"},
    {"$(- $FACILITY_NUM 20)", "-1"},
    {"$(* $FACILITY_NUM 2)", "38"},
    {"$(/ $FACILITY_NUM 2)", "9"},
    {"$(% $FACILITY_NUM 3)", "1"},
    {"$(/ $FACILITY_NUM 0)", "NaN"},
    {"$(% $FACILITY_NUM 0)", "NaN"},
    {"$(+ foo bar)", "NaN"},
    {"$(/ 2147483648 1)", "2147483648"},
    {"$(+ 5000000000 5000000000)", "10000000000"},
    {"$(% 10000000000 5000000001)", "4999999999"},
    {"$(* 5000000000 2)", "10000000000"},
    {"$(- 10000000000 5000000000)", "5000000000"},
  };

  assert_templates_format(test_cases, sizeof(test_cases) / sizeof(test_cases[0]));
}

typedef struct
{
  const gchar *macro;
  const gchar *result;
} MacroAndResult;

static void
_test_macros_with_context(const gchar *id, const gchar *numbers[], const MacroAndResult test_cases[])
{
  GPtrArray *messages = create_log_messages_with_values(id, numbers);

  for (const MacroAndResult *test_case = test_cases; test_case->macro; test_case++)
    assert_template_format_with_context_msgs(
      test_case->macro, test_case->result,
      (LogMessage **)messages->pdata, messages->len);

  free_log_message_array(messages);
}

Test(basicfuncs, test_numeric_aggregate_simple)
{
  _test_macros_with_context(
    "NUMBER", (const gchar *[])
  { "1", "-1", "3", NULL
  },
  (const MacroAndResult[])
  {
    { "$(sum ${NUMBER})", "3" },
    { "$(min ${NUMBER})", "-1" },
    { "$(max ${NUMBER})", "3" },
    { "$(average ${NUMBER})", "1" },
    { }
  });
}

Test(basicfuncs, test_numeric_aggregate_invalid_values)
{
  _test_macros_with_context(
    "NUMBER", (const gchar *[])
  { "abc", "1", "c", "2", "", NULL
  },
  (const MacroAndResult[])
  {
    { "$(sum ${NUMBER})", "3" },
    { "$(min ${NUMBER})", "1" },
    { "$(max ${NUMBER})", "2" },
    { "$(average ${NUMBER})", "1" },
    { }
  });
}

Test(basicfuncs, test_numeric_aggregate_full_invalid_values)
{
  _test_macros_with_context(
    "NUMBER", (const gchar *[])
  { "abc", "184467440737095516160", "c", "", NULL
  },
  (const MacroAndResult[])
  {
    { "$(sum ${NUMBER})", "" },
    { "$(min ${NUMBER})", "" },
    { "$(max ${NUMBER})", "" },
    { "$(average ${NUMBER})", "" },
    { }
  });
}

Test(basicfuncs, test_misc_funcs)
{
  unsetenv("OHHELLO");
  setenv("TEST_ENV", "test-env", 1);

  assert_template_format("$(env OHHELLO)", "");
  assert_template_format("$(env TEST_ENV)", "test-env");
}

Test(basicfuncs, test_tf_template)
{
  assert_template_format("foo $(template dummy) bar", "foo dummy template expanded bzorp bar");
  assert_template_failure("foo $(template unknown) bar", "Unknown template function or template \"unknown\"");
}
