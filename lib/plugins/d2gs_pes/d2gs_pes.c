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

#include "d2gs_pes.h"

/* ---------------------------- */

/* SETTINGS */

static struct setting module_settings[] = (struct setting []) {
	SETTING("RunPindleskin", FALSE, BOOLEAN),
	SETTING("RunEldritch", FALSE, BOOLEAN),
	SETTING("RunShenk", FALSE, BOOLEAN),
	SETTING("CastDelay", 200, INTEGER),
	SETTING("MaxSequencesPerMonster", 0, INTEGER)
};

static struct list module_settings_list = LIST(module_settings, struct setting, 5);

/* NPCs / OBJECTS */

static const char *s_npc_mod[] = {
	"none", "rndname", "hpmultiply", "light", "leveladd", "strong", "fast", "curse",
	"resist", "fire", "poisondead", "durieldead", "bloodraven", "rage", "spcdamage", "partydead",
	"champion", "lightning", "cold", "hireable", "scarab", "killself", "questcomplete", "poisonhit",
	"thief", "manahit", "teleport", "spectralhit", "stoneskin", "multishot", "aura", "goboom",
	"fireskipe_explode", "suicideminion_explode", "ai_after_death", "shatter_on_death", "ghostly", "fanatic", "possessed", "berserk",
	"worms_on_death", "always_run_ai", "lightningdeath"
};

static struct list npcs;

static struct list npcs_l;

static pthread_mutex_t npcs_m;

static object_t rp;

static unsigned long long int skip_mods = 0;

/* ACTIONS */

static struct list attack_sequence = LIST(NULL, action_t, 0);

/* STATISTICS */

static time_t run_start;

static int runs;

static int runtime;

static int merc_rez;

/* ---------------------------- */

