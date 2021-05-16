/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#ifndef __STRUCTS_H__
#define __STRUCTS_H__

#include "types.h"
#include "internal.defs.h"

// Used for table of object values in constants.c
struct oedit_objval_tab {
  char *prompt;
  SInt32 min;
  SInt32 max;
  const char **values;		/* list of values, terminated with '\n'   */
  SInt8 type;			/* type of variable (int/SInt32...) EDT_x   */
};


struct syllable {
  char *org;
  char *news;
};


struct advantage_dat {
  SInt32 advantage_filter;
  SInt32 disadvantage_filter;
  SInt8 cost;
};


#endif

