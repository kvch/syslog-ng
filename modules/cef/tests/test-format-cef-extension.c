/*
 * Copyright (c) 2015 Balabit
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
 */

#include "template_lib.h"
#include "apphook.h"
#include "plugin.h"
#include "cfg.h"

#include <criterion/criterion.h>
#include <stdarg.h>


typedef struct _CEFTestCaseWithFormat
{
  const gchar *format;
  NVPairStub *nv_pairs;
  const gint nr_of_nv_pairs;
  const gchar *expected;
} CEFTestCaseWithFormat;

typedef struct _CEFTestCase
{
  NVPairStub *nv_pairs;
  const gint nr_of_nv_pairs;
  const gchar *expected;
} CEFTestCase;

__attribute__((constructor))
static void global_test_init(void)
{
  app_startup();
  putenv("TZ=UTC");
  tzset();
  init_template_tests();
  plugin_load_module("cef", configuration, NULL);
}

__attribute__((destructor))
static void global_test_deinit(void)
{
  deinit_template_tests();
  app_shutdown();
}

static void
_expect_cef_result_properties_list(const gchar *expected, NVPairStub *nv_pairs, gint nr_of_nvpairs)
{
  LogMessage *msg = message_from_list(nv_pairs, nr_of_nvpairs);

  assert_template_format_msg("$(format-cef-extension --subkeys .cef.)", expected, msg);
  log_msg_unref(msg);
}

static void
_expect_cef_result(const gchar *expected, NVPairStub *nv_pairs, gint nr_of_nvpairs)
{
  configuration->template_options.on_error = ON_ERROR_DROP_MESSAGE | ON_ERROR_SILENT;
  _expect_cef_result_properties_list(expected, nv_pairs, nr_of_nvpairs);
}

static void
_expect_skip_bad_property(const gchar *expected, NVPairStub *nv_pairs, gint nr_of_nvpairs)
{
  configuration->template_options.on_error = ON_ERROR_DROP_PROPERTY | ON_ERROR_SILENT;
  _expect_cef_result_properties_list(expected, nv_pairs, nr_of_nvpairs);
}

static void
_expect_drop_message(NVPairStub *nv_pairs, gint nr_of_nvpairs)
{
  _expect_cef_result("", nv_pairs, nr_of_nvpairs);
}

static void
_expect_cef_result_format_va(CEFTestCaseWithFormat c)
{
  LogMessage *msg = message_from_list(c.nv_pairs, c.nr_of_nv_pairs);

  configuration->template_options.on_error = ON_ERROR_DROP_MESSAGE | ON_ERROR_SILENT;
  assert_template_format_msg(c.format, c.expected, msg);
  log_msg_unref(msg);
}

Test(cef_format_extension, test_null_in_value)
{
  LogMessage *msg = create_empty_message();

  configuration->template_options.on_error = ON_ERROR_DROP_MESSAGE | ON_ERROR_SILENT;
  log_msg_set_value_by_name(msg, ".cef.k", "a\0b", 3);
  assert_template_format_msg("$(format-cef-extension --subkeys .cef.)", "k=a\\u0000b", msg);
  log_msg_unref(msg);
}

Test(cef_format_extension, test_filter)
{
  gchar *expected = "k=v";
  NVPairStub nv_pairs[] =
  {
      {".cef.k", "v"},
      {"x", "w"},
  };
  _expect_cef_result(expected, nv_pairs, 2);
}

Test(cef_format_extension, test_multiple_properties_with_space)
{
  const gchar *expected = "act=c:/program files dst=10.0.0.1";
  NVPairStub nv_pairs[] =
  {
    {".cef.act", "c:/program files"},
    {".cef.dst", "10.0.0.1"},
  };
  _expect_cef_result(expected, nv_pairs, 2);
}

Test(cef_format_extension, test_multiple_properties)
{
  NVPairStub nv_pairs[] = 
  {
    {"cef.k", "v"},
    {"cef.x", "y"},
  };
  CEFTestCaseWithFormat test_case = {"", nv_pairs, 2, "k=v x=y" };
  _expect_cef_result_format_va(test_case);
}

Test(cef_format_extension, test_drop_property)
{
  NVPairStub nv_pairs[] = 
  {
    {".cef.a|b", "c"},
    {".cef.kkk", "v"},
    {".cef.x=y", "w"},
  };
  _expect_skip_bad_property("kkk=v", nv_pairs, 3);
}

Test(cef_format_extension, test_drop_message)
{
  NVPairStub nv_pairs[] = 
  {
    {".cef.a|b", "c"},
    {".cef.kkk", "v"},
    {".cef.x=y", "w"},
  };
  _expect_drop_message(nv_pairs, 3);
}