static int skill_lookup(char *skill) {
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

static void assemble_sequence(struct list *sequence, char *description) {
	int lskill = -1, rskill = -1;

	string_to_lower_case(description);

	char *tok = strtok(description, "/");
	while (tok) {
		char *separator = strchr(tok, ':');
		if (separator) {
			*separator = '\0';
			char *side = tok;
			char *s_skill = ++separator;
			int skill = skill_lookup(s_skill);

			if (skill >= 0) {

				if (!strcmp(side, "left") && skill >= 0) {
					if (skill != lskill) {
						action_t a = { me_swap_left, skill };
						list_add(sequence, &a);
						lskill = skill;
					}

					action_t a = { me_cast_left, 0 };
					list_add(sequence, &a);
				} else if (!strcmp(side, "right") && skill >= 0) {
					if (skill != rskill) {
						action_t a = { me_swap_right, skill };
						list_add(sequence, &a);
						rskill = skill;
					}

					action_t a = { me_cast_right, 0 };
					list_add(sequence, &a);
				} else if (!strcmp(side, "self") && skill >= 0) {
					if (skill != rskill) {
						action_t a = { me_swap_right, skill };
						list_add(sequence, &a);
						rskill = skill;
					}

					action_t a = { me_cast_self, 0 };
					list_add(sequence, &a);
				}
			}
		}

		tok = strtok(NULL, "/");
	}
}

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

static int npc_lookup_mod(char *mod) {
	int i;

	for (i = 0; i < N_NPC_MOD; i++) {
		if (string_compare(mod, (char *) s_npc_mod[i], FALSE)) {
			return i;
		}
	}

	return -1;
}

static char * npc_mod_string(long long int mod, char *s_mod, size_t len) {
	memset(s_mod, 0, len);

	int i;

	for (i = 0; i < N_NPC_MOD; i++) {
		if (mod & (1 << i)) {
			if (strlen(s_mod) && strlen(s_npc_mod[i])) {
				strncat(s_mod, " ", len - strlen(s_mod) - 1);
			}
			strncat(s_mod, s_npc_mod[i], len - strlen(s_mod) - 1);
		}
	}

	return s_mod;
}

static char * npc_dump(npc_t *npc, char *s_npc, size_t len) {
	snprintf(s_npc, len - 1, "%s (%08X) %i%% %i/%i", npc_lookup_name(npc), npc->object.id, NPC_HP(npc->hp), npc->object.location.x, npc->object.location.y);
	if (npc->mods) {
		strncat(s_npc, "(", len - strlen(s_npc) - 1);
		char s_mod[512];
		npc_mod_string(npc->mods, s_mod, 512);
		strncat(s_npc, s_mod, len - strlen(s_npc) - 1);
		strncat(s_npc, ")", len - strlen(s_npc) - 1);
	}

	return s_npc;
}

static int npc_compare_id(npc_t *a, npc_t *b) {
	return a->object.id == b->object.id;
}

static void npc_refresh() {
	pthread_mutex_lock(&npcs_m);

	list_clear(&npcs);

	list_clone(&npcs_l, &npcs);

	pthread_mutex_unlock(&npcs_m);
}

static void npc_attack(npc_t *npc) {
	int i;

	plugin_print("pes", "attacking %s\n", npc_lookup_name(npc));

	for (i = 0; npc->hp > 0; i++) {

		if (module_setting("MaxSequencesPerMonster")->i_var) {
			if (module_setting("MaxSequencesPerMonster")->i_var <= i) {
				break;
			}
		}

		struct iterator it = list_iterator(&attack_sequence);
		action_t *a;

		while ((a = iterator_next(&it))) {
			a->function(&(me.obj), a->arg);
		}

		plugin_print("pes", "%s's hp: %i%%\n", npc_lookup_name(npc), NPC_HP(npc->hp));
	}
}

static void attack(char *s_npc) {
	npc_refresh();

	struct iterator it = list_iterator(&npcs);
	npc_t *npc;

	bool found = FALSE;

	while ((npc = iterator_next(&it))) {
		if (string_compare(s_npc, npc_lookup_name(npc), FALSE)) {
			if (DISTANCE(me.obj.location, npc->object.location) > 50) {
				plugin_debug("pes", "%s out of range (%i, %i/%i <> %i/%i)\n", npc_lookup_name(npc), DISTANCE(me.obj.location, npc->object.location), me.obj.location.x, me.obj.location.y, npc->object.location.x, npc->object.location.y);

				found = TRUE;

				npc_refresh();

				continue;
			}

			if (NPC_MOD(npc, skip_mods)) {
				char s_mod[512];
				npc_mod_string(NPC_MOD(npc, skip_mods), s_mod, 512);
				plugin_print("pes", "skip %s (mods: %s)\n", npc_lookup_name(npc), s_mod);

				found = TRUE;

				npc_refresh();

				continue;
			}

			npc_attack(npc);

			found = TRUE;
		}

		npc_refresh();
	}

	if (!found) plugin_debug("pes", "couldn't find %s\n", s_npc);
}

static bool main_script() {
	/*
	// move to larzuk and repair equipment
	if (module_setting("RepairAfterRuns")->i_var && !(runs % module_setting("RepairAfterRuns")->i_var)) {
		me_move(5106, 5045);
		me_move(5124, 5040);
		me_move(5144, 5044);

		npc_repair("Larzuk");

		// move back to spawn
		me_move(5144, 5044);
		me_move(5124, 5040);
		me_move(5106, 5045);
		me_move(5098, 5018);
	}

	// move to malah and heal / shop
	me_move(5089, 5029);
	me_move(5074, 5027);

	npc_shop("Malah");

	me_move(5074, 5027);
	me_move(5072, 5040);
	me_move(5080, 5062);

	// resurrect merc
	if (module_setting("UseMerc")->b_var && !merc.id) {
		merc_rez++;

		me_move(5082, 5079);
		me_move(5061, 5076);

		npc_merc("qual-kehk");

		me_move(5082, 5079);
	} else {
		me_move(5082, 5079);
	}
	*/

	if (me.act == 5) { //okok, I'll improve that town_move thing...
		// move to waypoint from spawn
		me_move(5104, 5043);
		me_move(5114, 5069);
		waypoint(WP_KURASTDOCKS);
	}
	do_town_chores();
	town_move(WP);
	waypoint(WP_HARROGATH);

	plugin_print("pes", "starting P/E/S run %i\n", ++runs);

	if (module_setting("RunPindleskin")->b_var) {
		// move to red portal
		me_move(5081, 5088);
		me_move(5085, 5112);
		me_move(5108, 5123);
		me_move(5119, 5123);

		msleep(700);

		// take portal
		take_portal(rp.id);
		me_set_intown(FALSE);

		precast();

		plugin_print("pes", "running pindleskin now...\n");

		// teleport to pindleskin - let's test farcast
		me_move(10062, 13287);
		me_move(10061, 13262);
		me_move(10058, 13235);

		msleep(500); // dirty fix; detecting pindle is delayed so we have to give it some time

		// attack pindleskin
		//execute_module_schedule(MODULE_D2GS);
		attack("pindleskin");
		attack("defiled warrior");

		msleep(1200);

		// pickit
		//internal_send(0x9c, "%s 00", "pickit");
		execute_module_schedule(MODULE_D2GS);

		if (module_setting("RunEldritch")->b_var || module_setting("RunShenk")->b_var) {

			go_to_town();

			// move to waypoint from tp
			/* town_move(WP); */ //TODO
			me_move(5104, 5043);
			me_move(5114, 5069);
		}

	} else if (module_setting("RunEldritch")->b_var || module_setting("RunShenk")->b_var) {
		// move to waypoint from qual-khek
		/* town_move(WP); */ //TODO
		me_move(5102, 5087);
		me_move(5120, 5084);
		me_move(5114, 5069);
	}

	if (module_setting("RunEldritch")->b_var ||  module_setting("RunShenk")->b_var) {
		waypoint(WP_FRIGIDHIGHLANDS);
		precast();
	}

	if (module_setting("RunEldritch")->b_var) {
		plugin_print("pes", "running eldritch now...\n");

		// teleport to eldritch
		me_move(3763, 5094);
		me_move(3741, 5074);

		// attack eldritch
		//execute_module_schedule(MODULE_D2GS);
		attack("eldritch the rectifier");
		attack("enslaved");

		msleep(1200);

		// pickit
		//internal_send(0x9c, "%s 00", "pickit");
		execute_module_schedule(MODULE_D2GS);

		if (module_setting("RunShenk")->b_var) {
			// teleport to shenk
			me_move(3741, 5074);
			me_move(3763, 5085);
		}
	}

	if (module_setting("RunShenk")->b_var) {
		plugin_print("pes", "running shenk now...\n");

		me_move(3796, 5100);
		me_move(3822, 5107);
		me_move(3847, 5117);
		me_move(3869, 5116);
		me_move(3887, 5112);

		// attack shenk
		//execute_module_schedule(MODULE_D2GS);
		attack("shenk the overseer");
		attack("enslaved");

		// clear bloody foothills
		// monsters:
		// enslaved, death mauler,

		msleep(1200);

		// pickit
		//internal_send(0x9c, "%s 00", "pickit");
		execute_module_schedule(MODULE_D2GS);
	}


	return TRUE;
}


/* PACKETS HANDLERS */

static int d2gs_assign_npc(void *p) {
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
	plugin_debug("pes", "detected npc: %s\n", s_new);

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
		plugin_debug("pes", "npc update: %s\n", s_npc);
	}

	pthread_mutex_unlock(&npcs_m);

	return FORWARD_PACKET;
}

