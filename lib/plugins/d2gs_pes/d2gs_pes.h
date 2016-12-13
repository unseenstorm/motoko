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

#ifndef D2GS_PES_H_
#define D2GS_PES_H_

#include <me.h>

#include <data/waypoints.h>
#include <data/monster_fields.h>
#include <data/monster_names.h>
#include <data/super_uniques.h>
#include <data/skills.h>

typedef void (*cleanup_handler)(void *);

typedef struct {
	object_t object;
	word index;
	byte hp;
	bool super_unique;
	word super_unique_index;
	unsigned long long int mods;
} npc_t;

typedef void(*function_t)(object_t *, dword);

typedef struct {
	function_t function;
	dword arg;
} action_t;

#define N_NPC_MOD 43
#define NPC_HP(hp) ((byte) ((((double) (((byte) hp > 128) ? (hp ^ 0xFF) : hp)) / 128) * 100))
#define NPC_MOD(npc, mod) (npc->mods & mod)

#define npc_lookup_name(npc) (((npc)->super_unique && (npc)->super_unique_index < n_super_uniques) ? npc_superunique_lookup_name((npc)->super_unique_index) : npc_default_lookup_name((npc)->index))




#define precast() extension("precast")->call("pes")
#define take_portal(id) FATAL(extension("take_portal")->call("pes", id))
#define go_to_town() FATAL(extension("go_to_town")->call("pes"))
#define town_move(where) FATAL(extension("town_move")->call("pes", where))
#define do_town_chores() FATAL(extension("do_town_chores")->call("pes"))
#define waypoint(dst_area) FATAL(extension("waypoint")->call("pes", dst_area))
#define find_level_exit(from, to) FATAL(extension("find_level_exit")->call("pes", from, to))

#endif /* D2GS_PES_H_ */
