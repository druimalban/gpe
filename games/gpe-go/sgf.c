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

/*

  gcc -Wall `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0` -o z sgf.c

*/

#include <glib.h>
#include <fcntl.h>  //open()
#include <unistd.h> //close()

GScanner *scanner = NULL;

#define G_TOKEN_SEMICOLON ';'

#include "sgf.h"
#include "sgf-handler.h"

#ifdef DEBUG
#define TRACE(message...)   {g_printerr(message);}
#define TRACE1(message...)  {g_printerr(message); g_printerr("\n");}
#else
#define TRACE(message...)
#define TRACE1(message...) 
#endif

//------------------------------------------------------------------------
// SGF contexts
enum {
  /*used*/CTXT_COLLECTION = 0, // GameTree { GameTree }
  /*used*/CTXT_GAMETREE,       // "(" Sequence { GameTree } ")"
  /*----*/CTXT_SEQUENCE,       //  Node { Node }
  /*used*/CTXT_NODE,           // ";" { Property }
  /*----*/CTXT_PROPERTY,       // PropIdent PropValue { PropValue }
  /*----*/CTXT_PROPIDENT,      // UcLetter { UcLetter }
  /*----*/CTXT_PROPVALUE,      // "[" CValueType "]"
  /*used*/CTXT_CVALUETYPE,     // (ValueType | Compose)
  /*----*/CTXT_VALUETYPE,      // (None | Number | Text | Point  | Move | ...
  /*----*/CTXT_COMPOSE,        // ValueType ":" ValueType
} SgfContext;

gchar * context_name[] = {
  "Collection",
  "GameTree  ",
  "Sequence  ",
  "Node      ",
  "Property  ",
  "Prop Ident",
  "Prop Value",
  "CValuetype",
  "Value type",
  "Compose   ",
};

int sgf_context;

void change_sgf_context(int new_context){
  TRACE("[%s > %s] ", context_name[sgf_context], context_name[new_context]);
  sgf_context = new_context;
  //update scanner's scope
  g_scanner_set_scope(scanner, sgf_context);
}

#ifdef DEBUG
#define TRACE_CTXT() {g_printerr("[             %s] ", context_name[sgf_context]);}
#else
#define TRACE_CTXT()
#endif

//------------------------------------------------------------------------
// SGF symbols (PropIdent)

#define UNSUPPORTED_SGF_PROP -1

static const struct {
  guint   token;
  gchar * name;
  gchar * description;
} symbols[] = {
  { SYMBOL_GM, "GM", "Game" },/*GO == 1*/
  { SYMBOL_FF, "FF", "Fileformat", },/*range: 1-4*/
  { SYMBOL_RU, "RU", "Rules"},
  { SYMBOL_SZ, "SZ", "Size"},
  { SYMBOL_HA, "HA", "Handicap"},
  { SYMBOL_KM, "KM", "Komi"},
  { SYMBOL_PW, "PW", "Player White"},
  { SYMBOL_PB, "PB", "Player Black"},
  { SYMBOL_GN, "GN", "Game name"},
  { SYMBOL_DT, "DT", "Date"},
  { SYMBOL_B , "B" , "Black"},
  { SYMBOL_W , "W" , "White"},
  { SYMBOL_C , "C" , "Comment"},
  { SYMBOL_TM, "TM", "Timelimit"},
  //more to come...
  { 0, NULL, NULL, },
}, *symbol_p = symbols;


gchar * sgf_symbol_name(guint token){
  symbol_p = symbols;
  while (symbol_p->name){
    if(symbol_p->token == token) return symbol_p->name;
    symbol_p++;
  }
  return NULL;
}
gchar * sgf_symbol_description(guint token){
  symbol_p = symbols;
  while (symbol_p->name){
    if(symbol_p->token == token) return symbol_p->description;
    symbol_p++;
  }
  return NULL;
}

int last_sgf_property;

