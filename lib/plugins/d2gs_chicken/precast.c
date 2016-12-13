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

#include "precast.h"

static pthread_mutex_t switch_slot_m;

static pthread_cond_t switch_slot_cv;

//TODO: move to me/d2gs
static void switch_slot(object_t *obj, dword arg) {
	(void)arg;
	(void)obj;
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
		}

		tok = strtok(NULL, "/");
	}
}

bool precast() {
	struct iterator it = list_iterator(&precast_sequence);
	action_t *a;

	while ((a = iterator_next(&it))) {
		a->function(&(me.obj), a->arg);
	}

	return TRUE;
}

static bool precast_extension(char *caller, ...) {
	plugin_print(caller, "casting buff sequence...\n");
	return precast();
}


/* PACKETS HANDLERS */

static int d2gs_on_weapon_switch(void *p) {
	(void)p;

	pthread_mutex_lock(&switch_slot_m);
	pthread_cond_signal(&switch_slot_cv);
	pthread_mutex_unlock(&switch_slot_m);

	return FORWARD_PACKET;
}


/* MODULE */

void precast_init() {
	register_packet_handler(D2GS_RECEIVED, 0x97, d2gs_on_weapon_switch);

	register_extension("precast", precast_extension);
}

void precast_finit() {
	unregister_packet_handler(D2GS_RECEIVED, 0x97, d2gs_on_weapon_switch);

	list_clear(&precast_sequence);
}
