/*
 * copyright (c) 2010 gonzoj
 *
 * please check the CREDITS file for further information.
 *
 * this program is free software: you can redistribute it and/or modify
 * it under the terms of the gnu general public license as published by
 * the free software foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * this program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * gnu general public license for more details.
 *
 * you should have received a copy of the gnu general public license
 * along with this program.  if not, see <http://www.gnu.org/licenses/>.
 */

#define _PLUGIN

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <util/compat.h>

#include <module.h>

#include <moduleman.h>
#include <d2gs.h>
#include <packet.h>
#include <util/net.h>
#include <util/list.h>
#include <util/string.h>
#include <settings.h>
#include <gui.h>
#include <me.h>
extern bot_t me;

#include <data/monster_fields.h>
#include <data/monster_names.h>
#include <data/super_uniques.h>
#include <data/skills.h>

#include <util/config.h>
#include <util/system.h>

/* ---------------------------- */

typedef void (*cleanup_handler)(void *);

/* SETTINGS */

static struct setting module_settings[] = (struct setting []) {
	SETTING("UseMerc", FALSE, BOOLEAN),
	SETTING("CastDelay", 250, INTEGER),
	SETTING("ShopAfterRuns", 1, INTEGER),
	SETTING("RepairAfterRuns", 1, INTEGER),
	SETTING("MinGameTime", 180, INTEGER)
};

static struct list module_settings_list = LIST(module_settings, struct setting, 5);

/*
static const int waypoints[][] = {
	{0x01, 0x03, 0x04, 0x05, 0x06, 0x1b, 0x1d, 0x20, 0x23},
	{0x28, 0x30, 0x2a, 0x39, 0x2b, 0x2c, 0x34, 0x4a, 0x2e},
	{0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x53, 0x65},
	{0x67, 0x6a, 0x6b},
	{0x6d, 0x6f, 0x70, 0x71, 0x73, 0x7b, 0x75, 0x76, 0x81}
};
*/

static const word chest_preset_ids[] = {
	1, 3, 4, 7, 9, 50, 51, 52, 53, 54, 55, 56, 57, 58, 79, 80, 84, 85, 88, 89, 90, 93, 94, 95, 96, 97, 109, 138, 139, 142, 143, 154, 158, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 178, 182, 185, 186, 187, 188, 204, 207, 208, 209, 210, 211, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 227, 228, 233, 234, 235, 244, 247, 248, 259, 266, 268, 269, 270, 271, 272, 283, 284, 285, 286, 287, 289, 326, 337, 338, 339, 358, 359, 360, 362, 363, 364, 365, 369, 372, 373, 380, 381, 383, 384, 388, 416, 418, 419, 426, 438, 439, 443, 444, 445, 450, 463, 466, 467, 468, 469, 470, 471, 473, 477, 554, 556  ,//test
	5, 6, 87, 104, 105, 106, 107, 143, 140, 141, 144, 146, 147, 148, 176, 177,
	181, 183, 198, 240, 241, 242, 243, 329, 330, 331, 332, 333, 334, 335, 336,
	354, 355, 356, 371, 387, 389, 390, 391, 397, 405, 406, 407, 413, 420, 424,
	425, 430, 431, 432, 433, 454, 455, 501, 502, 504, 505, 580, 581, 0
};

static const word wp_preset_ids[] = {
	119, 145, 156, 157, 237, 238, 288, 323, 324, 398, 402, 429, 494, 496, 511, 539, 0
};

#define WP_LUTGHOLEIN   0x28
#define WP_KURASTDOCKS  0x4b
#define WP_LOWERKURAST  0x4f
#define WP_KURASTBAZAAR 0x50
#define WP_UPPERKURAST  0x51

typedef enum {
	VOID = 0,
	WP,
	TP,
	ORMUS,
	FARA,
	LYSANDER,
	GREIZ
} e_town_move;

#define town_move(where) if (!_town_move(where)) return FALSE
#define walk_straight(x, y) if (!_walk_straight(x, y)) return FALSE
#define moveto(x, y) if (!_moveto(x, y)) return FALSE
#define waypoint(area) if (!_waypoint(area)) return FALSE
#define townportal() if (!_townportal()) return FALSE

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

#define ACT2_CENTRAL_NODE_X  5115
#define ACT2_CENTRAL_NODE_Y  5097

#define GREIZ_NODE_X         5068
#define GREIZ_NODE_Y         5055

#define GREIZ_SPOT_X         5042
#define GREIZ_SPOT_Y         5055

#define ACT2_SPAWN_NODE1_X   5125
#define ACT2_SPAWN_NODE1_Y   5175

#define ACT2_SPAWN_NODE2_X   5125
#define ACT2_SPAWN_NODE2_Y   5114

static e_town_move g_previous_dest = 0;

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

#define module_setting(name) ((struct setting *)list_find(&module_settings_list, (comparator_t) compare_setting, name))

/* NPCs / OBJECTS */

typedef struct {
	object_t object;
	word index;
	byte hp;
	bool super_unique;
	word super_unique_index;
	unsigned long long int mods;
} npc_t;

typedef struct {
	object_t object;
	word preset_id;
	bool is_locked;
	bool is_opened;
} chest_t;

#define N_NPC_MOD 43

#define object_clear(o) memset(&(o), 0, sizeof(object_t))

static struct list npcs;

static struct list npcs_l;

static struct list chests_l;

pthread_mutex_t npcs_m;

pthread_mutex_t chests_m;

static object_t merc;

static object_t tp;

static object_t wp;

static int scrolls;

static dword scroll_id;

pthread_cond_t npc_interact_cv;

pthread_mutex_t npc_interact_mutex;

pthread_cond_t townportal_interact_cv;

pthread_mutex_t townportal_interact_mutex;

/* ACTIONS */

typedef void(*function_t)(dword);

typedef struct {
	function_t function;
	dword arg;
} action_t;

static struct list precast_sequence = LIST(NULL, action_t, 0);

static word cur_rskill;
static word cur_lskill;

pthread_mutex_t switch_slot_m;

pthread_cond_t switch_slot_cv;

/* STATISTICS */

static time_t run_start;

static int runs;

static int runtime;

static int merc_rez;

/* ---------------------------- */

void cast_self(dword arg) {
	(void)arg;
	d2gs_send(0x0c, "%w %w", me.obj.location.x, me.obj.location.y);

	msleep(module_setting("CastDelay")->i_var);
}

