/*
 * Copyright (C) 2016 JeanMax
 *
 * Please check the CREDITS file for further information.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef D2GS_MEPH_H_
#define D2GS_MEPH_H_

#include <me.h>

#include <data/waypoints.h>

#define precast() extension("precast")->call("meph")
#define go_to_town() FATAL(extension("go_to_town")->call("meph"))
#define town_move(where) FATAL(extension("town_move")->call("meph", where))
#define do_town_chores() FATAL(extension("do_town_chores")->call("meph"))
#define waypoint(dst_area) FATAL(extension("waypoint")->call("meph", dst_area))
#define find_level_exit(from, to) FATAL(extension("find_level_exit")->call("meph", from, to))

#endif /* D2GS_MEPH_H_ */