Test(cef_format_extension, test_empty)
{
  _expect_cef_result("", NULL, 0);
}

/*
Test(cef_format_extension, test_inline)
{
  _expect_cef_result_format("$(format-cef-extension --subkeys .cef. .cef.k=v)", "k=v");
}
*/

Test(cef_format_extension, test_space)
{
  const gchar *expected = "act=blocked a ping";
  NVPairStub nv_pairs[] =
  {
    {".cef.act", "blocked a ping"},
  };
  _expect_cef_result(expected, nv_pairs, 1);
}

/*
Test(cef_format_extension, test_charset)
{
  _expect_drop_message(".cef.árvíztűrőtükörfúrógép", "v");
  _expect_cef_result("k=árvíztűrőtükörfúrógép", ".cef.k", "árvíztűrőtükörfúrógép");

  _expect_cef_result("k=\\xff", ".cef.k", "\xff");
  _expect_cef_result("k=\\xc3", ".cef.k", "\xc3");
  _expect_drop_message(".cef.k\xff", "v");
  _expect_drop_message(".cef.k\xc3", "v");
}
*/
Test(cef_format_extension, test_escaping)
{
  NVPairStub stub[] = 
  {
    { ".cef.act", "\\"},
  };
  CEFTestCaseWithFormat test_cases[] =
  {
    {"act=\\\\", stub[0], 1},
    {"act=\\\\\\\\", (NVPairStub *) {".cef.act", "\\\\"}, 1},
    {"act=\\=", (NVPairStub *) {".cef.act", "="}, 1},
    {"act=|", (NVPairStub *) {".cef.act", "|"}, 1},
    {"act=\\u0009", (NVPairStub *) {".cef.act", "\t"}, 1},
    {"act=\\n", (NVPairStub *) {".cef.act", "\n"}, 1},
    {"act=\\r", (NVPairStub *) {".cef.act", "\r"}, 1},
    {"act=v\\n", (NVPairStub *) {".cef.act", "v\n"}, 1},
    {"act=v\\r", (NVPairStub *) {".cef.act", "v\r"}, 1},
    {"act=u\\nv", (NVPairStub *) {".cef.act", "u\nv"}, 1},
    {"act=\\r\\n", (NVPairStub *) {".cef.act", "\r\n"}, 1},
    {"act=\\n\\r", (NVPairStub *) {".cef.act", "\n\r"}, 1},
    {"act=this is a long value \\= something", (NVPairStub *) {".cef.act", "this is a long value = something"}, 1},
  };
  gint i;
  for (i = 0; i < 14; i++)
    _expect_cef_result(test_cases[i].expected, test_cases[i].nv_pairs, test_cases[i].nr_of_nvpairs);

  /*
  _expect_drop_message(".cef.k=w", "v");
  _expect_drop_message(".cef.k|w", "v");
  _expect_drop_message(".cef.k\\w", "v");
  _expect_drop_message(".cef.k\nw", "v");
  _expect_drop_message(".cef.k w", "v");
  */
}

/*
Test(cef_format_extension, test_prefix)
{
  configuration->template_options.on_error = ON_ERROR_DROP_MESSAGE | ON_ERROR_SILENT;

  _expect_cef_result_format("$(format-cef-extension --subkeys ..)", "k=v", "..k", "v");
  _expect_cef_result_format("$(format-cef-extension --subkeys ,)", "k=v", ",k", "v");
  _expect_cef_result_format("$(format-cef-extension --subkeys .cef.)", "", "k", "v");
  _expect_cef_result_format("$(format-cef-extension --subkeys ' ')", "k=v", " k", "v");
  _expect_cef_result_format("$(format-cef-extension --subkeys \" \")", "k=v", " k", "v");

  _expect_cef_result_format("$(format-cef-extension x=y)", "x=y", "k", "v");
  _expect_cef_result_format("$(format-cef-extension)", "", "k", "v");

  assert_template_failure("$(format-cef-extension --subkeys)",
                          "Missing argument for --subkeys");
  assert_template_failure("$(format-cef-extension --subkeys '')",
                          "Error parsing value-pairs: --subkeys requires a non-empty argument");
  assert_template_failure("$(format-cef-extension --subkeys \"\")",
                          "Error parsing value-pairs: --subkeys requires a non-empty argument");
}

Test(cef_format_extension, test_macro_parity)
{
  _expect_cef_result("", "k");
  _expect_cef_result_format("", "");
  _expect_cef_result_format("", "", "k");
  _expect_drop_message("");
  _expect_drop_message("", "k");
  _expect_skip_bad_property("", NULL, 0);
}
*/
