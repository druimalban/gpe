/* gpe-go, a GO board for GPE
 *
 * $Id$
 *
 * Copyright (C) 2003-2004 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef SGF_H
#define SGF_H

enum sgf_scanner_error{
  SGF_ERROR_NONE = 0,
  SGF_ERROR_OPEN_FILE
};

int load_sgf_file(const char * filename);

//------------------------------------------------------------------------
// SGF symbols (PropIdent)
enum _PropIdent{
  FIRST_SGF_SYMBOL = G_TOKEN_LAST + 1,
  SYMBOL_GM,
  SYMBOL_FF,
  SYMBOL_RU,
  SYMBOL_SZ,
  SYMBOL_HA,
  SYMBOL_KM,
  SYMBOL_PW,
  SYMBOL_PB,
  SYMBOL_GN,
  SYMBOL_DT,
  SYMBOL_B ,
  SYMBOL_W ,
  SYMBOL_C ,
  SYMBOL_TM,
  //more to come...
  LAST_SGF_SYMBOL
};
typedef enum _PropIdent PropIdent;

#endif
