/*
 * Copyright (c) 2012-2015 Balabit
 * Copyright (c) 2012-2015 Balázs Scheidler <balazs.scheidler@balabit.com>
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
#include <criterion/criterion.h>

__attribute__((constructor))
static void global_test_init(void)
{
  app_startup();
  init_template_tests();
  plugin_load_module("cryptofuncs", configuration, NULL);
}

__attribute__((destructor))
static void global_test_deinit(void)
{
  app_shutdown();
}

Test(cryptofuncs, test_hash)
{
  TemplateFormatTestCase test_cases[] =
  {
    {"$(sha1 foo)", "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33"},
    {"$(sha1 bar)", "62cdb7020ff920e5aa642c3d4066950dd1f01f4d"},
    {"$(md5 foo)", "acbd18db4cc2f85cedef654fccc4a4d8"},
    {"$(hash foo)", "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae"},
    {"$(md4 foo)", "0ac6700c491d70fb8650940b1ca1e4b2"},
    {"$(sha256 foo)", "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae"},
    {"$(sha512 foo)", "f7fbba6e0636f890e56fbbf3283e524c6fa3204ae298382d624741d0dc6638326e282c41be5e4254d8820772c5518a2c5a8c0c7f7eda19594a7eb539453e1ed7"},
    {"$(sha1 --length 5 foo)", "0beec"},
    {"$(sha1 -l 5 foo)", "0beec"},
    {"$(sha1 --length 99999 foo)", "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33"},
    {"$(sha1 foo bar)", "8843d7f92416211de9ebb963ff4ce28125932878"},
    {"$(sha1 \"foo bar\")", "3773dea65156909838fa6c22825cafe090ff8030"},
    {"$(md5 $(sha1 foo) bar)", "196894290a831b2d2755c8de22619a97"},
  };
  assert_templates_format(test_cases, sizeof(test_cases) / sizeof(test_cases[0]));

  assert_template_failure("$(sha1)", "$(hash) parsing failed, invalid number of arguments");
  assert_template_failure("$(sha1 --length 5)", "$(hash) parsing failed, invalid number of arguments");
  assert_template_failure("$(sha1 --length invalid_length_specification foo)",
                          "Cannot parse integer value 'invalid_length_specification' for --length");
}
