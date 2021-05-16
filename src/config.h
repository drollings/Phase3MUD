/* ************************************************************************
*  File: config.c                                       Part of CircleMUD *
*                                                                         *
*  Usage: Global vars for configuration stuff                             *
*                                                                         *
*  $Author: sfn $
*  $Date: 2001/02/04 15:57:57 $
*  $Revision: 1.3 $
************************************************************************ */

#ifndef _CONFIG_H_
#define _CONFIG_H_

extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;

extern int	DFLT_PORT;
extern char	*DFLT_DIR;
extern int	MAX_PLAYERS;
extern int	max_filesize;
extern int	max_bad_pws;
extern SInt8 name_services;

extern int	use_autowiz;
extern int	min_wizlist_lev;

extern int	max_exp_gain;
extern int	max_exp_loss;

extern int	death_ban_min_level;
extern int	death_ban_high_level;
extern int	death_ban_mid_time;
extern int	death_ban_high_time;

extern int	sleep_allowed;
extern int	roomaffect_allowed;
extern int	pk_allowed;
extern int	pt_allowed;

#endif

