/*
 * Copyright (c) 2007-2014 Balabit
 * Copyright (c) 2007-2014 Balázs Scheidler <balazs.scheidler@balabit.com>
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

#include "syslog-ng.h"
#include "template_lib.h"

#include "logmsg/logmsg.h"
#include "template/templates.h"
#include "template/user-function.h"
#include "apphook.h"
#include "cfg.h"
#include "timeutils.h"
#include "plugin.h"

#include <criterion/criterion.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

GCond *thread_ping;
GMutex *thread_lock;
gboolean thread_start;

__attribute__((constructor))
static void global_test_init(void)
{
  app_startup();

  init_template_tests();
  plugin_load_module("basicfuncs", configuration, NULL);
  configuration->template_options.frac_digits = 3;
  configuration->template_options.time_zone_info[LTZ_LOCAL] = time_zone_info_new(NULL);

  putenv("TZ=MET-1METDST");
  tzset();
}

__attribute__((destructor))
static void global_test_deinit(void)
{
  deinit_template_tests();
  app_shutdown();
}

static gpointer
format_template_thread(gpointer s)
{
  gpointer *args = (gpointer *) s;
  LogMessage *msg = args[0];
  LogTemplate *templ = args[1];
  const gchar *expected = args[2];
  GString *result;
  gint i;

  g_mutex_lock(thread_lock);
  while (!thread_start)
    g_cond_wait(thread_ping, thread_lock);
  g_mutex_unlock(thread_lock);

  result = g_string_sized_new(0);
  for (i = 0; i < 10000; i++)
    {
      log_template_format(templ, msg, NULL, LTZ_SEND, 5555, NULL, result);
      assert_string(result->str, expected, "multi-threaded formatting yielded invalid result (iteration: %d)", i);
    }
  g_string_free(result, TRUE);
  return NULL;
}

static void
assert_template_format_multi_thread(const gchar *template, const gchar *expected)
{
  LogTemplate *templ;
  LogMessage *msg;
  gpointer args[3];
  GThread *threads[16];
  gint i;

  msg = create_sample_message();
  templ = compile_template(template, FALSE);
  args[0] = msg;
  args[1] = templ;
  args[2] = (gpointer) expected;

  thread_start = FALSE;
  thread_ping = g_cond_new();
  thread_lock = g_mutex_new();
  args[1] = templ;
  for (i = 0; i < 16; i++)
    {
      threads[i] = g_thread_create(format_template_thread, args, TRUE, NULL);
    }

  thread_start = TRUE;
  g_mutex_lock(thread_lock);
  g_cond_broadcast(thread_ping);
  g_mutex_unlock(thread_lock);
  for (i = 0; i < 16; i++)
    {
      g_thread_join(threads[i]);
    }
  g_cond_free(thread_ping);
  g_mutex_free(thread_lock);
  log_template_unref(templ);
  log_msg_unref(msg);
}

Test(template, test_macros)
{
  /* pri 3, fac 19 == local3 */

  TemplateFormatTestCase test_cases[] =
  {
    {"$FACILITY", "local3"},
    {"$FACILITY_NUM", "19"},
    {"$PRIORITY", "err"},
    {"$LEVEL", "err"},
    {"$LEVEL_NUM", "3"},
    {"$TAG", "9b"},
    {"$TAGS", "alma,korte,citrom"},
    {"$PRI", "155"},
    {"$DATE", "Feb 11 10:34:56.000"},
    {"$FULLDATE", "2006 Feb 11 10:34:56.000"},
    {"$ISODATE", "2006-02-11T10:34:56.000+01:00"},
    {"$STAMP", "Feb 11 10:34:56.000"},
    {"$YEAR", "2006"},
    {"$YEAR_DAY", "042"},
    {"$MONTH", "02"},
    {"$MONTH_WEEK", "1"},
    {"$MONTH_ABBREV", "Feb"},
    {"$MONTH_NAME", "February"},
    {"$DAY", "11"},
    {"$HOUR", "10"},
    {"$MIN", "34"},
    {"$SEC", "56"},
    {"$WEEKDAY", "Sat"},
    {"$WEEK_DAY", "7"},
    {"$WEEK_DAY_NAME", "Saturday"},
    {"$WEEK_DAY_ABBREV", "Sat"},
    {"$WEEK", "06"},
    {"$UNIXTIME", "1139650496.000"},
    {"$TZOFFSET", "+01:00"},
    {"$TZ", "+01:00"},
    {"$R_DATE", "Feb 11 19:58:35.639"},
    {"$R_FULLDATE", "2006 Feb 11 19:58:35.639"},
    {"$R_ISODATE", "2006-02-11T19:58:35.639+01:00"},
    {"$R_STAMP", "Feb 11 19:58:35.639"},
    {"$R_YEAR", "2006"},
    {"$R_YEAR_DAY", "042"},
    {"$R_MONTH", "02"},
    {"$R_MONTH_WEEK", "1"},
    {"$R_MONTH_ABBREV", "Feb"},
    {"$R_MONTH_NAME", "February"},
    {"$R_DAY", "11"},
    {"$R_HOUR", "19"},
    {"$R_MIN", "58"},
    {"$R_SEC", "35"},
    {"$R_WEEKDAY", "Sat"},
    {"$R_WEEK_DAY", "7"},
    {"$R_WEEK_DAY_NAME", "Saturday"},
    {"$R_WEEK_DAY_ABBREV", "Sat"},
    {"$R_WEEK", "06"},
    {"$R_UNIXTIME", "1139684315.639"},
    {"$R_TZOFFSET", "+01:00"},
    {"$R_TZ", "+01:00"},
    {"$S_DATE", "Feb 11 10:34:56.000"},
    {"$S_FULLDATE", "2006 Feb 11 10:34:56.000"},
    {"$S_ISODATE", "2006-02-11T10:34:56.000+01:00"},
    {"$S_STAMP", "Feb 11 10:34:56.000"},
    {"$S_YEAR", "2006"},
    {"$S_YEAR_DAY", "042"},
    {"$S_MONTH", "02"},
    {"$S_MONTH_WEEK", "1"},
    {"$S_MONTH_ABBREV", "Feb"},
    {"$S_MONTH_NAME", "February"},
    {"$S_DAY", "11"},
    {"$S_HOUR", "10"},
    {"$S_MIN", "34"},
    {"$S_SEC", "56"},
    {"$S_WEEKDAY", "Sat"},
    {"$S_WEEK_DAY", "7"},
    {"$S_WEEK_DAY_NAME", "Saturday"},
    {"$S_WEEK_DAY_ABBREV", "Sat"},
    {"$S_WEEK", "06"},
    {"$S_UNIXTIME", "1139650496.000"},
    {"$S_TZOFFSET", "+01:00"},
    {"$S_TZ", "+01:00"},
    {"$HOST_FROM", "kismacska"},
    {"$FULLHOST_FROM", "kismacska"},
    {"$HOST", "bzorp"},
    {"$FULLHOST", "bzorp"},
    {"$PROGRAM", "syslog-ng"},
    {"$PID", "23323"},
    {"$MSGHDR", "syslog-ng[23323]:"},
    {"$MSG", "árvíztűrőtükörfúrógép"},
    {"$MESSAGE", "árvíztűrőtükörfúrógép"},
    {"$SOURCEIP", "10.11.12.13"},
    {"$RCPTID", "555"},

    {"$SEQNUM", "999"},
    {"$CONTEXT_ID", "test-context-id"},
    {"$UNIQID", "cafebabe@000000000000022b"},
  };
  assert_templates_format(test_cases, sizeof(test_cases) / sizeof(test_cases[0]));
}

