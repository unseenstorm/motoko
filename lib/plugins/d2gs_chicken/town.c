/*
 * copyright (c) 2016 JeanMax
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

#include "town.h"

/*
static const int waypoints[][] = {
	{0x01, 0x03, 0x04, 0x05, 0x06, 0x1b, 0x1d, 0x20, 0x23},
	{0x28, 0x30, 0x2a, 0x39, 0x2b, 0x2c, 0x34, 0x4a, 0x2e},
	{0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x53, 0x65},
	{0x67, 0x6a, 0x6b},
	{0x6d, 0x6f, 0x70, 0x71, 0x73, 0x7b, 0x75, 0x76, 0x81}
};
*/

static const word wp_preset_ids[] = {
	119, 145, 156, 157, 237, 238, 288, 323, 324, 398, 402, 429, 494, 496, 511, 539, 0
};

static object_t merc;

static object_t tp;

static object_t wp;

static int scrolls;

static dword scroll_id;

static pthread_mutex_t npcs_m;

static pthread_cond_t npc_interact_cv;

static pthread_mutex_t npc_interact_mutex;

static pthread_cond_t townportal_interact_cv;

static pthread_mutex_t townportal_interact_mutex;

static pthread_cond_t switch_slot_cv;

static pthread_mutex_t switch_slot_m;


static int bsr(int x) {
	int y;

	asm("bsr %1, %0" :"=r" (y) :"m" (x));

	return y;
}

static npc_t * npc_new(d2gs_packet_t *packet, npc_t *new) {
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

static char * npc_default_lookup_name(dword index) {
	return index < n_monster_names ? (char *) monster_names[index] : "";
}

static char * npc_superunique_lookup_name(dword index) {
	return index < n_super_uniques ? (char *) super_uniques[index] : "";
}

static char * npc_dump(npc_t *npc, char *s_npc, size_t len) {
	snprintf(s_npc, len - 1, "%s (%08X) %i/%i", npc_lookup_name(npc), npc->object.id, npc->object.location.x, npc->object.location.y);

	return s_npc;
}

static int npc_compare_id(npc_t *a, npc_t *b) {
	return a->object.id == b->object.id;
}

static bool npc_compare_name(npc_t *n, char *s_n) {
	return string_compare(npc_lookup_name(n), s_n, FALSE);
}

static void npc_refresh() {
	pthread_mutex_lock(&npcs_m);

	list_clear(&npcs);

	list_clone(&npcs_l, &npcs);

	pthread_mutex_unlock(&npcs_m);
}

static bool npc_interact(npc_t *npc) {
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
		plugin_error("town", "failed to interact with %s\n", npc_lookup_name(npc));
	} else {
		d2gs_send(0x2f, "01 00 00 00 %d", npc->object.id);

		msleep(200);
	}

	pthread_cleanup_pop(1);

	return s ? FALSE : TRUE;
}

static bool npc_shop(char *s_npc) {
	bool s;

	//pthread_mutex_lock(&npcs_m);
	//pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &npcs_m);

	npc_refresh();

	npc_t *npc = list_find(&npcs, (comparator_t) npc_compare_name, s_npc);
	if (npc) {
		if (npc_interact(npc)) {
			plugin_print("town", "healing at %s\n", npc_lookup_name(npc));

			/* if (!scrolls) { */
				plugin_print("town", "shopping at %s\n", npc_lookup_name(npc));

				int j = scrolls;

				d2gs_send(0x38, "01 00 00 00 %d 00 00 00 00", npc->object.id);

				msleep(2000);

				int i;
				for (i = 0; i < 20 - j && scroll_id; i++) {
					d2gs_send(0x32, "%d %d 00 00 00 00 64 00 00 00", npc->object.id, scroll_id);

					msleep(200);
				}

				if (j < 20) plugin_print("town", "restocked %i townportal(s)\n", 20 - j);
			/* } */

			s =  TRUE;
		} else {
			plugin_print("town", "failed to heal/shop at %s\n", npc_lookup_name(npc));

			s = FALSE;
		}

		d2gs_send(0x30, "01 00 00 00 %d", npc->object.id);
	} else {
		plugin_debug("town", "couldn't find %s\n", s_npc);

		s = FALSE;
	}

	//pthread_cleanup_pop(1);

	return s;
}

