/*
 * Copyright (c) 2002-2014 Balabit
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
#include "type-hinting.h"
#include "apphook.h"

#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


typedef struct _TypeHintTestCase
{
  const gchar *typename;
  const gint expected;
} TypeHintTestCase;

typedef struct _BooleanHintTestCase
{
  const gchar *value;
  const gboolean expected;
} BooleanHintTestCase;

__attribute__((constructor))
static void global_test_init(void)
{
  app_startup();
}

__attribute__((destructor))
static void global_test_deinit(void)
{
  app_shutdown();
}

static void
assert_error(GError *error, gint code, const gchar *expected_message)
{
  cr_assert_not_null(error, "GError expected to be non-NULL");

  cr_assert_eq(error->code, code, "GError error code is as expected");
  if (expected_message)
    cr_assert_str_eq(error->message, expected_message, "GError error message is as expected");
}

static void
assert_type_hint(TypeHintTestCase c)
{
  TypeHint t;
  GError *e = NULL;

  cr_assert(type_hint_parse(c.typename, &t, &e), "Parsing '%s' as type hint", c.typename);
  cr_assert_eq(t, c.expected,"Parsing '%s' as type hint results in correct type", c.typename);
}

Test(type_hints, test_type_hint_parse)
{
  TypeHintTestCase test_cases[] =
  {
    {NULL, TYPE_HINT_STRING},
    {"string", TYPE_HINT_STRING},
    {"literal", TYPE_HINT_LITERAL},
    {"boolean", TYPE_HINT_BOOLEAN},
    {"int", TYPE_HINT_INT32},
    {"int32", TYPE_HINT_INT32},
    {"int64", TYPE_HINT_INT64},
    {"double", TYPE_HINT_DOUBLE},
    {"datetime", TYPE_HINT_DATETIME},
    {"default", TYPE_HINT_DEFAULT},
  };
  gint i, nr_of_cases;

  nr_of_cases = sizeof(test_cases) / sizeof(test_cases[0]);
  for (i = 0; i < nr_of_cases; i++)
    assert_type_hint(test_cases[i]);

  TypeHint t;
  GError *e = NULL;
  cr_assert_not(type_hint_parse("invalid-hint", &t, &e),
               "Parsing an invalid hint results in an error.");

  assert_error(e, TYPE_HINTING_INVALID_TYPE, "Unknown type specified in type hinting: invalid-hint");
}

/*
#define assert_type_cast_fail(target,value,out)                 \
  do                                                            \
    {                                                           \
      assert_false(type_cast_to_##target(value, out, &error),   \
                   "Casting '%s' to %s fails", value, #target); \
      assert_error(error, TYPE_HINTING_INVALID_CAST, NULL);     \
      error = NULL;                                             \
    } while(0)

#define assert_double_cast(value,expected)                              \
  do                                                                    \
    {                                                                   \
      gdouble od;                                                       \
      assert_type_cast(double, value, &od);                             \
      assert_gdouble(od, expected, "'%s' casted to double is %s",       \
                      value, #expected);                                \
    } while(0)

#define assert_int_cast(value,width,expected)                           \
  do                                                                    \
    {                                                                   \
      gint##width i;                                                    \
      assert_type_cast(int##width, value, &i);                          \
      assert_gint##width(i, expected, "'%s' casted to int%s is %u",     \
                         value, expected);                              \
    } while(0) \
*/

  /*
static void
assert_bool_cast(BooleanHintTestCase c)
{
  gboolean ob;
  GError error;

  cr_assert(type_cast_to_boolean(c.value, ob, &error), "Casting '%s' to boolean does not work", value);
  assert_no_error(error, "Successful casting returns no error");    \
  assert_gboolean(ob, c.expected, "'%s' casted to boolean is %s", value, #expected);
}

Test(type_hints, test_type_cast)
{
  gint32 i32;
  gint64 i64;
  guint64 dt;
  gdouble d;

  BooleanHintTestCase boolean_test_cases[] =
  {
      {"True", TRUE},
      {"true", TRUE},
      {"1", TRUE},
      {"totally true", TRUE},
      {"False", FALSE},
      {"false", FALSE},
      {"0", FALSE},
      {"fatally false", FALSE},
  };
  gint nr_of_boolean_cases, i;
  nr_of_boolean_cases = sizeof(boolean_test_cases) / sizeof(boolean_test_cases[0]);
  for (i = 0; i < nr_of_boolean_cases; i++)
      assert_bool_cast(boolean_test_cases[i]);

  {
    gboolean ob;
    assert_type_cast_fail(boolean, "booyah", &ob);
  }

  assert_int_cast("12345", 32, 12345);
  assert_type_cast_fail(int32, "12345a", &i32);

  assert_int_cast("12345", 64, 12345);
  assert_type_cast_fail(int64, "12345a", &i64);

  assert_double_cast("1.0", 1.0);
  assert_type_cast_fail(double, "2.0bad", &d);
  assert_type_cast_fail(double, "bad", &d);
  assert_type_cast_fail(double, "", &d);
  assert_type_cast_fail(double, "1e1000000", &d);
  assert_type_cast_fail(double, "-1e1000000", &d);
  assert_double_cast("1e-100000000", 0.0);
#ifdef INFINITY
  assert_double_cast("INF", INFINITY);
#endif

  assert_type_cast(datetime_int, "12345", &dt);
  assert_guint64(dt, 12345000, "Casting '12345' to datetime works");
  assert_type_cast(datetime_int, "12345.5", &dt);
  assert_guint64(dt, 12345500, "Casting '12345.5' to datetime works");
  assert_type_cast(datetime_int, "12345.54", &dt);
  assert_guint64(dt, 12345540, "Casting '12345.54' to datetime works");
  assert_type_cast(datetime_int, "12345.543", &dt);
  assert_guint64(dt, 12345543, "Casting '12345.543' to datetime works");
  assert_type_cast(datetime_int, "12345.54321", &dt);
  assert_guint64(dt, 12345543, "Casting '12345.54321' to datetime works");

  assert_type_cast_fail(datetime_int, "invalid", &dt);
}
  */