void cast_right(dword arg) {
	(void)arg;
	d2gs_send(0x0d, "01 00 00 00 %d", me.obj.id);

	msleep(module_setting("CastDelay")->i_var);
}

void cast_left(dword arg) {
	(void)arg;
	d2gs_send(0x06, "01 00 00 00 %d", me.obj.id);

	msleep(module_setting("CastDelay")->i_var);
}

void swap_right(dword arg) {
	if (cur_rskill == arg) {
		return;
	}

	d2gs_send(0x3c, "%w 00 00 ff ff ff ff", arg & 0xFFFF);

	msleep(300);

	cur_rskill = arg;
}

void swap_left(dword arg) {
	if (cur_lskill == arg) {
		return;
	}

	d2gs_send(0x3c, "%w 00 80 ff ff ff ff", arg & 0xFFFF);

	msleep(300);

	cur_lskill = arg;
}

void switch_slot(dword arg) {
	(void)arg;
	msleep(300);

	pthread_mutex_lock(&switch_slot_m);
	pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &switch_slot_m);

	d2gs_send(0x60, "");

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 2;

	pthread_cond_timedwait(&switch_slot_cv, &switch_slot_m, &ts);

	pthread_cleanup_pop(1);

	msleep(200);
}

int skill_lookup(char *skill) {
	if (string_is_numeric(skill)) {
		int d;
		sscanf(skill, "%i", &d);

		return d;
	} else {
		int i;
		for (i = 0; i < n_skills; i++) {
			if (string_compare((char *) skills[i], skill, FALSE)) {
				return i;
			}
		}

		return -1;
	}
}

void assemble_sequence(struct list *sequence, char *description) {
	int lskill = -1, rskill = -1;

	string_to_lower_case(description);

	char *tok = strtok(description, "/");
	while (tok) {
		if (!strcmp(tok, "switch")) {
			action_t a = { switch_slot, 0 };
			list_add(sequence, &a);
		} else {
			char *separator = strchr(tok, ':');
			if (separator) {
				*separator = '\0';
				char *side = tok;
				char *s_skill = ++separator;
				int skill = skill_lookup(s_skill);

				if (skill >= 0) {

					if (!strcmp(side, "left") && skill >= 0) {
						if (skill != lskill) {
							action_t a = { swap_left, skill };
							list_add(sequence, &a);
							lskill = skill;
						}

						action_t a = { cast_left, 0 };
						list_add(sequence, &a);
					} else if (!strcmp(side, "right") && skill >= 0) {
						if (skill != rskill) {
							action_t a = { swap_right, skill };
							list_add(sequence, &a);
							rskill = skill;
						}

						action_t a = { cast_right, 0 };
						list_add(sequence, &a);
					} else if (!strcmp(side, "self") && skill >= 0) {
						if (skill != rskill) {
							action_t a = { swap_right, skill };
							list_add(sequence, &a);
							rskill = skill;
						}

						action_t a = { cast_self, 0 };
						list_add(sequence, &a);
					}
				}
			}
		}

		tok = strtok(NULL, "/");
	}
}

int bsr(int x) {
	int y;

	asm("bsr %1, %0" :"=r" (y) :"m" (x));

	return y;
}

npc_t * npc_new(d2gs_packet_t *packet, npc_t *new) {
	// incomplete

	new->object.id = net_get_data(packet->data, 0, dword);
	new->index = net_get_data(packet->data, 4, word);
	new->object.location.x = net_get_data(packet->data, 6, word);
	new->object.location.y = net_get_data(packet->data, 8, word);
	new->hp = net_get_data(packet->data, 10, byte);
	new->mods = 0;

	int i = 12 * 8;

	// section 1: animation mode
	i += 4;

	// section 2: monster comp info
	if (net_extract_bits(packet->data, i, 1)) {
		i += 1;

		int j;
		for (j = 0; j < 16; j++) {
			int comp = monster_fields[new->index][j];
			//if (comp > 0) {
			if (comp > 2 && comp < 10) {
				i += (bsr(comp - 1) + 1);
			} else {
				i += 1;
			}
		}
	} else {
		i += 1;
	}

	// section 3: monster mod info
	if (net_extract_bits(packet->data, i, 1)) {
		i += 1;

		i += 2;

		new->super_unique = net_extract_bits(packet->data, i, 1);
		i += 1;

		i += 2;

		if (new->super_unique) {
			new->super_unique_index = net_extract_bits(packet->data, i, 16);
			i += 16;
		} else {
			new->super_unique_index = -1;
		}

		int j;
		for (j = 0; j < 10; j++) {
			byte mod = net_extract_bits(packet->data, i, 8);

			i += 8;

			if (!mod) break;

			new->mods |= (1 << mod);
		}
	} else {
		i += 1;

		new->super_unique = FALSE;
		new->super_unique_index = -1;
	}

	return new;
}

char * npc_default_lookup_name(dword index) {
	return index < n_monster_names ? (char *) monster_names[index] : "";
}

char * npc_superunique_lookup_name(dword index) {
	return index < n_super_uniques ? (char *) super_uniques[index] : "";
}

#define npc_lookup_name(npc) (((npc)->super_unique && (npc)->super_unique_index < n_super_uniques) ? npc_superunique_lookup_name((npc)->super_unique_index) : npc_default_lookup_name((npc)->index))

char * npc_dump(npc_t *npc, char *s_npc, size_t len) {
	snprintf(s_npc, len - 1, "%s (%08X) %i/%i", npc_lookup_name(npc), npc->object.id, npc->object.location.x, npc->object.location.y);

	return s_npc;
}

int npc_compare_id(npc_t *a, npc_t *b) {
	return a->object.id == b->object.id;
}

bool npc_compare_name(npc_t *n, char *s_n) {
	return string_compare(npc_lookup_name(n), s_n, FALSE);
}

void npc_refresh() {
	pthread_mutex_lock(&npcs_m);

	list_clear(&npcs);

	list_clone(&npcs_l, &npcs);

	pthread_mutex_unlock(&npcs_m);
}

