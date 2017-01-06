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
#include "messages.h"

#include <stdlib.h>
#include <string.h>

gchar LOG_PATH_TEMPLATE[] = " %s(%s);";
gchar DRIVER_TEMPLATE[] = "%s %s { %s};\n";
gchar DRIVER_NAME_TEMPLATE[] = "%s%d";
gchar CFG_FILE_TEMPLATE[] = "@version: %s\n"
                            "@include scl.conf\n"
                            "source s_stdin { stdin(); };\n"
                            "destination d_stdout { stdout(); };\n"
                            "%s"
                            "log {source(s_stdin); %s destination(d_stdout);};";


gchar*
_generate_name(gchar *driver_type)
{
  gchar generated_name[50] = "";
  gint random_number = rand();
  sprintf(generated_name, DRIVER_NAME_TEMPLATE, driver_type, random_number);

  msg_debug("Generated name", evt_tag_str("driver_type", driver_type), evt_tag_str("name", generated_name));

  return g_strndup(generated_name, strlen(generated_name));
}

CliParam*
cli_param_new(gchar *type, gchar *cfg)
{
  CliParam *self = g_new0(CliParam, 1);
  self->type = type;
  self->cfg = cfg;
  self->name = _generate_name(type);
  return self;
}

gboolean
_get_cfg_string(CliParam *cfg, gchar *cfg_lines, gchar *cfg_path)
{
  gchar cfg_buffer[2048] = "";
  if (sprintf(cfg_buffer, LOG_PATH_TEMPLATE, cfg->type, cfg->name) < 1)
      return FALSE;
  strcat(cfg_path, cfg_buffer);

  if (sprintf(cfg_buffer, DRIVER_TEMPLATE, cfg->type, cfg->name, cfg->cfg) < 1)
      return FALSE;
  strcat(cfg_lines, cfg_buffer);
  return TRUE;
}

gboolean
_write_to_file_pointer(Cli *self, gchar* cfg_lines, gchar *cfg_path, GlobalConfig *global_config)
{
  gchar generated_cfg_lines[2048] = "";
  if(sprintf(generated_cfg_lines, CFG_FILE_TEMPLATE, SYSLOG_NG_VERSION, cfg_lines, cfg_path) < 1)
      return FALSE;
  global_config->cfg_file = fmemopen(generated_cfg_lines, strlen(generated_cfg_lines), "r");
  self->generated_config = generated_cfg_lines;
  return TRUE;
}

gboolean
cli_init_cfg(Cli *self, GlobalConfig *global_config)
{
  GList *current_cfg;
  gchar cfg_lines[5000] = "";
  gchar cfg_path[1000] = "";

  for (current_cfg = self->params; current_cfg != NULL; current_cfg = current_cfg->next)
    {
      CliParam *cli_param = (CliParam *) current_cfg->data;
      if (!_get_cfg_string(cli_param, cfg_lines, cfg_path))
          return FALSE;
    }
  if (!_write_to_file_pointer(self, cfg_lines, cfg_path, global_config))
      return FALSE;

  return TRUE;
}

gboolean
_more_tokens(gchar *token)
{
  return (token != NULL);
}

gboolean
_is_driver_ending(gchar *token)
{
  size_t token_length = strlen(token);
  return (token[token_length - 1] == ';');
}

gboolean
cli_setup_params(Cli *cli)
{
  guint i;
  const gchar delimiter[2] = " ";

  for (i = 0; cli->raw_params[i]; i++)
  {
      gchar *driver_type = NULL;
      gchar driver_cfg[1000] = "";
      gchar *current_arg = g_strndup(cli->raw_params[i], strlen(cli->raw_params[i]));
      gchar *token = strtok(current_arg, delimiter);
      while (_more_tokens(token))
        {
          if (driver_type == NULL)
              driver_type = token;
          else
              sprintf(driver_cfg, "%s %s", driver_cfg, token);

          if (_is_driver_ending(token))
            {
              if (driver_cfg[0] == '\0')
                {
                  cli->params = NULL;
                  return FALSE;
                }
              cli->params = g_list_append(cli->params, cli_param_new(driver_type,
                                                                     g_strndup(driver_cfg, strlen(driver_cfg))));
              driver_type = NULL;
              driver_cfg[0] = '\0';
            }
          token = strtok(NULL, delimiter);
        }
      if (driver_type != NULL || driver_cfg[0] != '\0')
        {
          cli->params = NULL;
          return FALSE;
        }
  }
  return TRUE;
}

gboolean
cli_write_generated_config_to_file(Cli *self)
{
  FILE *f = fopen("generated.conf", "w");
  if (f == NULL)
      return FALSE;

  if(fwrite(self->generated_config, 1, strlen(self->generated_config), f) < 1)
      return FALSE;
  return TRUE;
}

Cli*
cli_new(gchar **args, gboolean is_cli_param)
{
  Cli *self = g_new0(Cli, 1);
  self->raw_params = args;
  self->is_command_line_drivers = (self->raw_params != NULL);
  self->is_cli = self->is_command_line_drivers || is_cli_param;
  self->params = NULL;
  return self;
}
