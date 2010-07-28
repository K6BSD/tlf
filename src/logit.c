/*
 * Tlf - contest logging program for amateur radio operators
 * Copyright (C) 2001-2002-2003-2004-2005 Rein Couperus <pa0r@amsat.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/* ------------------------------------------------------------------------
 *
 * 		Log routine
 *
 ------------------------------------------------------------------------*/

#include "logit.h"

int logit(void)
{
    extern int use_rxvt;
    extern char mode[];
    extern int trxmode;
    extern char hiscall[];
    extern int cqmode;
    extern int contest;
    extern char buffer[];
    extern char message[15][80];
    extern char ph_message[14][80];
    extern char comment[];
    extern int cqww;
    extern char cqzone[];
    extern char itustr[];
    extern char cq_return[];
    extern char sp_return[];
    extern int cury, curx;
    extern int defer_store;
    extern int stop_backgrnd_process;
    extern int recall_mult;
    extern int simulator;
    extern int simulator_mode;
    extern int lan_mutex;
    extern int ctcomp;
    extern int wazmult;
    extern int itumult;
    extern int qsonum;
    extern int exchange_serial;
    extern char tonestr[];
    extern int dxped;

    int callreturn = 0;
    int qrg_out = 0;
    int time_out = 0;

    strcpy(mode, "Log     ");
    clear_display();
    defer_store = 0;

    stop_backgrnd_process = 0;	/* start it up */

    while (1) {
	printcall();

	if ((callreturn == 0) && (defer_store == 2))
	    callreturn = ' ';
	else
	    callreturn = callinput();

	qrg_out = sendqrg();

	if (qrg_out == 0) {		/* no frequency entered? */

	    if ((trxmode == CWMODE || trxmode == DIGIMODE)
		&& (callreturn == '\n') && ctcomp == 1) {
		callreturn = 92; 	/* '\' */
		strcpy(comment, cqzone);
	    }
	    if ((callreturn == 9 || callreturn == 32)) {
//              if (trxmode == CWMODE){

		callreturn = getexchange();

//              }
//              else if (getexchange() == '\n')
//                      callreturn = 92;
	    }

	    if (callreturn == '\n' && strlen(hiscall) >= 3) {

		if (!strlen(comment) && contest == CONTEST && !ctcomp
		    && !dxped)
		    defer_store = 0;
		if ((cqmode == CQ) && (contest == CONTEST) && (defer_store == 0)) {	/* contest */
		    if (trxmode == CWMODE || trxmode == DIGIMODE)
			strcpy(buffer, message[2]);	/*  send F3  on  ENTER  */
		    else {
			play_file(ph_message[2]);
			if (exchange_serial == 1)
			    mvprintw(13, 29, "Serial number: %d", qsonum);
			refresh();
		    }

		    if (simulator != 0) {
			strcpy(tonestr, "700");
			write_tone();
			if (strstr(hiscall, "?") != NULL)
			    simulator_mode = 3;
			else
			    simulator_mode = 2;
		    }

		    if ((cqww == 1) || (wazmult == 1) || (itumult == 1)) {

			if (recall_exchange() == -1) {
			    if (itumult == 1)
				strcpy(comment, itustr);	/* fill in the ITUzone */
			    else
				strcpy(comment, cqzone);	/* fill in the CQzone */
			}

			if (use_rxvt == 0)
			    attron(COLOR_PAIR(NORMCOLOR) | A_BOLD);
			else
			    attron(COLOR_PAIR(NORMCOLOR));

			mvprintw(12, 54, comment);
		    }

		    if (recall_mult == 1) {
			if (recall_exchange() == -1) {	/* get the power */
			    comment[0] = '\0';

			    if (use_rxvt == 0)
				attron(COLOR_PAIR(NORMCOLOR) | A_BOLD);
			    else
				attron(COLOR_PAIR(NORMCOLOR));

			    mvprintw(12, 54, comment);
			}
		    }

		    sendbuf();
		    defer_store = 1;
		    callreturn = 0;
		}

		if ((cqmode == S_P) && (contest == CONTEST)
		    && (defer_store == 0)) {

		    if (cqww == 1) {
			if (recall_exchange() == -1)
			    strcpy(comment, cqzone);	/* fill in the zone */

			if (use_rxvt == 0)
			    attron(COLOR_PAIR(NORMCOLOR) | A_BOLD);
			else
			    attron(COLOR_PAIR(NORMCOLOR));

			mvprintw(12, 54, comment);
		    } else if (recall_mult == 1) {
			if (recall_exchange() == -1) {	/* get the mult */
			    comment[0] = '\0';

			    if (use_rxvt == 0)
				attron(COLOR_PAIR(NORMCOLOR) | A_BOLD);
			    else
				attron(COLOR_PAIR(NORMCOLOR));

			    mvprintw(12, 54, comment);
			}
		    }

		    if (trxmode == CWMODE || trxmode == DIGIMODE)
			sendspcall();
		    else {
			play_file(ph_message[5]);
			if (exchange_serial == 1)
			    mvprintw(13, 29, "Serial number: %d", qsonum);
			refresh();
		    }
		    callreturn = 0;
		    defer_store = 1;
		}
		if (defer_store == 1) {
		    defer_store++;
		    callreturn = 0;
		} else if (defer_store > 1) {
		    if ((cqmode == CQ) && (contest == CONTEST)) {
			if (trxmode == CWMODE || trxmode == DIGIMODE) {
			    strcat(buffer, cq_return);	/* send cq return */
			    sendbuf();
			    if (simulator != 0)
				simulator_mode = 1;
			    if (simulator != 0)
				write_tone();
			} else {
			    play_file(ph_message[13]);
			}

			defer_store = 0;

		    }

		    if ((cqmode == S_P) && (contest == CONTEST)) {
			if (trxmode == CWMODE || trxmode == DIGIMODE) {
			    strcat(buffer, sp_return);
			    sendbuf();	/* send S&P return */
			} else
			    play_file(ph_message[12]);

			defer_store = 0;
		    }
		    while (lan_mutex == 1) {
			usleep(10000);
			if (time_out++ > 100) {
			    time_out = 0;
			    break;
			}
		    }
		    lan_mutex = 1;
		    log_to_disk();
		    lan_mutex = 0;

		}
		/*  end of else */
	    }
	    /* end of if */
	    if ((callreturn == 92) && (strlen(hiscall) > 0)) {
		defer_store = 0;

		while (lan_mutex == 1) {
		    usleep(10000);
		    if (time_out++ > 100) {
			time_out = 0;
			break;
		    }
		}
		lan_mutex = 1;
		log_to_disk();
		lan_mutex = 0;

	    }

	    if (callreturn == 11 || callreturn == 44 || callreturn == 235) {	/*  CTRL K  */
		getyx(stdscr, cury, curx);
		mvprintw(5, 0, "");
		keyer();
		mvprintw(cury, curx, "");
	    }
	} else {		/* user entered frequency */
	    			/* -> clear input field */
	    hiscall[0] = 0;
	}
    }	/* while(1) */
    return (1);
}