bool npc_interact(npc_t *npc) {
	int s;

	pthread_mutex_lock(&npc_interact_mutex);
	pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &npc_interact_mutex);

	d2gs_send(0x59, "01 00 00 00 %d %d %d", npc->object.id, me.obj.location.x, me.obj.location.y);

	unsigned d = DISTANCE(me.obj.location, npc->object.location);
	int diff;

	for (diff = d * 120; diff > 0; diff -= 200) {
		d2gs_send(0x04, "01 00 00 00 %d", npc->object.id);

		msleep((diff < 200) ? diff : 200);
	}

	d2gs_send(0x13, "01 00 00 00 %d", npc->object.id);

	msleep(200);

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 4;

	if ((s = pthread_cond_timedwait(&npc_interact_cv, &npc_interact_mutex, &ts))) {
		plugin_error("chest", "failed to interact with %s\n", npc_lookup_name(npc));
	} else {
		d2gs_send(0x2f, "01 00 00 00 %d", npc->object.id);

		msleep(200);
	}

	pthread_cleanup_pop(1);

	return s ? FALSE : TRUE;
}

bool npc_shop(char *s_npc) {
	bool s;

	//pthread_mutex_lock(&npcs_m);
	//pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &npcs_m);

	npc_refresh();

	npc_t *npc = list_find(&npcs, (comparator_t) npc_compare_name, s_npc);
	if (npc) {
		if (npc_interact(npc)) {
			plugin_print("chest", "healing at %s\n", npc_lookup_name(npc));

			if ((module_setting("ShopAfterRuns")->i_var && !(runs % module_setting("ShopAfterRuns")->i_var)) || !scrolls) {
				plugin_print("chest", "shopping at %s\n", npc_lookup_name(npc));

				int j = scrolls;

				d2gs_send(0x38, "01 00 00 00 %d 00 00 00 00", npc->object.id);

				msleep(2000);

				int i;
				for (i = 0; i < 20 - j && scroll_id; i++) {
					d2gs_send(0x32, "%d %d 00 00 00 00 64 00 00 00", npc->object.id, scroll_id);

					msleep(200);
				}

				if (j < 20) plugin_print("chest", "restocked %i townportal(s)\n", 20 - j);
			}

			s =  TRUE;
		} else {
			plugin_print("chest", "failed to heal/shop at %s\n", npc_lookup_name(npc));

			s = FALSE;
		}

		d2gs_send(0x30, "01 00 00 00 %d", npc->object.id);
	} else {
		plugin_debug("chest", "couldn't find %s\n", s_npc);

		s = FALSE;
	}

	//pthread_cleanup_pop(1);

	return s;
}

bool npc_repair(char *s_npc) {
	bool s;

	npc_refresh();

	npc_t *npc = list_find(&npcs, (comparator_t) npc_compare_name, s_npc);
	if (npc) {
		if (npc_interact(npc)) {
			plugin_print("chest", "repairing equipment at %s\n", npc_lookup_name(npc));

			d2gs_send(0x35, "%d 00 00 00 00 00 00 00 00 00 00 00 80", npc->object.id);

			msleep(200);

			s = TRUE;
		} else {
			plugin_print("chest", "failed to repair at %s\n", npc_lookup_name(npc));

			s = FALSE;
		}

		d2gs_send(0x30, "01 00 00 00 %d", npc->object.id);
	} else {
		plugin_debug("chest", "couldn't find %s\n", s_npc);

		s = FALSE;
	}

	return s;
}

bool npc_merc(char *s_npc) {
	bool s;

	//pthread_mutex_lock(&npcs_m);
	//pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &npcs_m);

	npc_refresh();

	npc_t *npc = list_find(&npcs, (comparator_t) npc_compare_name, s_npc);
	if (npc) {
		if (npc_interact(npc)) {
			d2gs_send(0x38, "03 00 00 00 %d 00 00 00 00", npc->object.id);

			msleep(300);

			d2gs_send(0x62, "%d", npc->object.id);

			msleep(300);

			d2gs_send(0x38, "03 00 00 00 %d 00 00 00 00", npc->object.id);

			msleep(300);

			plugin_print("chest", "resurrected merc at %s\n", npc_lookup_name(npc));

			s = TRUE;;
		} else {
			plugin_print("chest", "failed to resurrect merc at %s\n", npc_lookup_name(npc));

			s = FALSE;
		}

		d2gs_send(0x30, "01 00 00 00 %d", npc->object.id);
	} else {
		plugin_debug("chest", "couldn't find %s\n", s_npc);

		s = FALSE;
	}

	//pthread_cleanup_pop(1);

	return s;
}

int d2gs_assign_npc(void *p) {
	d2gs_packet_t *packet = D2GS_CAST(p);

	npc_t new;
	npc_new(packet, &new);

	pthread_mutex_lock(&npcs_m);

	if (!list_find(&npcs_l, (comparator_t) npc_compare_id, &new)) {
		list_add(&npcs_l, &new);
	}

	pthread_mutex_unlock(&npcs_m);

	char s_new[512];
	npc_dump(&new, s_new, 512);
	plugin_debug("chest", "detected npc: %s\n", s_new);

	return FORWARD_PACKET;
}

int chest_compare_id(chest_t *a, chest_t *b) {
	return a->object.id == b->object.id;
}

void print_chest(chest_t *chest) {
	plugin_debug("chest", "chest at:%d/%d (d=%d), id:%u, preset_id:%d, opened:%d, locked:%d\n", \
				 chest->object.location.x, chest->object.location.y,	\
				 DISTANCE(me.obj.location, chest->object.location),		\
				 chest->object.id, chest->preset_id, chest->is_opened, chest->is_locked);
}

void print_chest_list() {		/* DEBUG */
	struct iterator it = list_iterator(&chests_l);
	chest_t *chest;

	plugin_debug("chest", "chests_l list:\n");
	while ((chest = iterator_next(&it))) {
		printf("DEBUG: %d: ", it.index);
		print_chest(chest);
	}
	printf("\n");
}

chest_t * chest_new(d2gs_packet_t *packet, chest_t *new) {
	new->is_opened = !(net_get_data(packet->data, 11, byte) == NEUTRAL);
	new->is_locked = !!(net_get_data(packet->data, 12, byte) == LOCKED);
	new->preset_id = net_get_data(packet->data, 5, word);
	new->object.id = net_get_data(packet->data, 1, dword);
	new->object.location.x = net_get_data(packet->data, 7, word);
	new->object.location.y = net_get_data(packet->data, 9, word);

	/* printf("ZBOUB:mode:%x, type:%x\n", net_get_data(packet->data, 11, byte), net_get_data(packet->data, 12, byte));			 /\* DEBUG *\/ */

	return new;
}