static bool npc_repair(char *s_npc) {
	bool s;

	npc_refresh();

	npc_t *npc = list_find(&npcs, (comparator_t) npc_compare_name, s_npc);
	if (npc) {
		if (npc_interact(npc)) {
			plugin_print("town", "repairing equipment at %s\n", npc_lookup_name(npc));

			d2gs_send(0x35, "%d 00 00 00 00 00 00 00 00 00 00 00 80", npc->object.id);

			msleep(200);

			s = TRUE;
		} else {
			plugin_print("town", "failed to repair at %s\n", npc_lookup_name(npc));

			s = FALSE;
		}

		d2gs_send(0x30, "01 00 00 00 %d", npc->object.id);
	} else {
		plugin_debug("town", "couldn't find %s\n", s_npc);

		s = FALSE;
	}

	return s;
}

static bool npc_merc(char *s_npc) {
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

			plugin_print("town", "resurrected merc at %s\n", npc_lookup_name(npc));

			s = TRUE;;
		} else {
			plugin_print("town", "failed to resurrect merc at %s\n", npc_lookup_name(npc));

			s = FALSE;
		}

		d2gs_send(0x30, "01 00 00 00 %d", npc->object.id);
	} else {
		plugin_debug("town", "couldn't find %s\n", s_npc);

		s = FALSE;
	}

	//pthread_cleanup_pop(1);

	return s;
}


bool waypoint(dword area) {
	//see src/data/waypoints.h for ids
	if (!wp.id)
		return FALSE;

	d2gs_send(0x13, "02 00 00 00 %d", wp.id);

	msleep(300);

	d2gs_send(0x49, "%d %d", wp.id, area);

	if (merc.id) {
		msleep(300);

		d2gs_send(0x4b, "01 00 00 00 %d", merc.id);
	}

	if (IN_TOWN(area)) {
		g_previous_dest = WP;
	} else {
		me_set_intown(FALSE);
	}

	msleep(500);

	plugin_print("town", "took waypoint %02X\n", area);

	return TRUE;
}