/* Assuming scanner has eaten "[" */
guint parse_sgf_number    (GScanner *scanner){/*int*/
  GTokenType  type;
  GTokenValue value;

  type = g_scanner_cur_token(scanner); 
  if(type != G_TOKEN_INT) return G_TOKEN_INT;

  value = g_scanner_cur_value(scanner);
  TRACE_CTXT();TRACE(" CVALUETYPE int  : %lu\n", value.v_int);

#ifndef STAND_ALONE
  sgf_parsed_prop_int(last_sgf_property, value.v_int);
#endif

  return G_TOKEN_NONE;
}

guint parse_sgf_real      (GScanner *scanner){/*float*/
  GTokenType  type;
  GTokenValue value;

  type = g_scanner_cur_token(scanner); 
  if(type != G_TOKEN_FLOAT) return G_TOKEN_FLOAT;

  value = g_scanner_cur_value(scanner);
  TRACE_CTXT();TRACE(" CVALUETYPE float: %g\n", value.v_float);
  return G_TOKEN_NONE;
}
guint parse_sgf_simpletext(GScanner *scanner){
  /*

  SimpleText is a simple string. Whitespaces other than space must be
  converted to space, i.e. there's no newline! Applications must be
  able to handle SimpleTexts of any size.

  Formatting: linebreaks preceded by a "\" are converted to "", i.e.
  they are removed (same as Text type). All other linebreaks are
  converted to space (no newline on display!!).

  Escaping (same as Text type): "\" is the escape character. Any char
  following "\" is inserted verbatim (exception: whitespaces still
  have to be converted to space!). Following chars have to be escaped,
  when used in SimpleText: "]", "\" and ":" (only if used in compose
  data type).

  */

  GTokenType  type;
  GTokenValue value;

  GString * text;

  text = g_string_sized_new(1000);//will grow if needed

  /**/TRACE_CTXT();TRACE(" CVALUETYPE char*: ");

  //FIXME: tel the scanner to parse EVERYTHING as char!!!
  // fixed: pb with  -->[blabla "ho ho" blabla]<--
  // bug  : parse identifiers, so -->C[blabla FF[3\] ho]<-- *FAILS*.
  // bug  : stop on -->WR[6d]<-- and  -->TM[30:00(5x1:00)]<-- after '(' (parse int error)
  // adding G_CSET_DIGITS to config->cset_identifier_first allow parsing, but then error when parsing int and floats
  //==> workaround, display '?' when unknown token.

  type  = g_scanner_cur_token(scanner); 
  value = g_scanner_cur_value(scanner);

  while(!(type == G_TOKEN_CHAR && value.v_char == ']')){

    if(type == G_TOKEN_STRING){
      TRACE("\"%s\"", value.v_string);
      g_string_append_printf(text,"\"%s\"", value.v_string);
    }
    else if(type == G_TOKEN_INT){
      TRACE("%lu", value.v_int);
      g_string_append_printf(text,"%lu", value.v_int);
    }
    else if(type == G_TOKEN_CHAR && value.v_char == '\\'){//escaped char
      type  = g_scanner_get_next_token(scanner); 
      if(type != G_TOKEN_CHAR) return G_TOKEN_CHAR;

      value = g_scanner_cur_value(scanner);
      switch(value.v_char){
        case 'n':
        case 't':
        case 'r':
        case '\\':
          TRACE("\%c", value.v_char);
          g_string_append_printf(text,"\%c", value.v_char);//realy ???
         break;
        default:
          TRACE("%c", value.v_char);
          g_string_append_c (text, value.v_char);
      }
    }
    else if(type == G_TOKEN_CHAR){
      TRACE("%c", value.v_char);
      g_string_append_c (text, value.v_char);
    }
    else{//unknown token (or parsing error)
      TRACE("?");
      g_string_append_c (text, '?');
    }

    type  = g_scanner_get_next_token(scanner); 
    value = g_scanner_cur_value(scanner);
  }//while

  if(type != G_TOKEN_CHAR /* && ... */) return G_TOKEN_CHAR;

  TRACE("\n");

  if(value.v_char == ']'){
    change_sgf_context(CTXT_NODE);
    TRACE("] (simpletext)\n");
  }

#ifndef STAND_ALONE
  sgf_parsed_prop_string(last_sgf_property, text->str);
#endif
  /* gchar* */g_string_free(text, FALSE);

  return G_TOKEN_NONE;
}