Test(template, test_nvpairs)
{
  TemplateFormatTestCase test_cases[] =
  {
    {"$PROGRAM/var/log/messages/$HOST/$HOST_FROM/$MONTH$DAY${QQQQQ}valami", "syslog-ng/var/log/messages/bzorp/kismacska/0211valami"},
    {"${APP.VALUE}", "value"},
    {"${APP.VALUE:-ures}", "value"},
    {"${APP.VALUE2:-ures}", "ures"},
    {"${1}", "first-match"},
    {"$1", "first-match"},
    {"$$$1$$", "$first-match$"},
  };
  assert_templates_format(test_cases, sizeof(test_cases) / sizeof(test_cases[0]));
}

Test(template, test_template_functions)
{
  /* template functions */
  TemplateFormatTestCase test_cases[] =
  {
    {"$(echo $HOST $PID)", "bzorp 23323"},
    {"$(echo \"$(echo $HOST)\" $PID)", "bzorp 23323"},
    {"$(echo \"$(echo '$(echo $HOST)')\" $PID)", "bzorp 23323"},
    {"$(echo \"$(echo '$(echo $HOST)')\" $PID)", "bzorp 23323"},
    {"$(echo '\"$(echo $(echo $HOST))\"' $PID)", "\"bzorp\" 23323"},
  };

  assert_templates_format(test_cases, sizeof(test_cases) / sizeof(test_cases[0]));
}