int d2gs_assign_chest(void *p) {
	d2gs_packet_t *packet = D2GS_CAST(p);

	chest_t new;
	chest_new(packet, &new);

	pthread_mutex_lock(&chests_m);

	if (!list_find(&chests_l, (comparator_t) chest_compare_id, &new)) {
		list_add(&chests_l, &new);
		/* plugin_debug("chest", "detected chest at %i/%i (%d)\n",			\ */
					 /* net_get_data(packet->data, 7, word), net_get_data(packet->data, 9, word), chest_preset_ids[i]); */
	}

	pthread_mutex_unlock(&chests_m);


	return FORWARD_PACKET;
}

int d2gs_update_npc(void *p) {
	d2gs_packet_t *packet = D2GS_CAST(p);

	pthread_mutex_lock(&npcs_m);

	npc_t n, *npc = NULL;
	switch(packet->id) {

		case 0x0c: {
			n.object.id = net_get_data(packet->data, 1, dword);

			npc = list_find(&npcs_l, (comparator_t) npc_compare_id, &n);
			if (npc) {
				npc->hp = net_get_data(packet->data, 7, byte);

				if (!npc->hp) {
					list_remove(&npcs_l, npc);
				}
			}

			npc = list_find(&npcs, (comparator_t) npc_compare_id, &n);
			if (npc) {
				npc->hp = net_get_data(packet->data, 7, byte);
			}
		}
		break;

		case 0x69: {
			n.object.id = net_get_data(packet->data, 0, dword);

			npc = list_find(&npcs_l, (comparator_t) npc_compare_id, &n);
			if (npc) {
				npc->object.location.x = net_get_data(packet->data, 5, word);
				npc->object.location.y = net_get_data(packet->data, 7, word);
				byte state = net_get_data(packet->data, 4, byte);
				if (state == 0x08 || state == 0x09) {
					npc->hp = 0;
				} else {
					npc->hp = net_get_data(packet->data, 9, byte);
				}

				if (!npc->hp) {
					list_remove(&npcs_l, npc);
				}
			}

			npc = list_find(&npcs, (comparator_t) npc_compare_id, &n);
			if (npc) {
				npc->object.location.x = net_get_data(packet->data, 5, word);
				npc->object.location.y = net_get_data(packet->data, 7, word);
				byte state = net_get_data(packet->data, 4, byte);
				if (state == 0x08 || state == 0x09) {
					npc->hp = 0;
				} else {
					npc->hp = net_get_data(packet->data, 9, byte);
				}
			}
		}
		break;

		case 0x6d: {
			n.object.id = net_get_data(packet->data, 0, dword);

			npc = list_find(&npcs_l, (comparator_t) npc_compare_id, &n);
			if (npc) {
				npc->object.location.x = net_get_data(packet->data, 4, word);
				npc->object.location.y = net_get_data(packet->data, 6, word);
				npc->hp = net_get_data(packet->data, 8, byte);

				if (!npc->hp) {
					list_remove(&npcs_l, npc);
				}
			}

			npc = list_find(&npcs, (comparator_t) npc_compare_id, &n);
			if (npc) {
				npc->object.location.x = net_get_data(packet->data, 4, word);
				npc->object.location.y = net_get_data(packet->data, 6, word);
				npc->hp = net_get_data(packet->data, 8, byte);
			}
		}
		break;

	}

	if (npc) {
		char s_npc[512];
		npc_dump(npc, s_npc, 512);
		plugin_debug("chest", "npc update: %s\n", s_npc);
	}

	pthread_mutex_unlock(&npcs_m);

	return FORWARD_PACKET;
}

int d2gs_item_action(void *p) {
	d2gs_packet_t *packet = D2GS_CAST(p);

	byte action = net_extract_bits(packet->data, 0, 8);
	dword id = net_extract_bits(packet->data, 24, 32);

	if (action == 0x0b) {
		char code[4] = { 0 };
		*(dword *)code = net_extract_bits(packet->data, 116, 24);

		if (!strcmp((char *) &code, "tsc")) {
			scroll_id = id;
		}
	}

	return FORWARD_PACKET;
}

int d2gs_char_location_update(void *p) {
	d2gs_packet_t *packet = D2GS_CAST(p);

	switch (packet->id) {

		case 0x15: { // received packet
			me_set_x(net_get_data(packet->data, 5, word)); //TODO: 2 byte alignment
			me_set_y(net_get_data(packet->data, 7, word)); //TODO: 2 byte alignment
		}
		break;

		case 0x95: { // received packet
			me_set_x(net_extract_bits(packet->data, 45, 15));
			me_set_y(net_extract_bits(packet->data, 61, 15));
		}
		break;

		case 0x01:
		case 0x03: { // sent packets
			me_set_x(net_get_data(packet->data, 0, word));
			me_set_y(net_get_data(packet->data, 2, word));
		}
		break;

		case 0x0c: { // sent packet
			if (cur_rskill == 0x36) {
				me_set_x(net_get_data(packet->data, 0, word));
				me_set_y(net_get_data(packet->data, 2, word));
			}
		}
		break;

	}

	/* plugin_debug("chest", "bot update (%02X): %i/%i\n", packet->id, me.obj.location.x, me.obj.location.y); */

	return FORWARD_PACKET;
}

