/*
 * copyright (c) 2011 gonzoj
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

#include "d2gs_meph.h"

static struct setting module_settings[] = (struct setting []) {
	SETTING("CastDelay", INTEGER, 250)
};

static struct list module_settings_list = LIST(module_settings, struct setting, 1);


static bool main_script() {
	if (me.act < 1 || me.act > 3) {
		plugin_error("meph", "can't start from act %i\n", me.act);
		return FALSE;
	}

	do_town_chores();

	plugin_print("meph", "running mephisto...\n");

	town_move(WP);
	waypoint(WP_DURANCEOFHATELEVEL2);
	precast();

	find_level_exit("Durance of Hate Level 2", "Mephisto Down");
	precast();
	//eheh, yep that's all...

	return TRUE;
}


/* MODULE */

_export void * module_thread(void *arg) {
	(void)arg;
	time_t run_start;
	time(&run_start);

	if (!main_script())
		plugin_print("meph", "error: something went wrong :/\n");

	if (!me.intown) {
		go_to_town();
	}

	me_leave();

	time_t cur;
	int runtime = (int) difftime(time(&cur), run_start);
	char *s_runtime = string_format_time(runtime);
	plugin_print("meph", "run took %s\n", s_runtime);
	free(s_runtime);
	pthread_exit(NULL);
}

_export bool module_init() {
	return TRUE;
}

_export bool module_finit() {
	return TRUE;
}

_export void module_cleanup() {
}

_export const char * module_get_title() {
	return "meph";
}

_export const char * module_get_version() {
	return "0.1.0";
}

_export const char * module_get_author() {
	return "gonzoj";
}

_export const char * module_get_description() {
	return "supposed to do mephisto runs";
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
			/*if (s->settings[i].type == STRING) {
				set->s_var = strdup(s->settings[i].s_var);
				if (set->s_var) {
					setting_cleanup_t sc = { cleanup_string_setting, set };
					list_add(&setting_cleaners, &sc);
				}
			}*/
		}
	}
	return TRUE;
}