guint parse_sgf_move      (GScanner *scanner){/*two letters*/
  GTokenType  type;
  GTokenValue value;
  char col;
  char row;

  //FIXME: support B[] for "pass"
  type = g_scanner_cur_token(scanner); 
  if(type != G_TOKEN_CHAR) return G_TOKEN_CHAR;

  value = g_scanner_cur_value(scanner);
  col = value.v_char;

  type = g_scanner_get_next_token(scanner); 
  if(type != G_TOKEN_CHAR) return G_TOKEN_CHAR;

  value = g_scanner_cur_value(scanner);
  row = value.v_char;

  /**/TRACE_CTXT();TRACE(" CVALUETYPE move: (%c,%c)\n", col, row);

#ifndef STAND_ALONE
  sgf_parsed_prop_move(last_sgf_property, row, col);
#endif

  return G_TOKEN_NONE;
}
//guint parse_sgf_double    (GScanner *scanner){/*1/2*/}
//guint parse_sgf_color     (GScanner *scanner){/*B/W*/}
//guint parse_sgf_text      (GScanner *scanner){}
//guint parse_sgf_point     (GScanner *scanner){} cf move
//guint parse_sgf_stone     (GScanner *scanner){} cf move

guint parse_unsupported_prop(GScanner *scanner){
//  GTokenType  type;
//  GTokenValue value;

  return parse_sgf_simpletext(scanner);
}

static guint parse_sgf_CValueType (GScanner *scanner){
  guint expected_token = G_TOKEN_NONE;

  //NOTE: add value type to symbols[] and loop on it (?)
  switch(last_sgf_property){
    case SYMBOL_GM:
    case SYMBOL_FF:
    case SYMBOL_SZ:
    case SYMBOL_HA:
      expected_token = parse_sgf_number(scanner);
      break;
    case SYMBOL_KM:
      expected_token = parse_sgf_real(scanner);
      break;
    case SYMBOL_RU:
    case SYMBOL_PW:
    case SYMBOL_PB:
    case SYMBOL_GN:
    case SYMBOL_DT:
    case SYMBOL_TM://FIXME: is a real (!)
    case SYMBOL_C ://FIXME: is a text
      scanner->config->scan_float            = FALSE;
      expected_token = parse_sgf_simpletext(scanner);
      break;
    case SYMBOL_W :
    case SYMBOL_B :
      expected_token = parse_sgf_move(scanner);
      break;
    case UNSUPPORTED_SGF_PROP:
    default:
      /**/TRACE_CTXT();TRACE("Unsupported ID\n");
      scanner->config->scan_float            = FALSE;
      parse_unsupported_prop(scanner);
      return G_TOKEN_NONE;
  }
  return expected_token;
}