int process_incoming_packet(void *p) {
	d2gs_packet_t *packet = (d2gs_packet_t *) p;

	switch (packet->id) {

	/* case 0x07: { */
	/* 	word x = net_get_data(packet->data, 0, word); */
	/* 	word y = net_get_data(packet->data, 2, word); */
	/* 	byte area = net_get_data(packet->data, 4, byte); */
	/* 	break; */
	/* } */

	case 0x0c: {
		d2gs_update_npc(packet);
		break;
	}

	case 0x11: {
		if (net_get_data(packet->data, 1, dword) == merc.id) {
			merc.id = 0;
		}
		break;
	}

	case 0x15: {
		d2gs_char_location_update(packet);
		break;
	}

	case 0x22: {
		if (net_get_data(packet->data, 6, word) == 0xdc) {
			scrolls = net_get_data(packet->data, 8, byte);
		}
		break;
	}

	case 0x26: {
		if (net_get_data(packet->data, 0, byte) == 0x05) break;

		char *player = (char *) (packet->data + 9);
		char *message = (char *) (packet->data + 9 + strlen(player) + 1);
		plugin_print("chest", "%s says: \"%s\"\n", player, message);
		break;
	}

	case 0x27: {
		pthread_mutex_lock(&npc_interact_mutex);
		pthread_cond_signal(&npc_interact_cv);
		pthread_mutex_unlock(&npc_interact_mutex);
		break;
	}

	case 0x51: {
		if (net_get_data(packet->data, 0, byte) == 0x02) {
				int i = 0;

				while (wp_preset_ids[i] && net_get_data(packet->data, 5, word) != wp_preset_ids[i])
					i++;
				if (wp_preset_ids[i]) {
					wp.id = net_get_data(packet->data, 1, dword);
					wp.location.x = net_get_data(packet->data, 7, word);
					wp.location.y = net_get_data(packet->data, 9, word);

					plugin_debug("chest", "detected waypoint at %i/%i\n", wp.location.x, wp.location.y);
				} else if (net_get_data(packet->data, 11, byte) == NEUTRAL) {
					i = 0;

					while (chest_preset_ids[i] && net_get_data(packet->data, 5, word) != chest_preset_ids[i])
						i++;
					if (chest_preset_ids[i]) {
						d2gs_assign_chest(packet);
					}
			}
			/* else { */
			/* 	plugin_debug("chest", "chest was not neutral: mode:%d, type:%d\n", net_get_data(packet->data, 11, byte), net_get_data(packet->data, 12, byte)); /\* DEBUG *\/ */
			/* } */
		}
		break;
	}

	case 0x5a: { // auto-accept party
		if (net_get_data(packet->data, 0, byte) == 0x07) {
			if (net_get_data(packet->data, 1, byte) == 0x02) {
				dword id = net_get_data(packet->data, 2, dword);

				if (net_get_data(packet->data, 6, byte) == 0x05) {
					d2gs_send(0x5e, "08 %d", id);
				}
			}
		}
		break;
	}

	case 0x69: {
		d2gs_update_npc(packet);
		break;
	}

	case 0x6d: {
		d2gs_update_npc(packet);
		break;
	}

	case 0x81: {
		if (net_get_data(packet->data, 3, dword) == me.obj.id) { //TODO: 4 byte alignment
			merc.id = net_get_data(packet->data, 7, dword); //TODO: 4 byte alignment
		}
		break;
	}

	case 0x82: {
		if (net_get_data(packet->data, 0, dword) == me.obj.id && !tp.id) {
			tp.id = net_get_data(packet->data, 20, dword);


			pthread_mutex_lock(&townportal_interact_mutex);
			pthread_cond_signal(&townportal_interact_cv);
			pthread_mutex_unlock(&townportal_interact_mutex);
		}
		break;
	}

	case 0x95: {
		d2gs_char_location_update(packet);
		break;
	}

	case 0x97: {
		pthread_mutex_lock(&switch_slot_m);
		pthread_cond_signal(&switch_slot_cv);
		pthread_mutex_unlock(&switch_slot_m);
		break;
	}

	case 0x9c: {
		d2gs_item_action(packet);
		break;
	}

	case 0xac: {
		d2gs_assign_npc(packet);
		break;
	}

	}

	return FORWARD_PACKET;
} //TODO

bool _moveto(int x, int y) {
	point_t p = {x, y};
	//pthread_mutex_lock(&game_exit_m);
	//pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &game_exit_m);

	/*if (exited) {
		goto end;
	}*/
	int d = DISTANCE(me.obj.location, p);

	int delay = 2000; //ugly hack to wait me.object.location update
	while (d > MAX_WALKING_DISTANCE && delay) {
		msleep(100);
		delay -= 100;
		d = DISTANCE(me.obj.location, p);
	}

	if (d > MAX_WALKING_DISTANCE) {
		plugin_print("chest", "got stuck trying to move to %i/%i (%i)\n", x, y, d);

		return FALSE;
	}


	/*
	 * sleeping can lead to a problem:
	 * bot's at 10000/10000, opens portal, chickens, moves to 5000/5000 -> long sleep
	 * C -> S 0x03 | C-> S 0x69 | msleep()
	 */


	//int t_s = t / 1000;
	//int t_ns = (t - (t_s * 1000)) * 1000 * 1000;

	//struct timespec ts;
	//clock_gettime(CLOCK_REALTIME, &ts);
	//ts.tv_sec += t_s;
	//ts.tv_nsec += t_ns;

	int t;
	int retry = 0;
	do {
		plugin_print("chest", "moving to %i/%i (retry:%d)\n", (word) x, (word) y, retry);
		d2gs_send(0x03, "%w %w", (word) x, (word) y);

		retry++;

		t = d * 50;
		/* plugin_debug("chest", "sleeping for %ims\n", t); */

		msleep(t > 2000 ? 2000 : t);

		d2gs_send(0x4b, "00 00 00 00 %d", me.obj.id);

		delay = 2000; //ugly hack to wait me.object.location update
		do {
			msleep(100);
			delay -= 100;
			d = DISTANCE(me.obj.location, p);
		} while (d > 10 && delay);
	} while (retry < 3 && d > 10);

	//pthread_cond_timedwait(&game_exit_cv, &game_exit_m, &ts);


	//end:
	//pthread_cleanup_pop(1);

	return d <= 10;
}

bool teleport(int x, int y) {
	plugin_print("chest", "teleporting to %i/%i\n", x, y);

	swap_right(0x36);

	d2gs_send(0x0c, "%w %w", x, y);

	msleep(module_setting("CastDelay")->i_var);

	return TRUE;
}

bool split_path(int x, int y, int min_range, int max_range, bool (*move_fun)(int, int)) {
	point_t p = {x, y};
	int d = DISTANCE(me.obj.location, p);

	if (d < min_range) {
		return TRUE;
	}

	if (d > max_range) {
		int n_nodes = d / max_range + 1;

		int inc_x = (x - me.obj.location.x) / n_nodes;
		int inc_y = (y - me.obj.location.y) / n_nodes;

		int target_x = me.obj.location.x;
		int target_y = me.obj.location.y;

		int i;
		for (i = 0; i <= n_nodes - 1; i++) {
			target_x += inc_x;
			target_y += inc_y;
			if (!move_fun(target_x, target_y)) {
				return FALSE;
			}
		}

		d = DISTANCE(me.obj.location, p);
	}

	if (d >= min_range && d <= max_range && !move_fun(x, y)) {
		return FALSE;
	}

	return split_path(x, y, min_range, max_range, move_fun);
}

bool teleport_straight(int x, int y) {
	plugin_debug("chest", "teleporting straight to %i/%i\n", x, y);

	return split_path(x, y, MIN_TELEPORT_DISTANCE, MAX_TELEPORT_DISTANCE, teleport);
}