static int process_incoming_packet(void *p) {
	d2gs_packet_t *packet = (d2gs_packet_t *) p;

	switch (packet->id) {

		case 0x0c: {
			d2gs_update_npc(packet);
			break;
		}

		case 0x26: {
			if (net_get_data(packet->data, 0, byte) == 0x05) break;

			char *player = (char *) (packet->data + 9);
			char *message = (char *) (packet->data + 9 + strlen(player) + 1);
			plugin_print("pes", "%s says: \"%s\"\n", player, message);
			break;
		}

		case 0x51: {
			if (net_get_data(packet->data, 0, byte) == 0x02 && net_get_data(packet->data, 5, word) == 0x3c) {
				rp.id = net_get_data(packet->data, 1, dword);
				rp.location.x = net_get_data(packet->data, 7, word);
				rp.location.y = net_get_data(packet->data, 9, word);

				plugin_debug("pes", "detected red portal at %i/%i\n", rp.location.x, rp.location.y);
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

		case 0xac: {
			d2gs_assign_npc(packet);
			break;
		}

	}

	return FORWARD_PACKET;
}


/* MODULE */

_export void * module_thread(void *unused) {
	(void)unused;
	time(&run_start);

	sleep(3);

	if (!main_script())
		plugin_print("chest", "error: something went wrong :/\n");

	// leave game
	me_leave();

	pthread_exit(NULL);
}

_export bool module_init() {
	register_packet_handler(D2GS_RECEIVED, 0x03, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x11, process_incoming_packet);
	register_packet_handler(D2GS_RECEIVED, 0x59, process_incoming_packet);
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

	pthread_mutex_init(&npcs_m, NULL);

	npcs = list_new(npc_t);
	npcs_l = list_new(npc_t);

	return TRUE;
}

_export bool module_finit() {
	unregister_packet_handler(D2GS_RECEIVED, 0x03, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x11, process_incoming_packet);
	unregister_packet_handler(D2GS_RECEIVED, 0x59, process_incoming_packet);
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

	pthread_mutex_destroy(&npcs_m);
	list_clear(&attack_sequence);

	char *s_average, *s_runtime;
	s_average = string_format_time(runs > 0 ? runtime / runs : 0);
	s_runtime = string_format_time(runtime);

	ui_add_statistics_plugin("pes", "runs: %i\n", runs);
	ui_add_statistics_plugin("pes", "average run duration: %s\n", s_average);
	ui_add_statistics_plugin("pes", "total time spent running: %s\n", s_runtime);
	ui_add_statistics_plugin("pes", "merc resurrections: %i\n", merc_rez);

	free(s_average);
	free(s_runtime);

	return TRUE;
}

_export void module_cleanup() {
	object_clear(rp);
	list_clear(&npcs);
	list_clear(&npcs_l);

	if (!runs) return;

	time_t cur;
	int duration = (int) difftime(time(&cur), run_start);
	runtime += duration;

	char * s_duration, * s_average;
	s_duration = string_format_time(duration);
	s_average = string_format_time(runs > 0 ? runtime / runs : 0);

	plugin_print("pes", "run took %s (average: %s)\n", s_duration, s_average);

	free(s_duration);
	free(s_average);
}

_export const char * module_get_title() {
	return "pes";
}

_export const char * module_get_version() {
	return "0.1.0";
}

_export const char * module_get_author() {
	return "gonzoj";
}

_export const char * module_get_description() {
	return "a simple bot for pindle, eldritch and shenk";
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
		if (!strcmp(s->settings[i].name, "AttackSequence")) {
			char *c_setting = strdup(s->settings[i].s_var);
			assemble_sequence(&attack_sequence, c_setting);
			free(c_setting);
		} else if (!strcmp(s->settings[i].name, "SkipMods")) {
			char *c_setting = strdup(s->settings[i].s_var);
			char *tok = strtok(c_setting, ":");
			while (tok) {
				int mod = npc_lookup_mod(tok);
				if (mod >= 0) {
					skip_mods |= (1 << mod);
				}

				tok = strtok(NULL, ":");
			}
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


//TODO: see the duplicates npc/monsters with this file and d2gs_chicken/town.c -> split town folks from monsters
