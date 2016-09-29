/*
 * Copyright (c) 2014 Balabit
 * Copyright (c) 2014 Viktor Tusa <tusa@balabit.hu>
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

#include "pathutils.h"
#include <criterion/criterion.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

Test(pathutils, test_is_directory_return_false_in_case_of_regular_file)
{
  const gchar *FILENAME = "test.file";
  int fd, error;
  ssize_t done;

  fd = open(FILENAME, O_CREAT | O_RDWR, 0644);
  cr_assert_geq(fd, 0, "open error; fd=%d", fd);

  done = write(fd, "a", 1);
  cr_assert_eq(done, 1, "write error; done=%d", done);

  error = close(fd);
  cr_assert_not(error, "close error");
  cr_assert_not(is_file_directory(FILENAME), "File is not a directory!");

  error = unlink(FILENAME);
  cr_assert_not(error, "unlink error");
}