Test(template, test_message_refs)
{
  /* message refs */
  assert_template_format_with_context("$(echo ${HOST}@0 ${PID}@1)", "bzorp 23323");
  assert_template_format_with_context("$(echo $HOST $PID)@0", "bzorp 23323");
}

Test(template, test_syntax_errors)
{
  /* template syntax errors */
  TemplateFormatTestCase test_cases[] =
  {
    {"$unbalanced_brace}", "}"},
    {"$}", "$}"},
    {"$unbalanced_paren)", ")"},
  };

  assert_templates_format(test_cases, sizeof(test_cases) / sizeof(test_cases[0]));

  assert_template_failure("$(unbalanced_paren", "missing function name or inbalanced '('");
  assert_template_failure("${unbalanced_brace", "'}' is missing");
}

Test(template, test_compat)
{
  gint old_version;

  old_version = configuration->user_version;
  /* old version for various macros */
  configuration->user_version = 0x0201;

  start_grabbing_messages();
  assert_template_format("$MSGHDR", "syslog-ng[23323]:");
  assert_grabbed_messages_contain("the default value for template-escape has changed to 'no' from syslog-ng 3.0", NULL);
  reset_grabbed_messages();
  assert_template_format("$MSG", "syslog-ng[23323]:árvíztűrőtükörfúrógép");
  assert_grabbed_messages_contain("the meaning of the $MSG/$MESSAGE macros has changed from syslog-ng 3.0", NULL);
  stop_grabbing_messages();
  assert_template_format("$MSGONLY", "árvíztűrőtükörfúrógép");
  assert_template_format("$MESSAGE", "syslog-ng[23323]:árvíztűrőtükörfúrógép");

  configuration->user_version = old_version;
}

Test(template, test_multi_thread)
{
  /* name-value pair */
  assert_template_format_multi_thread("alma $HOST bela", "alma bzorp bela");
  assert_template_format_multi_thread("kukac $DATE mukac", "kukac Feb 11 10:34:56.000 mukac");
  assert_template_format_multi_thread("dani $(echo $HOST $DATE $(echo huha)) balint",
                                      "dani bzorp Feb 11 10:34:56.000 huha balint");
}

Test(template, test_escaping)
{
  assert_template_format_with_escaping("${APP.QVALUE}", FALSE, "\"value\"");
  assert_template_format_with_escaping("${APP.QVALUE}", TRUE, "\\\"value\\\"");
  assert_template_format_with_escaping("$(if (\"${APP.VALUE}\" == \"value\") \"${APP.QVALUE}\" \"${APP.QVALUE}\")",
                                       FALSE, "\"value\"");
  assert_template_format_with_escaping("$(if (\"${APP.VALUE}\" == \"value\") \"${APP.QVALUE}\" \"${APP.QVALUE}\")",
                                       TRUE, "\\\"value\\\"");
}

Test(template, test_user_template_function)
{
  LogTemplate *template;

  template = compile_template("this is a user-defined template function $DATE", FALSE);
  user_template_function_register(configuration, "dummy", template);
  assert_template_format("$(dummy)", "this is a user-defined template function Feb 11 10:34:56.000");
  assert_template_failure("$(dummy arg)", "User defined template function $(dummy) cannot have arguments");
  log_template_unref(template);
}

Test(template, test_template_function_args)
{
  TemplateFormatTestCase test_cases[] =
  {
    {"$(echo foo bar)", "foo bar"},
    {"$(echo 'foobar' \"barfoo\")", "foobar barfoo"},
    {"$(echo foo '' bar)", "foo  bar"},
    {"$(echo foo '')", "foo "},
  };
  assert_templates_format(test_cases, sizeof(test_cases) / sizeof(test_cases[0]));
}