GScanner * sgf_scanner_new(){
  GScanner * scanner;

  scanner = g_scanner_new (NULL);

  //Character sets
  scanner->config->cset_skip_characters  = "\t\r\n";
  scanner->config->cset_identifier_first = G_CSET_A_2_Z ".";
  scanner->config->cset_identifier_nth   = G_CSET_A_2_Z;
  //scanner->config->cpair_comment_single  = "#\n";
   
  //Should symbol lookup work case sensitive?
  scanner->config->case_sensitive = TRUE;

  //Other parameters
  //scanner->config->skip_comment_multi    = TRUE;// C like comment
  //scanner->config->skip_comment_single   = TRUE;// single line comment
  //scanner->config->scan_comment_multi    = TRUE;// scan multi line comments?

  scanner->config->scan_identifier       = TRUE;// OK
  scanner->config->scan_identifier_1char = TRUE;// OK : "B" "W" ...
  scanner->config->scan_identifier_NULL  = FALSE;
  scanner->config->scan_symbols          = TRUE;// OK
  //scanner->config->scan_binary           = TRUE;
  //scanner->config->scan_octal            = TRUE;
  scanner->config->scan_float            = TRUE;
  //scanner->config->scan_hex              = TRUE; // `0x0ff0'
  //scanner->config->scan_hex_dollar       = TRUE; // `$0ff0'
  scanner->config->scan_string_sq        = FALSE;//TRUE; // string: 'anything'
  scanner->config->scan_string_dq        = TRUE; // string: "\\-escapes!\n"
  //scanner->config->numbers_2_int         = TRUE; // bin, octal, hex => int
  scanner->config->int_2_float           = FALSE; // OK // int => G_TOKEN_FLOAT?

  scanner->config->identifier_2_string   = FALSE;
  scanner->config->char_2_token          = TRUE; /* return G_TOKEN_CHAR? */
  scanner->config->symbol_2_token        = FALSE;

  scanner->config->scope_0_fallback      = FALSE; // try scope 0 on lookups?
  //store_int64           = ; /* use value.v_int64 rather than v_int */


  //--Load symbols into the scanner

  //semicolon ";"
  g_scanner_scope_add_symbol (scanner, CTXT_NODE, ";",
                              GINT_TO_POINTER (G_TOKEN_SEMICOLON));
  //FF properties
  while (symbol_p->name){
      g_scanner_scope_add_symbol (scanner,
                                  CTXT_NODE,
                                  symbol_p->name,
                                  GINT_TO_POINTER (symbol_p->token));
      symbol_p++;
  }

  return scanner;
}

