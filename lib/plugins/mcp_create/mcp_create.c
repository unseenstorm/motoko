/*
 * Copyright (C) 2010 gonzoj
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

#define _PLUGIN

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <module.h>

#include <mcp.h>
#include <packet.h>
#include <settings.h>
#include <gui.h>
#include <internal.h>

#include <util/list.h>
#include <util/net.h>

#define msleep(ms) usleep((ms) * 1000)

static struct setting module_settings[] = (struct setting []) {
	SETTING("NameList", .s_var = "", STRING),
	SETTING("Delay", .i_var = 5000, INTEGER)
};

static struct list module_settings_list = LIST(module_settings, struct setting, 2);

#define module_setting(name) ((struct setting *)list_find(&module_settings_list, (comparator_t) compare_setting, name))

typedef void (*setting_cleaner_t)(struct setting *);

typedef struct {
	setting_cleaner_t cleanup;
	struct setting *set;
} setting_cleanup_t;

struct list setting_cleaners = LIST(NULL, setting_cleanup_t, 0);

void cleanup_string_setting(struct setting *);

int mcp_startup_handler(void *p);
int mcp_charlist_handler(void *p);
int mcp_charcreate_handler(void *p);

_export const char * module_get_title() {
	return "mcp create";
}

_export const char * module_get_version() {
	return "0.1.0";
}

_export const char * module_get_author() {
	return "notvita";
}

_export const char * module_get_description() {
	return "creates chars";
}

_export int module_get_license() {
	return LICENSE_GPL_V3;
}

_export module_type_t module_get_type() {
	return (module_type_t) { MODULE_MCP, MODULE_ACTIVE };
}

_export bool module_load_config(struct setting_section *s) {
	int i;
	for (i = 0; i < s->entries; i++) {
		struct setting *set = module_setting(s->settings[i].name);

		if (set) {
			if (s->settings[i].type == STRING) {
				if (set->type == INTEGER) {
					sscanf(s->settings[i].s_var, "%li",  &set->i_var);
				}
			}

			if (s->settings[i].type == INTEGER && set->type == INTEGER) {
				set->i_var = s->settings[i].i_var;
			}
		}
	}
	return TRUE;
}

_export bool module_init() {
	register_packet_handler(MCP_RECEIVED, 0x01, mcp_startup_handler);
	register_packet_handler(MCP_RECEIVED, 0x19, mcp_charlist_handler);
	register_packet_handler(MCP_RECEIVED, 0x02, mcp_charcreate_handler);

	return TRUE;
}

_export bool module_finit() {
	unregister_packet_handler(MCP_RECEIVED, 0x01, mcp_startup_handler);
	unregister_packet_handler(MCP_RECEIVED, 0x19, mcp_charlist_handler);
	unregister_packet_handler(MCP_RECEIVED, 0x02, mcp_charcreate_handler);

	struct iterator it = list_iterator(&setting_cleaners);
	setting_cleanup_t *sc;

	while ((sc = iterator_next(&it))) {
		sc->cleanup(sc->set);
	}

	list_clear(&setting_cleaners);

	return TRUE;
}

_export void * module_thread(void *arg) {
	(void)arg;
	return NULL;
}

_export void module_cleanup() {
	return;
}

void cleanup_string_setting(struct setting *s) {
	free(s->s_var);
}

int mcp_startup_handler(void *p) {
	mcp_packet_t incoming = *MCP_CAST(p);

	dword status = net_get_data(incoming.data, 0, dword);

	switch (status) {

	case 0x00: {
		plugin_print("mcp create", "successfully logged on to realm\n");
		break;
	}

	case 0x7e: {
		plugin_error("mcp create", "error: CD key banned from this realm\n");

		plugin_print("mcp create", "sleeping for 1h\n");
		sleep(3600);

		return FORWARD_PACKET;
	}

	case 0x7f: {
		plugin_error("mcp create", "error: temporary IP ban (\"your connection has been temporarily restricted from this realm.\")\n");

		plugin_print("mcp create", "sleeping for 1h\n");
		sleep(3600);

		plugin_print("mcp create", "requesting client restart\n");

		internal_send(INTERNAL_REQUEST, "%d", CLIENT_RESTART);

		return FORWARD_PACKET;
	}

	default: {
		plugin_error("mcp create", "error: realm unavailable (%i)\n", status);

		return FORWARD_PACKET;
	}

	}

	//plugin_print("mcp create", "request list of characters\n");

	mcp_send(0x19, "08 00 00 00");

	return FORWARD_PACKET;
}

int mcp_charlist_handler(void *p) {
	mcp_packet_t incoming = *MCP_CAST(p);

	word count = net_get_data(incoming.data, 6, word);

	if (count == 8) {
		plugin_print("mcp create", "account is full");

		return FORWARD_PACKET;
	}

	msleep(module_setting("Delay")->i_var);

	// create
	mcp_send(0x02, "%d %w %s 00", 0x01, 0x00, "tsstxotonf");

	return FORWARD_PACKET;
}

int mcp_charcreate_handler(void *p) {
	mcp_packet_t incoming = *MCP_CAST(p);

	dword result = net_get_data(incoming.data, 0, dword);

	switch (result) {

	case 0x00: {
		plugin_print("mcp create", "successfully created character\n");
		break;
	}

	// character already exists
	case 0x14: {
		plugin_error("mcp create", "character exists\n");
		break;
	}

	case 0x15: {
		plugin_error("mcp create", "invalid character name\n");
		sleep(1);
		break;
	}

	}

	mcp_send(0x19, "08 00 00 00");

	return FORWARD_PACKET;
}