bool _walk_straight(int x, int y) {
	plugin_debug("chest", "walking straight to %i/%i\n", x, y);

	return split_path(x, y, MIN_WALKING_DISTANCE, MAX_WALKING_DISTANCE / 2, _moveto);
}

bool _waypoint(dword area) {
	if (!wp.id)
		return FALSE;

	d2gs_send(0x13, "02 00 00 00 %d", wp.id);

	msleep(300);

	d2gs_send(0x49, "%d %d", wp.id, area);

	if (merc.id) {
		msleep(300);

		d2gs_send(0x4b, "01 00 00 00 %d", merc.id);
	}

	if (!IN_TOWN(area)) {
		me_set_intown(FALSE);
	}

	msleep(500);

	plugin_print("chest", "took waypoint %02X\n", area);

	return TRUE;
}

bool move_to_wp() {
	e_town_move previous_dest = g_previous_dest;
	g_previous_dest = WP;

	if (me.act == 1) {
		plugin_debug("chest", "change_act to WP1 (act:%d bot:%d,%d to:%d/%d)\n", \
					 me.act, me.obj.location.x, me.obj.location.y, wp.location.x, wp.location.y);
		sleep(2);
		walk_straight(wp.location.x, wp.location.y);
	} else if (me.act == 2) {
		plugin_debug("chest", "change_act to WP2 (act:%d bot:%d,%d to:%d/%d)\n", \
					 me.act, me.obj.location.x, me.obj.location.y, wp.location.x, wp.location.y);
		if (previous_dest == GREIZ) {
			walk_straight(GREIZ_NODE_X, GREIZ_NODE_Y);
		} else if (previous_dest) {
			walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
		} else {
			walk_straight(ACT2_SPAWN_NODE1_X, ACT2_SPAWN_NODE1_Y);
			walk_straight(ACT2_SPAWN_NODE2_X, ACT2_SPAWN_NODE2_Y);
			walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
		}
		walk_straight(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
	} else if (me.act == 3) {
		plugin_debug("chest", "change_act to WP3 (act:%d bot:%d,%d to:%d/%d)\n", \
					 me.act, me.obj.location.x, me.obj.location.y, wp.location.x, wp.location.y);
		if (previous_dest) {
			walk_straight(ORMUS_NODE_X, ORMUS_NODE_Y);
		}
		walk_straight(ACT3_WP_SPOT_X, ACT3_WP_SPOT_Y);
	}

	return TRUE;
}

bool change_act(int dst_act) {
	if (!move_to_wp())
		return FALSE;

	if (dst_act == 2) {
		waypoint(WP_LUTGHOLEIN);
	} else if (dst_act == 3) {
		waypoint(WP_KURASTDOCKS);
	}

	msleep(500);
	/* me_set_act(dst_act); */

	return TRUE;
}

bool _town_move(e_town_move where) {
	e_town_move previous_dest = g_previous_dest;
	g_previous_dest = where;

	switch (where) {
		case WP:
			move_to_wp();
			break;

		case TP:
			if (me.act != 3 && !change_act(3)) {
				return FALSE;
			}
			plugin_debug("chest", "town_move to TP (act:%d bot:%d,%d to:%d/%d)\n", \
						 me.act, me.obj.location.x, me.obj.location.y, wp.location.x, wp.location.y);
			if (!previous_dest) {
				walk_straight(ACT3_SPAWN_NODE_X, ACT3_SPAWN_NODE_Y);
			} else if (previous_dest == ORMUS) {
				walk_straight(ORMUS_NODE_X, ORMUS_NODE_Y);
			}
			walk_straight(ACT3_TP_SPOT_X, ACT3_TP_SPOT_Y);
			break;

		case ORMUS:
			if (me.act != 3 && !change_act(3)) {
				return FALSE;
			}
			plugin_debug("chest", "town_move to ORMUS (act:%d bot:%d,%d to:%d/%d)\n", \
						 me.act, me.obj.location.x, me.obj.location.y, ORMUS_SPOT_Y, ORMUS_SPOT_X);
			if (!previous_dest) {
				walk_straight(ACT3_SPAWN_NODE_X, ACT3_SPAWN_NODE_Y);
			} else {
				walk_straight(ORMUS_NODE_X, ORMUS_NODE_Y);
			}
			walk_straight(ORMUS_SPOT_X, ORMUS_SPOT_Y);
			break;

		case FARA:
			if (me.act != 2 && !change_act(2)) {
				return FALSE;
			}
			plugin_debug("chest", "town_move to FARA (act:%d bot:%d,%d to:%d/%d)\n", \
						 me.act, me.obj.location.x, me.obj.location.y, FARA_SPOT_X, FARA_SPOT_Y);
			if (!previous_dest) {
				walk_straight(ACT2_SPAWN_NODE1_X, ACT2_SPAWN_NODE1_Y);
				walk_straight(ACT2_SPAWN_NODE2_X, ACT2_SPAWN_NODE2_Y);
				walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			} else if (previous_dest == WP) {
				walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			} else if (previous_dest == GREIZ) {
				walk_straight(GREIZ_NODE_X, GREIZ_NODE_Y);
				walk_straight(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
				walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			}
			walk_straight(FARA_SPOT_X, FARA_SPOT_Y);
			break;

		case LYSANDER:
			if (me.act != 2 && !change_act(2)) {
				return FALSE;
			}
			plugin_debug("chest", "town_move to LYSANDER (act:%d bot:%d,%d to:%d/%d)\n", \
						 me.act, me.obj.location.x, me.obj.location.y, LYSANDER_SPOT_X, LYSANDER_SPOT_Y);
			if (!previous_dest) {
				walk_straight(ACT2_SPAWN_NODE1_X, ACT2_SPAWN_NODE1_Y);
				walk_straight(ACT2_SPAWN_NODE2_X, ACT2_SPAWN_NODE2_Y);
				walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			} else if (previous_dest == WP) {
				walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			} else if (previous_dest == GREIZ) {
				walk_straight(GREIZ_NODE_X, GREIZ_NODE_Y);
				walk_straight(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
				walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			}
			walk_straight(LYSANDER_SPOT_X, LYSANDER_SPOT_Y);
			break;

		case GREIZ:
			if (me.act != 2 && !change_act(2)) {
				return FALSE;
			}
			plugin_debug("chest", "town_move to GREIZ (act:%d bot:%d,%d to:%d/%d)\n", \
						 me.act, me.obj.location.x, me.obj.location.y, GREIZ_SPOT_X, GREIZ_SPOT_Y);
			if (!previous_dest) {
				walk_straight(ACT2_SPAWN_NODE1_X, ACT2_SPAWN_NODE1_Y);
				walk_straight(ACT2_SPAWN_NODE2_X, ACT2_SPAWN_NODE2_Y);
				walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
				walk_straight(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
			} else if (previous_dest == FARA || previous_dest == LYSANDER) {
				walk_straight(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
				walk_straight(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
			}
			walk_straight(GREIZ_NODE_X, GREIZ_NODE_Y);
			walk_straight(GREIZ_SPOT_X, GREIZ_SPOT_Y);
			break;

		default:
			plugin_error("chest", "town_move: destination '%d' unknown\n", where);
			return FALSE;
	}

	return TRUE;
}

void precast() {
	plugin_print("chest", "casting buff sequence...\n");

	struct iterator it = list_iterator(&precast_sequence);
	action_t *a;

	while ((a = iterator_next(&it))) {
		a->function(a->arg);
	}
}

bool _townportal() {
	int s;

	if (me.intown) {
		plugin_error("chest", "can't tp from town\n");

		return FALSE;
	}

	d2gs_send(0x3c, "%d ff ff ff ff", 0xdc); //select skill tp... TODO: use book

	msleep(400);

	pthread_mutex_lock(&townportal_interact_mutex);
	pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &townportal_interact_mutex);

	d2gs_send(0x0c, "%w %w", me.obj.location.x, me.obj.location.y);

	msleep(500);

	plugin_print("chest", "opened townportal\n");

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 2;

	if ((s = pthread_cond_timedwait(&townportal_interact_cv, &townportal_interact_mutex, &ts))) {
		plugin_error("chest", "failed to take townportal\n");
	}
	else {
		d2gs_send(0x13, "02 00 00 00 %d", tp.id);

		if (merc.id) {
			msleep(300);

			d2gs_send(0x4b, "01 00 00 00 %d", merc.id);
		}

		plugin_print("chest", "took townportal\n");

		me_set_intown(TRUE);


		msleep(400);

		/* if (me.act == 3) { //eheh :p */
		/* 	me_set_x(5153); */
		/* 	me_set_y(5067); */
		/* } */
        d2gs_send(0x4b, "00 00 00 00 %d", me.obj.id);
		msleep(200);
		tp.id = 0;
	}

	pthread_cleanup_pop(1);

	return s ? FALSE : TRUE;
}

void leave() {
	d2gs_send(0x69, "");

	plugin_print("chest", "leaving game\n");
}

void open_chest(chest_t *chest) {
	plugin_debug("chest", "open chest at %i/%i (%u)\n", chest->object.location.x, chest->object.location.y, chest->object.id);

	/* d2gs_send(0x4b, "00 00 00 00 %d", me.obj.id); */
	/* msleep(200); */

	d2gs_send(0x13, "02 00 00 00 %d", chest->object.id);
	//TODO: handle keys / locked chests

	msleep(400); //TODO: increase delay if "long" chest? and see if this is enough for the normal ones

	// pickit
	//internal_send(0x9c, "%s 00", "pickit");
	execute_module_schedule(MODULE_D2GS);
}

chest_t * get_closest_chest() {
	struct iterator it;
	chest_t *current_chest;
	chest_t *closest_chest;
	int min_dist;
	int tmp_dist;

	closest_chest = NULL;
	min_dist = 0xffff;
	it = list_iterator(&chests_l);
	while ((current_chest = iterator_next(&it))) {
		if (!current_chest->is_opened) {
			tmp_dist = DISTANCE(me.obj.location, current_chest->object.location);
			if (tmp_dist < min_dist && tmp_dist < 1000) {
				min_dist = tmp_dist;
				closest_chest = current_chest;
			}
		}
	}

	return closest_chest;
}

void open_all_chests() {
	chest_t *closest_chest;
	chest_t chest;

	plugin_debug("chest", "open all chests around\n");

	chest.is_opened = TRUE;
	while (TRUE) {
		pthread_mutex_lock(&chests_m);

		/* print_chest_list();		/\* DEBUG *\/ */
		if ((closest_chest = get_closest_chest())) {
			memcpy(&chest, closest_chest, sizeof(chest_t));
			list_remove(&chests_l, closest_chest);
		}

		pthread_mutex_unlock(&chests_m);

		if (chest.is_opened) {
			break ;
		}

		print_chest(&chest); /* DEBUG */

		teleport_straight(chest.object.location.x, chest.object.location.y);
		open_chest(&chest);
		chest.is_opened = TRUE;
	}
}

bool do_town_chores() {
	plugin_print("chest", "running town chores...\n");

	town_move(ORMUS);
	npc_shop("Ormus");

	// move to lysander and buy keys
	/* if (module_setting("RepairAfterRuns")->i_var && !(runs % module_setting("RepairAfterRuns")->i_var)) { */
	/* 	town_move(LYSANDER); */
	/* 	npc_key("Lysander"); */
	/* } */

	// move to fara and repair equipment
	if (module_setting("RepairAfterRuns")->i_var && !(runs % module_setting("RepairAfterRuns")->i_var)) {
		town_move(FARA);
		npc_repair("Fara");
	}

	// resurrect merc
	if (module_setting("UseMerc")->b_var && !merc.id) {
		merc_rez++;

		town_move(GREIZ);
		npc_merc("Greiz");
	}

	return TRUE;
}

bool main_script() {
	if (me.act < 1 || me.act > 3) {
		plugin_error("chest", "can't start from act %i\n", me.act);
		return FALSE;
	}

	do_town_chores();

	plugin_print("chest", "running lower kurast...\n");

	town_move(WP);
	waypoint(WP_LOWERKURAST);
	precast();
	open_all_chests();
	townportal();

	town_move(WP);
	waypoint(WP_KURASTBAZAAR);
	precast();
	open_all_chests();
	townportal();

	town_move(WP);
	waypoint(WP_UPPERKURAST);
	precast();
	open_all_chests();
	townportal();

	town_move(WP);
	waypoint(WP_UPPERKURAST);
	precast();
	extension("find_level_exit")->call("chest", "Upper Kurast", "Kurast to Sewer");
	precast();
	open_all_chests();

	extension("find_level_exit")->call("chest", "Sewers Level 1", "Sewer Down");
	precast();
	open_all_chests();

	return TRUE;
}

_export void * module_thread(void *arg) {
	(void)arg;

	time(&run_start);
	plugin_print("chest", "starting chest run %i\n", ++runs);

	sleep(3);

	if (!main_script())
		plugin_print("chest", "error: something went wrong :/\n");

	if (!me.intown) {
		townportal();
	}

	if (me.intown) {
		time_t cur;
		int wait_delay = module_setting("MinGameTime")->i_var - (int) difftime(time(&cur), run_start);
		while (wait_delay > 0) {
			plugin_print("chest", "sleeping for %d seconds...\n", wait_delay);
			/* townportal(); */
			sleep(5);
			wait_delay -= 5;
		}
	}

	// leave game
	leave();

	pthread_exit(NULL);
}

_export void module_cleanup() {
	object_clear(merc);
	object_clear(tp);
	object_clear(wp);
	scroll_id = 0;
	list_clear(&npcs);
	list_clear(&npcs_l);
	list_clear(&chests_l);

	if (!runs) return;

	time_t cur;
	int duration = (int) difftime(time(&cur), run_start);
	runtime += duration;

	char * s_duration, * s_average;
	s_duration = string_format_time(duration);
	s_average = string_format_time(runs > 0 ? runtime / runs : 0);

	plugin_print("chest", "run took %s (average: %s)\n", s_duration, s_average);

	free(s_duration);
	free(s_average);
}

_export const char * module_get_title() {
	return "chest";
}

_export const char * module_get_version() {
	return "0.1.0";
}

_export const char * module_get_author() {
	return "JeanMax";
}

_export const char * module_get_description() {
	return "open chests in act3 :D";
}

_export int module_get_license() {
	return LICENSE_GPL_V3;
}

_export module_type_t module_get_type() {
	return (module_type_t) { MODULE_D2GS, MODULE_ACTIVE };
}

_export bool module_load_config(struct setting_section *s) {
	int i;
	for (i = 0; i < s->entries; i++) {
		if (!strcmp(s->settings[i].name, "PrecastSequence")) {
			char *c_setting = strdup(s->settings[i].s_var);
			assemble_sequence(&precast_sequence, c_setting);
			free(c_setting);
		} else {
			struct setting *set = module_setting(s->settings[i].name);
			if (set) {
				if (s->settings[i].type == INTEGER && set->type == INTEGER) {
					set->i_var = s->settings[i].i_var;
				} else if (s->settings[i].type == STRING) {
					if (set->type == BOOLEAN) {
						set->b_var = !strcmp(string_to_lower_case(s->settings[i].s_var), "yes");
					} else if (set->type == STRING){
						set->s_var = strdup(s->settings[i].s_var);
					} else if (set->type == INTEGER) {
						sscanf(s->settings[i].s_var, "%li", &set->i_var);
					}
				}
			}
		}
	}

	return TRUE;
}

_export bool module_init() {
	/* register_packet_handler(D2GS_RECEIVED, 0x03, process_incoming_packet); */
	register_packet_handler(D2GS_RECEIVED, 0x11, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x51, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0xac, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x97, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x9c, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x27, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x81, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x82, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x22, process_incoming_packet);
	if (setting("Debug")->b_var)
		register_packet_handler(D2GS_RECEIVED, 0x5a, process_incoming_packet); // auto-accept party
	register_packet_handler(D2GS_RECEIVED, 0x15, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x95, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x0c, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x69, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x6d, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x26, process_incoming_packet);

	register_packet_handler(D2GS_SENT, 0x01, d2gs_char_location_update);
	register_packet_handler(D2GS_SENT, 0x03, d2gs_char_location_update);
	register_packet_handler(D2GS_SENT, 0x0c, d2gs_char_location_update);

	pthread_mutex_init(&npcs_m, NULL);
	pthread_mutex_init(&chests_m, NULL);

	pthread_cond_init(&npc_interact_cv, NULL);
	pthread_mutex_init(&npc_interact_mutex, NULL);

	pthread_cond_init(&townportal_interact_cv, NULL);
	pthread_mutex_init(&townportal_interact_mutex, NULL);

	pthread_mutex_init(&switch_slot_m, NULL);
	pthread_cond_init(&switch_slot_cv, NULL);

	npcs = list_new(npc_t);
	npcs_l = list_new(npc_t);
	chests_l = list_new(chest_t);

	cur_rskill = -1;
	cur_lskill = -1;

	return TRUE;
}

_export bool module_finit() {
	/* unregister_packet_handler(D2GS_RECEIVED, 0x03, process_incoming_packet); */
	unregister_packet_handler(D2GS_RECEIVED, 0x11, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x51, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0xac, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x97, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x9c, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x27, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x81, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x82, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x22, process_incoming_packet);
	if (setting("Debug")->b_var)
		unregister_packet_handler(D2GS_RECEIVED, 0x5a, process_incoming_packet); // auto-accept party
	unregister_packet_handler(D2GS_RECEIVED, 0x15, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x95, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x0c, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x69, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x6d, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x26, process_incoming_packet);

	unregister_packet_handler(D2GS_SENT, 0x01, d2gs_char_location_update);
	unregister_packet_handler(D2GS_SENT, 0x03, d2gs_char_location_update);
	unregister_packet_handler(D2GS_SENT, 0x0c, d2gs_char_location_update);

	pthread_mutex_destroy(&npcs_m);
	pthread_mutex_destroy(&chests_m);

	pthread_cond_destroy(&npc_interact_cv);
	pthread_mutex_destroy(&npc_interact_mutex);

	pthread_cond_destroy(&townportal_interact_cv);
	pthread_mutex_destroy(&townportal_interact_mutex);

	pthread_mutex_destroy(&switch_slot_m);
	pthread_cond_destroy(&switch_slot_cv);

	list_clear(&precast_sequence);

	char *s_average, *s_runtime;
	s_average = string_format_time(runs > 0 ? runtime / runs : 0);
	s_runtime = string_format_time(runtime);

	ui_add_statistics_plugin("chest", "runs: %i\n", runs);
	ui_add_statistics_plugin("chest", "average run duration: %s\n", s_average);
	ui_add_statistics_plugin("chest", "total time spent running: %s\n", s_runtime);
	ui_add_statistics_plugin("chest", "merc resurrections: %i\n", merc_rez);

	free(s_average);
	free(s_runtime);

	return TRUE;
}