//------------------------------------------------------------------------
int load_sgf_file(const char * filename){
  int file_descriptor;
  int gametree_level;
  gint  content = 0;// see case: G_TOKEN_IDENTIFIER

  sgf_parsed_init();

  last_sgf_property = 0;

  //--Config scanner
  if(!scanner) scanner = sgf_scanner_new();

  //--Load the file
  file_descriptor = open(filename, O_RDONLY);
  if(file_descriptor == -1){
    g_printerr("Error opening %s\n", filename);
    return(SGF_ERROR_OPEN_FILE);
  }
  g_scanner_input_file(scanner, file_descriptor);

  //give the error handler an idea on how the input is named
  scanner->input_name = filename;

  //-----------------------------------------------------------
  //--Scanner main loop

  sgf_context = CTXT_COLLECTION;
  gametree_level = 0;
  /**/TRACE_CTXT();TRACE("\n");

  while(!g_scanner_eof(scanner)){
    GTokenType  type;
    GTokenValue value;

    if(sgf_context == CTXT_CVALUETYPE){

      //change the scanner config on specific context
      //FIXME: create 2 static GScannerConfig, and switch them.
      scanner->config->cset_skip_characters  = "";
      scanner->config->scan_identifier_1char = FALSE;
      scanner->config->char_2_token          = FALSE; /* return G_TOKEN_CHAR? */

      type  = g_scanner_get_next_token(scanner);
      value = g_scanner_cur_value(scanner);

      if(type == G_TOKEN_CHAR && value.v_char == ']'){//--> back to Node level
          change_sgf_context(CTXT_NODE);
          TRACE("] (main loop)\n");
      }
      else{
        GTokenType expected_token = G_TOKEN_NONE;
        expected_token = parse_sgf_CValueType(scanner);

        if(expected_token != G_TOKEN_NONE){
          TRACE("\n***> error parsing CValueType (expected: %d, got: %d)\n",
                  expected_token,
                  g_scanner_cur_token(scanner));
        }
      }      

      //revert scanner config
      scanner->config->cset_skip_characters  = "\t\r\n";
      scanner->config->scan_identifier_1char = TRUE;
      scanner->config->char_2_token          = TRUE; /* return G_TOKEN_CHAR? */
      scanner->config->scan_float            = TRUE;

    }
    else {
      type  = g_scanner_get_next_token(scanner);

      switch(type){
        case ' '://eat white spaces
        break;

      case G_TOKEN_LEFT_PAREN: //GameTree = "(" Node { Node } { GameTree } ")"
        gametree_level++;
        change_sgf_context(CTXT_GAMETREE);
#ifndef STAND_ALONE
        sgf_parsed_open_gametree();
#endif
        TRACE("(\n");
        break;
      case G_TOKEN_RIGHT_PAREN:
        gametree_level--;//should not go below 0
        if(gametree_level == 0){
          change_sgf_context(CTXT_COLLECTION);
        }
        else{
          change_sgf_context(CTXT_GAMETREE);
        }
#ifndef STAND_ALONE
        sgf_parsed_close_gametree();
#endif
        TRACE(")\n");
        g_scanner_set_scope(scanner, sgf_context);
        break;

      case G_TOKEN_SEMICOLON:
        //Node = ";" { Property }
        //Property   = PropIdent PropValue { PropValue }
        //PropIdent  = UcLetter { UcLetter }
        //PropValue  = "[" CValueType "]"
        change_sgf_context(CTXT_NODE);
        TRACE(";\n");
        break;

      case G_TOKEN_LEFT_BRACE: //PropValue  = "[" CValueType "]" 
        change_sgf_context(CTXT_CVALUETYPE);
        TRACE("[\n");
        break;
      case G_TOKEN_RIGHT_BRACE:
        change_sgf_context(CTXT_NODE);//useless... (?)
        TRACE("]\n");
        break;

      case G_TOKEN_SYMBOL:
        {
          guint symbol_value;
          value = g_scanner_cur_value(scanner);
          symbol_value = GPOINTER_TO_INT(value.v_symbol);

          TRACE_CTXT();
          if(FIRST_SGF_SYMBOL <= symbol_value && symbol_value <= LAST_SGF_SYMBOL){
            TRACE(" sgf SYMBOL #%d : %s (%s)\n",
                       symbol_value,
                       sgf_symbol_name(symbol_value),
                       sgf_symbol_description(symbol_value)
                       );
            last_sgf_property = symbol_value;
          }
          //else{
          //  TRACE(" reg SYMBOL #%3d : %s\n", symbol_value, );
          //}
        }
        break;

      case G_TOKEN_IDENTIFIER:
        content++;
        value = g_scanner_cur_value(scanner);
        TRACE_CTXT();
        TRACE(" new SYMBOL #%03d : %s\n", content, value.v_identifier);

        ////register it as a new symbol
        //g_scanner_scope_add_symbol(scanner, sgf_context,
        //                           value.v_identifier,
        //                           GINT_TO_POINTER(content));
        last_sgf_property = UNSUPPORTED_SGF_PROP;
       break;

      case G_TOKEN_ERROR:
        TRACE_CTXT();g_scanner_error(scanner, "scanning ERROR !!!");
        break;

      case G_TOKEN_EOF:
        TRACE_CTXT();TRACE("EOF\n");
        break;

      default:
        TRACE_CTXT();
        TRACE("unknown token (%3d)", type);
        if(type < 256){ TRACE(" - '%c'\n", type);}
        else{ TRACE("\n");}

    }//switch
    }
  }//while

  //--Clean up
  //g_scanner_destroy (scanner);
  close(file_descriptor);

  sgf_parsed_end();

  return SGF_ERROR_NONE;
}

#ifdef STAND_ALONE
int main(int argc, char ** argv){
  load_sgf_file(argv[1]);
  return 0;
}
#endif
