/*
 * Copyright (c) 2017 Balabit
 * Copyright (c) 2017 Noemi Vanyi
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
#ifndef STATS_VIEWS_H_INCLUDED
#define STATS_VIEWS_H_INCLUDED 1

#include "stats/stats-query.h"

void stats_register_written_view(StatsCluster *cluster, StatsCounterItem *processed, StatsCounterItem *dropped, StatsCounterItem *queued);
void stats_register_fifo_fillup_rate(StatsCluster *cluster_of_actual, StatsCluster *cluster_of_max, StatsCounterItem *actual, StatsCounterItem *max);

#endif

