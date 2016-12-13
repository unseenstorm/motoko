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

#ifndef TOWN_H_
#define TOWN_H_

#include <data/monster_fields.h>
#include <data/monster_names.h>
#include <data/super_uniques.h>
#include <data/waypoints.h>

#include <me.h>

#define ACT3_SPAWN_NODE_X    5132
#define ACT3_SPAWN_NODE_Y    5168

#define ACT3_WP_SPOT_X       5152
#define ACT3_WP_SPOT_Y       5052

#define ACT3_TP_SPOT_X       5154
#define ACT3_TP_SPOT_Y       5067

#define ORMUS_NODE_X         5152
#define ORMUS_NODE_Y         5094

#define ORMUS_SPOT_X         5132
#define ORMUS_SPOT_Y         5094

#define ACT2_WP_SPOT_X       5068
#define ACT2_WP_SPOT_Y       5090

#define LYSANDER_SPOT_X      5115
#define LYSANDER_SPOT_Y      5104

#define FARA_SPOT_X          5115
#define FARA_SPOT_Y          5083

#define ACT2_CENTRAL_NODE_X  5112
#define ACT2_CENTRAL_NODE_Y  5095

#define GREIZ_NODE_X         5068
#define GREIZ_NODE_Y         5065

#define GREIZ_SPOT_X         5034
#define GREIZ_SPOT_Y         5049

#define ACT2_SPAWN_NODE1_X   5125
#define ACT2_SPAWN_NODE1_Y   5175

#define ACT2_SPAWN_NODE2_X   5125
#define ACT2_SPAWN_NODE2_Y   5108

#define N_NPC_MOD 43

#define npc_lookup_name(npc) (((npc)->super_unique && (npc)->super_unique_index < n_super_uniques) ? npc_superunique_lookup_name((npc)->super_unique_index) : npc_default_lookup_name((npc)->index))

typedef struct {
	object_t object;
	word index;
	byte hp;
	bool super_unique;
	word super_unique_index;
	unsigned long long int mods;
} npc_t;

typedef void (*cleanup_handler)(void *);

extern struct list module_settings_list;
extern int merc_rez;
extern e_town_move g_previous_dest;
extern struct list npcs;
extern struct list npcs_l;


#define walk(x, y) FATAL(me_move(x, y))

bool waypoint(dword area);
bool town_move(e_town_move where);
bool change_act(int dst_act);
bool do_town_chores(void);
bool go_to_town(void);

void town_init(void);
void town_finit(void);

#endif /* TOWN_H_ */
