/*
 * Copyright (c) 2002-2017 Balabit
 * Copyright (c) 1998-2012 Balázs Scheidler
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
#include "str-utils.h"

GString *
g_string_assign_len(GString *s, const gchar *val, gint len)
{
  g_string_truncate(s, 0);
  if (val && len)
    g_string_append_len(s, val, len);
  return s;
}

void
g_string_steal(GString *s)
{
  s->str = g_malloc0(1);
  s->allocated_len = 1;
  s->len = 0;
}

static gchar *
str_replace_char(const gchar *str, const gchar from, const gchar to)
{
  gchar *p;
  gchar *ret = g_strdup(str);
  p = ret;
  while (*p)
    {
      if (*p == from)
        *p = to;
      p++;
    }
  return ret;
}

gchar *
__normalize_key(const gchar *buffer)
{
  return str_replace_char(buffer, '-', '_');
}

size_t
_get_str_array_len(gchar *arr[])
{
  size_t len = 0;
  while (arr[len] != NULL)
    len++;
  return len;
}

gchar *
str_array_join(const gchar *separator, gchar *argv[])
{
  size_t i;
  gchar *result = "";
  size_t len = _get_str_array_len(argv);

  if (len == 0)
    return NULL;

  for(i = 0; i < len - 1; i++)
    result = g_strconcat(result, argv[i], separator, NULL);
  result = g_strconcat(result, argv[len-1], NULL);

  return result;
}