//TODO: seriously? just find something better ><
bool town_move(e_town_move where) {
	switch (where) {
		case WP:
			if (me.act == 1) {
				plugin_debug("town", "town_move from %d to WP1 (act:%d bot:%d,%d to:%d/%d)\n", \
							 g_previous_dest, me.act, me.obj.location.x, me.obj.location.y, wp.location.x, wp.location.y);
				sleep(2);
				walk(wp.location.x, wp.location.y);
			} else if (me.act == 2) {
				plugin_debug("town", "town_move from %d to WP2 (act:%d bot:%d,%d to:%d/%d)\n", \
							 g_previous_dest, me.act, me.obj.location.x, me.obj.location.y, wp.location.x, wp.location.y);
				if (g_previous_dest == GREIZ) {
					walk(GREIZ_NODE_X, GREIZ_NODE_Y);
				} else if (g_previous_dest) {
					walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
				} else {
					walk(ACT2_SPAWN_NODE1_X, ACT2_SPAWN_NODE1_Y);
					walk(ACT2_SPAWN_NODE2_X, ACT2_SPAWN_NODE2_Y);
					walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
				}
				walk(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
			} else if (me.act == 3) {
				plugin_debug("town", "town_move from %d to WP3 (act:%d bot:%d,%d to:%d/%d)\n", \
							 g_previous_dest, me.act, me.obj.location.x, me.obj.location.y, wp.location.x, wp.location.y);
				if (g_previous_dest == ORMUS) {
					walk(ORMUS_NODE_X, ORMUS_NODE_Y);
				}
				walk(ACT3_WP_SPOT_X, ACT3_WP_SPOT_Y);
			}
			break;

		case TP:
			if (me.act != 3 && !change_act(3)) {
				return FALSE;
			}
			plugin_debug("town", "town_move from %d to TP (act:%d bot:%d,%d to:%d/%d)\n", \
						 g_previous_dest, me.act, me.obj.location.x, me.obj.location.y, wp.location.x, wp.location.y);
			if (!g_previous_dest) {
				walk(ACT3_SPAWN_NODE_X, ACT3_SPAWN_NODE_Y);
			} else if (g_previous_dest == ORMUS) {
				walk(ORMUS_NODE_X, ORMUS_NODE_Y);
			}
			walk(ACT3_TP_SPOT_X, ACT3_TP_SPOT_Y);
			break;

		case ORMUS:
			if (me.act != 3 && !change_act(3)) {
				return FALSE;
			}
			plugin_debug("town", "town_move from %d to ORMUS (act:%d bot:%d,%d to:%d/%d)\n", \
						 g_previous_dest, me.act, me.obj.location.x, me.obj.location.y, ORMUS_SPOT_Y, ORMUS_SPOT_X);
			if (!g_previous_dest) {
				walk(ACT3_SPAWN_NODE_X, ACT3_SPAWN_NODE_Y);
			} else {
				walk(ORMUS_NODE_X, ORMUS_NODE_Y);
			}
			walk(ORMUS_SPOT_X, ORMUS_SPOT_Y);
			break;

		case FARA:
			if (me.act != 2 && !change_act(2)) {
				return FALSE;
			}
			plugin_debug("town", "town_move from %d to FARA (act:%d bot:%d,%d to:%d/%d)\n", \
						 g_previous_dest, me.act, me.obj.location.x, me.obj.location.y, FARA_SPOT_X, FARA_SPOT_Y);
			if (!g_previous_dest) {
				walk(ACT2_SPAWN_NODE1_X, ACT2_SPAWN_NODE1_Y);
				walk(ACT2_SPAWN_NODE2_X, ACT2_SPAWN_NODE2_Y);
				walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			} else if (g_previous_dest == WP) {
				walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			} else if (g_previous_dest == GREIZ) {
				walk(GREIZ_NODE_X, GREIZ_NODE_Y);
				walk(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
				walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			}
			walk(FARA_SPOT_X, FARA_SPOT_Y);
			break;

		case LYSANDER:
			if (me.act != 2 && !change_act(2)) {
				return FALSE;
			}
			plugin_debug("town", "town_move from %d to LYSANDER (act:%d bot:%d,%d to:%d/%d)\n", \
						 g_previous_dest, me.act, me.obj.location.x, me.obj.location.y, LYSANDER_SPOT_X, LYSANDER_SPOT_Y);
			if (!g_previous_dest) {
				walk(ACT2_SPAWN_NODE1_X, ACT2_SPAWN_NODE1_Y);
				walk(ACT2_SPAWN_NODE2_X, ACT2_SPAWN_NODE2_Y);
				walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			} else if (g_previous_dest == WP) {
				walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			} else if (g_previous_dest == GREIZ) {
				walk(GREIZ_NODE_X, GREIZ_NODE_Y);
				walk(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
				walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
			}
			walk(LYSANDER_SPOT_X, LYSANDER_SPOT_Y);
			break;

		case GREIZ:
			if (me.act != 2 && !change_act(2)) {
				return FALSE;
			}
			plugin_debug("town", "town_move from %d to GREIZ (act:%d bot:%d,%d to:%d/%d)\n", \
						 g_previous_dest, me.act, me.obj.location.x, me.obj.location.y, GREIZ_SPOT_X, GREIZ_SPOT_Y);
			if (!g_previous_dest) {
				walk(ACT2_SPAWN_NODE1_X, ACT2_SPAWN_NODE1_Y);
				walk(ACT2_SPAWN_NODE2_X, ACT2_SPAWN_NODE2_Y);
				walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
				walk(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
			} else if (g_previous_dest == FARA || g_previous_dest == LYSANDER) {
				walk(ACT2_CENTRAL_NODE_X, ACT2_CENTRAL_NODE_Y);
				walk(ACT2_WP_SPOT_X, ACT2_WP_SPOT_Y);
			}
			walk(GREIZ_NODE_X, GREIZ_NODE_Y);
			walk(GREIZ_SPOT_X, GREIZ_SPOT_Y);
			break;

		default:
			plugin_error("town", "town_move: destination '%d' unknown\n", where);
			return FALSE;
	}

	g_previous_dest = where;
	return TRUE;
}

bool change_act(int dst_act) {
	if (!town_move(WP)) {
		return FALSE;
	}

	if (dst_act == 1) {
		return waypoint(WP_ROGUEENCAMPMENT);
	} else if (dst_act == 2) {
		return waypoint(WP_LUTGHOLEIN);
	} else if (dst_act == 3) {
		return waypoint(WP_KURASTDOCKS);
	} else if (dst_act == 4) {
		return waypoint(WP_THEPANDEMINOUMFORTRESS);
	} else if (dst_act == 5) {
		return waypoint(WP_HARROGATH);
	}

	plugin_error("town", "change_act: unknow act %d", dst_act);
	return FALSE;
}

bool do_town_chores() {
	plugin_print("town", "running town chores...\n");

	if (!town_move(ORMUS)) {
		return FALSE;
	}
	npc_shop("Ormus"); //fatal?

	// TODO: move to lysander and buy keys
//	if (module_setting("RepairAfterRuns")->i_var && !(runs % module_setting("RepairAfterRuns")->i_var)) {
//		town_move(LYSANDER);
//		npc_key("Lysander");
//	}

	// move to fara and repair equipment
	if (!town_move(FARA)) {
		return FALSE;
	}
	npc_repair("Fara"); //fatal?

	// resurrect merc
	if (module_setting("UseMerc")->b_var && !merc.id) {
		merc_rez++;

		if (!town_move(GREIZ)) {
			return FALSE;
		}

		npc_merc("Greiz"); //fatal?
	}

	return TRUE;
}

bool take_portal(dword id) {
	d2gs_send(0x13, "02 00 00 00 %d", id);

	if (merc.id) {
		msleep(300);

		d2gs_send(0x4b, "01 00 00 00 %d", merc.id);
	}

	msleep(500);

	plugin_print("town", "took portal\n");

	return TRUE;
}

bool go_to_town() {
	int s;

	if (me.intown) {
		plugin_debug("town", "go_to_town: eheh, that was easy\n");

		return TRUE;
	}

	d2gs_send(0x3c, "%d ff ff ff ff", 0xdc); //select skill tp... TODO: use book

	msleep(400);

	pthread_mutex_lock(&townportal_interact_mutex);
	pthread_cleanup_push((cleanup_handler) pthread_mutex_unlock, (void *) &townportal_interact_mutex);

	tp.id = 0;
	d2gs_send(0x0c, "%w %w", me.obj.location.x, me.obj.location.y);

	msleep(500);

	plugin_print("town", "opened townportal\n");

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += 2;

	if ((s = pthread_cond_timedwait(&townportal_interact_cv, &townportal_interact_mutex, &ts))) {
		plugin_error("town", "failed to take townportal\n");
	}
	else {
		take_portal(tp.id);

		me_set_intown(TRUE);
		g_previous_dest = TP;

		msleep(400);

        d2gs_send(0x4b, "00 00 00 00 %d", me.obj.id);
		msleep(200);
	}

	pthread_cleanup_pop(1);

	return s ? FALSE : TRUE;
}


static bool waypoint_extension(char *caller, ...) {
	va_list vl;
	va_start(vl, caller);
	dword area = va_arg(vl, dword);
	va_end(vl);

	return waypoint(area);
}

static bool town_move_extension(char *caller, ...) {
	va_list vl;
	va_start(vl, caller);
	e_town_move where = va_arg(vl, e_town_move);
	va_end(vl);

	return town_move(where);
}

static bool change_act_extension(char *caller, ...) {
	va_list vl;
	va_start(vl, caller);
	int dst_act = va_arg(vl, int);
	va_end(vl);

	return change_act(dst_act);
}

static bool take_portal_extension(char *caller, ...) {
	va_list vl;
	va_start(vl, caller);
	dword id = va_arg(vl, dword);
	va_end(vl);

	return take_portal(id);
}

static bool do_town_chores_extension(char *caller, ...) {
	(void)caller;
	return do_town_chores();
}

static bool go_to_town_extension(char *caller, ...) {
	(void)caller;
	return go_to_town();
}


/* PACKETS HANDLERS */

static int d2gs_item_action(void *p) {
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

static int d2gs_update_npc(void *p) {
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
		plugin_debug("town", "npc update: %s\n", s_npc);
	}

	pthread_mutex_unlock(&npcs_m);

	return FORWARD_PACKET;
}

static int d2gs_assign_npc(void *p) {
	d2gs_packet_t *packet = D2GS_CAST(p);

	npc_t new;
	npc_new(packet, &new);

	pthread_mutex_lock(&npcs_m);

	if (!list_find(&npcs_l, (comparator_t) npc_compare_id, &new)) {
		list_add(&npcs_l, &new);
	}

//TODO: filter monsters

	pthread_mutex_unlock(&npcs_m);

	char s_new[512];
	npc_dump(&new, s_new, 512);
	plugin_debug("town", "detected npc: %s\n", s_new);

	return FORWARD_PACKET;
}

static int process_incoming_packet(void *p) {
	d2gs_packet_t *packet = (d2gs_packet_t *) p;

	switch (packet->id) {
		case 0x11: {
			if (net_get_data(packet->data, 1, dword) == merc.id) {
				merc.id = 0;
			}
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
			plugin_print("town", "%s says: \"%s\"\n", player, message);
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

					plugin_debug("town", "detected waypoint at %i/%i\n", wp.location.x, wp.location.y);
				}
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

		case 0x74: { //pick corpse
			if (net_get_data(packet->data, 1, dword) == me.obj.id) {
				d2gs_send(0x13, "00 00 00 00 %d", net_get_data(packet->data, 5, dword));
				//TODO: be sure everything is picked/equiped + send pots to belt
				msleep(200);
			}
			break;
		}

		case 0x81: {
			if (net_get_data(packet->data, 3, dword) == me.obj.id) { //TODO: 4 byte alignment
				merc.id = net_get_data(packet->data, 7, dword); //TODO: 4 byte alignment
			}
			break;
		}

		case 0x82: {
			if (net_get_data(packet->data, 0, dword) == me.obj.id) {
				plugin_debug("town", "detected bot's town portal \n");

				if (!tp.id) {
					tp.id = net_get_data(packet->data, 20, dword);

					pthread_mutex_lock(&townportal_interact_mutex);
					pthread_cond_signal(&townportal_interact_cv);
					pthread_mutex_unlock(&townportal_interact_mutex);
				} else {
					tp.id = net_get_data(packet->data, 20, dword);
				}
			}
			break;
		}

	}

	return FORWARD_PACKET;
}


/* MODULE */

void town_init() {
	register_packet_handler(D2GS_RECEIVED, 0x0c, d2gs_update_npc);
	register_packet_handler(D2GS_RECEIVED, 0x11, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0xac, d2gs_assign_npc);
	register_packet_handler(D2GS_RECEIVED, 0x22, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x26, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x27, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x51, process_incoming_packet);
	if (setting("Debug")->b_var) // auto-accept party
		register_packet_handler(D2GS_RECEIVED, 0x5a, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x69, d2gs_update_npc);
	register_packet_handler(D2GS_RECEIVED, 0x6d, d2gs_update_npc);
	register_packet_handler(D2GS_RECEIVED, 0x74, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x81, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x82, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x9c, d2gs_item_action);


	register_extension("waypoint", waypoint_extension);
	register_extension("town_move", town_move_extension);
	register_extension("change_act", change_act_extension);
	register_extension("do_town_chores", do_town_chores_extension);
	register_extension("take_portal", take_portal_extension);
	register_extension("go_to_town", go_to_town_extension);

	pthread_mutex_init(&npcs_m, NULL);

	pthread_cond_init(&npc_interact_cv, NULL);
	pthread_mutex_init(&npc_interact_mutex, NULL);

	pthread_cond_init(&townportal_interact_cv, NULL);
	pthread_mutex_init(&townportal_interact_mutex, NULL);

	pthread_mutex_init(&switch_slot_m, NULL);
	pthread_cond_init(&switch_slot_cv, NULL);

	npcs = list_new(npc_t);
	npcs_l = list_new(npc_t);

	tp.id = 42;

	g_previous_dest = VOID; //TODO: might not be enough
}

void town_finit() {
	unregister_packet_handler(D2GS_RECEIVED, 0x0c, d2gs_update_npc);
	unregister_packet_handler(D2GS_RECEIVED, 0x11, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0xac, d2gs_assign_npc);
	unregister_packet_handler(D2GS_RECEIVED, 0x22, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x26, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x27, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x51, process_incoming_packet);
	if (setting("Debug")->b_var) // auto-accept party
		unregister_packet_handler(D2GS_RECEIVED, 0x5a, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x69, d2gs_update_npc);
	unregister_packet_handler(D2GS_RECEIVED, 0x6d, d2gs_update_npc);
	unregister_packet_handler(D2GS_RECEIVED, 0x74, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x81, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x82, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x9c, d2gs_item_action);

	object_clear(merc);
	object_clear(tp);
	object_clear(wp);
	scroll_id = 0;
	g_previous_dest = VOID;
	list_clear(&npcs);
	list_clear(&npcs_l);

	pthread_mutex_destroy(&npcs_m);

	pthread_cond_destroy(&npc_interact_cv);
	pthread_mutex_destroy(&npc_interact_mutex);

	pthread_cond_destroy(&townportal_interact_cv);
	pthread_mutex_destroy(&townportal_interact_mutex);

	pthread_mutex_destroy(&switch_slot_m);
	pthread_cond_destroy(&switch_slot_cv);
}
