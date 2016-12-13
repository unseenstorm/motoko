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

#ifndef PRECAST_H_
#define PRECAST_H_

#include <data/skills.h>

#include <me.h>

typedef void (*cleanup_handler)(void *);
typedef void(*function_t)(object_t *, dword);

typedef struct {
	function_t function;
	dword arg;
} action_t;

void assemble_sequence(struct list *sequence, char *description);

bool precast(void);

void precast_init(void);
void precast_finit(void);

extern struct list precast_sequence;

#endif /* PRECAST_H_ */
