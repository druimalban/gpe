/* gpe-go
 *
 * Copyright (C) 2003 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef SGF_HANDLER_H
#define SGF_HANDLER_H

#include "sgf.h"

void sgf_parsed_init();
void sgf_parsed_end();

void sgf_parsed_open_gametree ();
void sgf_parsed_close_gametree();

void sgf_parsed_prop_int      (PropIdent prop_id, int value);
void sgf_parsed_prop_string   (PropIdent prop_id, const char * s);
void sgf_parsed_prop_move     (PropIdent prop_id, char row, char col);

#endif
