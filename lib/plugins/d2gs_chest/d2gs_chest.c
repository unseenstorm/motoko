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

#include "d2gs_chest.h"


/* SETTINGS */

static struct setting module_settings[] = (struct setting []) {
	SETTING("MinGameTime", 180, INTEGER)
};

static struct list module_settings_list = LIST(module_settings, struct setting, 1);

static const word chest_preset_ids[] = {
	1, 3, 4, 7, 9, 50, 51, 52, 53, 54, 55, 56, 57, 58, 79, 80, 84, 85, 88, 89, 90, 93, 94, 95, 96, 97, 109, 138, 139, 142, 143, 154, 158, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 178, 182, 185, 186, 187, 188, 204, 207, 208, 209, 210, 211, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 227, 228, 233, 234, 235, 244, 247, 248, 259, 266, 268, 269, 270, 271, 272, 283, 284, 285, 286, 287, 289, 326, 337, 338, 339, 358, 359, 360, 362, 363, 364, 365, 369, 372, 373, 380, 381, 383, 384, 388, 416, 418, 419, 426, 438, 439, 443, 444, 445, 450, 463, 466, 467, 468, 469, 470, 471, 473, 477, 554, 556  ,//test
	5, 6, 87, 104, 105, 106, 107, 143, 140, 141, 144, 146, 147, 148, 176, 177,
	181, 183, 198, 240, 241, 242, 243, 329, 330, 331, 332, 333, 334, 335, 336,
	354, 355, 356, 371, 387, 389, 390, 391, 397, 405, 406, 407, 413, 420, 424,
	425, 430, 431, 432, 433, 454, 455, 501, 502, 504, 505, 580, 581, 0
};

static struct list chests_l;

pthread_mutex_t chests_m;


/* STATISTICS */

static time_t run_start;

static int runs;

static int runtime;

/* ---------------------------- */

static int chest_compare_id(chest_t *a, chest_t *b) {
	return a->object.id == b->object.id;
}

static void print_chest(chest_t *chest) {
	plugin_debug("chest", "chest at:%d/%d (d=%d), id:%u, preset_id:%d, opened:%d, locked:%d\n", \
				 chest->object.location.x, chest->object.location.y,	\
				 DISTANCE(me.obj.location, chest->object.location),		\
				 chest->object.id, chest->preset_id, chest->is_opened, chest->is_locked);
}

/*
static void print_chest_list() {
	struct iterator it = list_iterator(&chests_l);
	chest_t *chest;

	plugin_debug("chest", "chests_l list:\n");
	while ((chest = iterator_next(&it))) {
		printf("DEBUG: %d: ", it.index);
		print_chest(chest);
	}
	printf("\n");
}
*/

static void open_chest(chest_t *chest) {
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

static chest_t * get_closest_chest() {
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

static void open_all_chests() {
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

		me_move(chest.object.location.x, chest.object.location.y);
		open_chest(&chest);
		chest.is_opened = TRUE;
	}
}

static bool main_script() {
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
	go_to_town();

	town_move(WP);
	waypoint(WP_KURASTBAZAAR);
	precast();
	open_all_chests();
	go_to_town();

	town_move(WP);
	waypoint(WP_UPPERKURAST);
	precast();
	open_all_chests();
	go_to_town();

	town_move(WP);
	waypoint(WP_UPPERKURAST);
	precast();
	find_level_exit("Upper Kurast", "Kurast to Sewer");
	precast();
	open_all_chests();

	find_level_exit("Sewers Level 1", "Sewer Down");
	precast();
	open_all_chests();

	return TRUE;
}


/* PACKETS HANDLERS */

static chest_t * chest_new(d2gs_packet_t *packet, chest_t *new) {
	new->is_opened = !(net_get_data(packet->data, 11, byte) == NEUTRAL);
	new->is_locked = !!(net_get_data(packet->data, 12, byte) == LOCKED);
	new->preset_id = net_get_data(packet->data, 5, word);
	new->object.id = net_get_data(packet->data, 1, dword);
	new->object.location.x = net_get_data(packet->data, 7, word);
	new->object.location.y = net_get_data(packet->data, 9, word);

	return new;
}

static int d2gs_assign_chest(void *p) {
	d2gs_packet_t *packet = D2GS_CAST(p);

	if (net_get_data(packet->data, 0, byte) != 0x02
		|| net_get_data(packet->data, 11, byte) != NEUTRAL) {
		return FORWARD_PACKET;
	}

	int i = 0;

	while (chest_preset_ids[i] && net_get_data(packet->data, 5, word) != chest_preset_ids[i])
		i++;
	if (!chest_preset_ids[i]) {
		return FORWARD_PACKET;
	}

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


/* MODULE */

_export void * module_thread(void *arg) {
	(void)arg;

	time(&run_start);
	plugin_print("chest", "starting chest run %i\n", ++runs);

	sleep(3);

	if (!main_script())
		plugin_print("chest", "error: something went wrong :/\n");

	if (!me.intown) {
		go_to_town();
	}

	if (me.intown) {
		time_t cur;
		int wait_delay = module_setting("MinGameTime")->i_var - (int) difftime(time(&cur), run_start);
		while (wait_delay > 0) {
			plugin_print("chest", "sleeping for %d seconds...\n", wait_delay);
			sleep(5);
			wait_delay -= 5;
		}
	}

	// leave game
	me_leave();

	pthread_exit(NULL);
}

_export bool module_init() {
	register_packet_handler(D2GS_RECEIVED, 0x51, d2gs_assign_chest);

	pthread_mutex_init(&chests_m, NULL);

	chests_l = list_new(chest_t);

	return TRUE;
}

_export bool module_finit() {
	unregister_packet_handler(D2GS_RECEIVED, 0x51, d2gs_assign_chest);

	pthread_mutex_destroy(&chests_m);

	char *s_average, *s_runtime;
	s_average = string_format_time(runs > 0 ? runtime / runs : 0);
	s_runtime = string_format_time(runtime);

	ui_add_statistics_plugin("chest", "runs: %i\n", runs);
	ui_add_statistics_plugin("chest", "average run duration: %s\n", s_average);
	ui_add_statistics_plugin("chest", "total time spent running: %s\n", s_runtime);

	free(s_average);
	free(s_runtime);

	return TRUE;
}

_export void module_cleanup() {
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

	return TRUE;
}
