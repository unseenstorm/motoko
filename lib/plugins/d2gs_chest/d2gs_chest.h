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

#ifndef D2GS_CHEST_H_
#define D2GS_CHEST_H_

#include <data/waypoints.h>

#include <me.h>

typedef void (*cleanup_handler)(void *);

typedef enum {
	NEUTRAL = 0,
	OPERATING = 1,
	OPENEDED = 2,
	SPECIAL1 = 3,
	SPECIAL2 = 4,
	SPECIAL3 = 5,
	SPECIAL4 = 6,
	SPECIAL5 = 7
} e_chest_mode;

typedef enum { // ->interaction type (last byte) */
	GENERAL_OBJECT = 0x00,
	REFILING_SHRINE = 0x02,
	HEALTH_SHRINE = 0x02,
	MANA_SHRINE = 0x03,
	TRAPPED_CHEST = 0x05,
	MONSTER_CHEST = 0x08, //not monster-shrine :p
	POISON_RES_SHRINE = 0x0b,
	SKILL_SHRINE = 0x0c,
	MANA_RECHARGE_SHRINE = 0x0d,
	TOWN_PORTAL = 0x4b,
	LOCKED = 0x80
} e_chest_interact_type;

typedef struct {
	object_t object;
	word preset_id;
	bool is_locked;
	bool is_opened;
} chest_t;


#define precast() extension("precast")->call("chest")
#define go_to_town() FATAL(extension("go_to_town")->call("chest"))
#define town_move(where) FATAL(extension("town_move")->call("chest", where))
#define do_town_chores() FATAL(extension("do_town_chores")->call("chest"))
#define waypoint(dst_area) FATAL(extension("waypoint")->call("chest", dst_area))
#define find_level_exit(from, to) FATAL(extension("find_level_exit")->call("chest", from, to))

#endif /* D2GS_CHEST_H_ */
