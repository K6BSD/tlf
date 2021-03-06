/*
 * Tlf - contest logging program for amateur radio operators
 * Copyright (C) 2001-2002-2003 Rein Couperus <pa0rct@amsat.org>
 *               2012-2013           Thomas Beierlein <tb@forth-ev.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* ------------------------------------------------------------
 *   write cabrillo  file
 *
 *--------------------------------------------------------------*/


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "getsummary.h"
#include "tlf_curses.h"
#include "ui_utils.h"
#include "cabrillo_utils.h"

extern char call[];

/* conversion table between tag name in format file and internal tag */
extern struct tag_conv tag_tbl[];

int is_comment(char *buffer);
struct qso_t *get_next_record(FILE *fp);
struct qso_t *get_next_qtc_record(FILE *fp, int qtcdirection);
void free_qso(struct qso_t *ptr);

/** check if logline is only a comment */
int is_comment(char *buf) {

    if (buf[0] != ';' && strlen(buf) > 60) /** \todo better check */
	return 0;
    else
	return 1;
}

/** get next qso record from log
 *
 * Read next line from logfile until it is no comment.
 * Then parse the logline into a new allocated QSO data structure
 * and return that structure.
 *
 * \return ptr to new qso record (or NULL if eof)
 */
struct qso_t *get_next_record(FILE *fp) {

    char buffer[160];
    char *tmp;
    char *sp;
    struct qso_t *ptr;
    struct tm date_n_time;

    while ((fgets(buffer, sizeof(buffer), fp)) != NULL) {

	if (!is_comment(buffer)) {

	    ptr = g_malloc0(sizeof(struct qso_t));

	    /* remember whole line */
	    ptr->logline = g_strdup(buffer);
	    ptr->qtcdirection = 0;
	    ptr->qsots = 0;

	    /* split buffer into parts for qso_t record and parse
	     * them accordingly */
	    tmp = strtok_r(buffer, " \t", &sp);

	    /* band */
	    ptr->band = atoi(tmp);


	    /* mode */
	    if (strcasestr(tmp, "CW"))
		ptr->mode = CWMODE;
	    else if (strcasestr(tmp, "SSB"))
		ptr->mode = SSBMODE;
	    else
		ptr->mode = DIGIMODE;

	    /* date & time */
	    memset(&date_n_time, 0, sizeof(struct tm));

	    strptime(strtok_r(NULL, " \t", &sp), "%d-%b-%y", &date_n_time);
	    strptime(strtok_r(NULL, " \t", &sp), "%H:%M", &date_n_time);

	    ptr->year = date_n_time.tm_year + 1900;	/* convert to
							   1968..2067 */
	    ptr->month = date_n_time.tm_mon + 1;	/* tm_mon = 0..11 */
	    ptr->day   = date_n_time.tm_mday;

	    ptr->hour  = date_n_time.tm_hour;
	    ptr->min   = date_n_time.tm_min;

	    /* qso number */
	    ptr->qso_nr = atoi(strtok_r(NULL, " \t", &sp));

	    /* his call */
	    ptr->call = g_strdup(strtok_r(NULL, " \t", &sp));

	    /* RST send and received */
	    ptr->rst_s = atoi(strtok_r(NULL, " \t", &sp));
	    ptr->rst_r = atoi(strtok_r(NULL, " \t", &sp));

	    /* comment (exchange) */
	    ptr->comment = g_strndup(buffer + 54, 13);

	    /* tx */
	    ptr->tx = (buffer[79] == '*') ? 1 : 0;

	    /* frequency */
	    ptr->freq = atof(buffer + 80);
	    if ((ptr->freq < 1800.) || (ptr->freq >= 30000.)) {
		ptr->freq = 0.;
	    }

	    return ptr;
	}
    }

    return NULL;
}

/** get next qtc record from log
 *
 * Read next line from logfile.
 * Then parse the received or sent qtc logline into a new allocated received QTC data structure
 * and return that structure.
 *
 * \return ptr to new qtc record (or NULL if eof)
 */
struct qso_t *get_next_qtc_record(FILE *fp, int qtcdirection) {

    char buffer[100];
    char *tmp;
    char *sp;
    struct qso_t *ptr;
    int pos, shift;
    struct tm date_n_time;

    if (fp == NULL) {
	return NULL;
    }

    while ((fgets(buffer, sizeof(buffer), fp)) != NULL) {


	ptr = g_malloc0(sizeof(struct qso_t));

	/* remember whole line */
	ptr->logline = g_strdup(buffer);
	ptr->qtcdirection = qtcdirection;

	/* tx */
	if (qtcdirection == RECV) {
	    pos = 28;
	    shift = 0;
	} else {
	    pos = 33;
	    shift = 5;
	}
	ptr->tx = (buffer[pos] == ' ') ? 0 : 1;

	/* split buffer into parts for qso_t record and parse
	  * them accordingly */
	tmp = strtok_r(buffer, " \t", &sp);

	/* band */
	ptr->band = atoi(tmp);

	/* mode */
	if (strcasestr(tmp, "CW"))
	    ptr->mode = CWMODE;
	else if (strcasestr(tmp, "SSB"))
	    ptr->mode = SSBMODE;
	else
	    ptr->mode = DIGIMODE;

	/* qso number */
	ptr->qso_nr = atoi(strtok_r(NULL, " \t", &sp));

	/* in case of SEND direction, the 3rd field is the original number of sent QSO,
	   but it doesn't need for QTC line */
	if (qtcdirection & SEND) {
	    tmp = strtok_r(NULL, " \t", &sp);
	}
	/* date & time */
	memset(&date_n_time, 0, sizeof(struct tm));

	strptime(strtok_r(NULL, " \t", &sp), "%d-%b-%y", &date_n_time);
	strptime(strtok_r(NULL, " \t", &sp), "%H:%M", &date_n_time);

	ptr->qsots = timegm(&date_n_time);

	ptr->year = date_n_time.tm_year + 1900;	/* convert to
							1968..2067 */
	ptr->month = date_n_time.tm_mon + 1;	/* tm_mon = 0..11 */
	ptr->day   = date_n_time.tm_mday;

	ptr->hour  = date_n_time.tm_hour;
	ptr->min   = date_n_time.tm_min;

	if (ptr->tx == 1) {
	    /* ignore TX if set */
	    strtok_r(NULL, " \t", &sp);
	}
	/* his call */
	ptr->call = g_strdup(strtok_r(NULL, " \t", &sp));

	/* QTC serial and number */
	ptr->qtc_serial = atoi(strtok_r(NULL, " \t", &sp));
	ptr->qtc_number = atoi(strtok_r(NULL, " \t", &sp));

	ptr->qtc_qtime = g_strdup(strtok_r(NULL, " \t", &sp));
	ptr->qtc_qcall = g_strdup(strtok_r(NULL, " \t", &sp));
	ptr->qtc_qserial = g_strdup(strtok_r(NULL, " \t", &sp));

	/* frequency */
	ptr->freq = atof(buffer + 80 + shift);
	if ((ptr->freq < 1800.) || (ptr->freq >= 30000.)) {
	    ptr->freq = 0.;
	}

	return ptr;
    }

    return NULL;
}

/** free qso record pointed to by ptr */
void free_qso(struct qso_t *ptr) {

    if (ptr != NULL) {
	g_free(ptr->comment);
	g_free(ptr->logline);
	g_free(ptr->call);
	if (ptr->qtc_qtime != NULL) {
	    g_free(ptr->qtc_qtime);
	    g_free(ptr->qtc_qcall);
	    g_free(ptr->qtc_qserial);
	}
	g_free(ptr);
    }
}

/** write out information */
void info(char *s) {
    attron(modify_attr(COLOR_PAIR(C_INPUT) | A_STANDOUT));
    mvprintw(13, 29, "%s", s);
    refreshp();
}


char *to_mode[] = {
    "CW",
    "PH",
    "RY"
};

/* converts band to frequency of start of band */
float band2freq(int band) {
    float freq;

    switch (band) {
	case 160:
	    freq = 1800.;
	    break;
	case 80:
	    freq = 3500.;
	    break;
	case 40:
	    freq = 7000.;
	    break;
	case 30:
	    freq = 10100.;
	    break;
	case 20:
	    freq = 14000.;
	    break;
	case 17:
	    freq = 18068;
	    break;
	case 15:
	    freq = 21000.;
	    break;
	case 12:
	    freq = 24890;
	    break;
	case 10:
	    freq = 28000.;
	    break;
	default:
	    freq = 0.;
	    break;
    }

    return freq;
}

/* add 'src' to 'dst' with max. 'len' chars left padded */
void add_lpadded(char *dst, char *src, int len) {
    char *field;
    int l;

    field = g_malloc(len + 1);
    strcat(dst, " ");
    memset(field, ' ', len);
    l = strlen(src);
    if (l > len) l = len;
    memcpy(field + len - l, src, l);
    field[len] = '\0';
    strcat(dst, field);
    g_free(field);
}

/* add 'src' to 'dst' with max. 'len' char right padded */
void add_rpadded(char *dst, char *src, int len) {
    char *field;
    int l;

    field = g_malloc(len + 1);
    strcat(dst, " ");
    memset(field, ' ', len);
    l = strlen(src);
    if (l > len) l = len;
    memcpy(field, src, l);
    field[len] = '\0';
    strcat(dst, field);
    g_free(field);
}

/* get the n-th token of a string, return empty string if no n-th token */
gchar *get_nth_token(gchar *str, int n) {
    gchar *string = g_strdup(str);
    gchar *ptr;
    char *sp;

    ptr = strtok_r(string, " \t", &sp);

    while (n > 0 && ptr != NULL) {
	ptr = strtok_r(NULL, " \t", &sp);
	n--;
    }

    /* if no n-th element in string, return empty string */
    if (ptr == NULL)
	ptr = strdup("");
    else
	ptr = strdup(ptr);

    g_free(string);
    return ptr;
}


/* format QSO: line for actual qso according to cabrillo format description
 * and put it into buffer */
void prepare_line(struct qso_t *qso, struct cabrillo_desc *desc, char *buf) {

    extern char exchange[];

    int freq;
    int i;
    char tmp[80];
    struct line_item *item;
    gchar *token;
    int item_count;
    GPtrArray *item_array;

    if (qso == NULL) {
	strcpy(buf, "");
	return;
    }

    freq = (int)qso->freq;
    if (freq < 1800.)
	freq = (int)band2freq(qso->band);

    if (qso->qtcdirection == 0) {
	strcpy(buf, "QSO:");		/* start the line */
	item_count = desc->item_count;
	item_array = desc->item_array;
    } else {
	strcpy(buf, "QTC:");		/* start the line */
	item_count = desc->qtc_item_count;
	item_array = desc->qtc_item_array;
    }
    for (i = 0; i < item_count; i++) {
	item = g_ptr_array_index(item_array, i);
	switch (item->tag) {
	    case FREQ:
		sprintf(tmp, "%d", freq);
		add_lpadded(buf, tmp, item->len);
		break;
	    case MODE:
		sprintf(tmp, "%s", to_mode[qso->mode]);
		add_lpadded(buf, tmp, item->len);
		break;
	    case DATE:
		sprintf(tmp, "%4d-%02d-%02d",
			qso->year, qso->month, qso->day);
		add_lpadded(buf, tmp, item->len);
		break;
	    case TIME:
		sprintf(tmp, "%02d%02d", qso->hour, qso->min);
		add_lpadded(buf, tmp, item->len);
		break;
	    case MYCALL:
		strcpy(tmp, call);
		add_rpadded(buf, g_strchomp(tmp), item->len);
		break;
	    case HISCALL:
		add_rpadded(buf, qso->call, item->len);
		break;
	    case RST_S:
		sprintf(tmp, "%d", qso->rst_s);
		add_rpadded(buf, tmp, item->len);
		break;
	    case RST_R:
		sprintf(tmp, "%d", qso->rst_r);
		add_rpadded(buf, tmp, item->len);
		break;
	    case EXCH:
		add_rpadded(buf, qso->comment, item->len);
		break;
	    case EXC1:
		token = get_nth_token(qso->comment, 0);
		add_rpadded(buf, token, item->len);
		g_free(token);
		break;
	    case EXC2:
		token = get_nth_token(qso->comment, 1);
		add_rpadded(buf, token, item->len);
		g_free(token);
		break;
	    case EXC3:
		token = get_nth_token(qso->comment, 2);
		add_rpadded(buf, token, item->len);
		g_free(token);
		break;
	    case EXC4:
		token = get_nth_token(qso->comment, 3);
		add_rpadded(buf, token, item->len);
		g_free(token);
		break;
	    case EXC_S: {
		int pos;
		char *start = exchange;
		tmp[0] = '\0';
		pos = strcspn(start, "#");
		strncat(tmp, start, pos);   /** \todo avoid buffer overflow */
		while (pos < strlen(start)) {
		    if (start[pos] == '#') {
			/* format and add serial number */
			char number[6];
			sprintf(number, "%04d", qso->qso_nr);
			strcat(tmp, number);
		    }

		    start = start + pos + 1; 	/* skip special character */
		    pos = strcspn(start, "#");
		    strncat(tmp, start, pos);
		}
		add_rpadded(buf, tmp, item->len);
	    }
	    break;
	    case TX:
		sprintf(tmp, "%1d", qso->tx);
		add_rpadded(buf, tmp, item->len);
		break;
	    case QTCRCALL:
		if (qso->qtcdirection == 1) {	// RECV
		    strcpy(tmp, call);
		}
		if (qso->qtcdirection == 2) {	// SEND
		    strcpy(tmp, qso->call);
		}
		add_rpadded(buf, g_strchomp(tmp), item->len);
		break;
	    case QTCHEAD:
		tmp[0] = '\0';
		sprintf(tmp, "%*d/%d", 3, qso->qtc_serial, qso->qtc_number);
		add_rpadded(buf, g_strchomp(tmp), item->len);
		break;
	    case QTCSCALL:
		if (qso->qtcdirection == 1) {	// RECV
		    strcpy(tmp, qso->call);
		}
		if (qso->qtcdirection == 2) {	// SEND
		    strcpy(tmp, call);
		}
		add_rpadded(buf, g_strchomp(tmp), item->len);
		break;
	    case QTC:
		sprintf(tmp, "%s %-13s %4s", qso->qtc_qtime, qso->qtc_qcall, qso->qtc_qserial);
		add_rpadded(buf, g_strchomp(tmp), item->len);
	    case NO_ITEM:
	    default:
		tmp[0] = '\0';
	}

    }
    strcat(buf, "\n"); 		/* closing nl */
}

int write_cabrillo(void) {

    extern char *cabrillo;
    extern char logfile[];
    extern char exchange[];
    extern char call[];

    char *cab_dfltfile;
    struct cabrillo_desc *cabdesc;
    char cabrillo_tmp_name[80];
    char buffer[4000] = "";

    FILE *fp1, *fp2, *fpqtcrec = NULL, *fpqtcsent = NULL;
    struct qso_t *qso, *qtcrec = NULL, *qtcsent = NULL;
    int qsonr, qtcrecnr, qtcsentnr;

    if (cabrillo == NULL) {
	info("Missing CABRILLO= keyword (see man page)");
	sleep(2);
	return (1);
    }

    /* Try to read cabrillo format first from local directory.
     * Try also in default data dir if not found.
     */
    cabdesc = read_cabrillo_format("cabrillo.fmt", cabrillo);
    if (!cabdesc) {
	cab_dfltfile = g_strconcat(PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S,
				   "cabrillo.fmt", NULL);
	cabdesc = read_cabrillo_format(cab_dfltfile, cabrillo);
	g_free(cab_dfltfile);
    }

    if (!cabdesc) {
	info("Cabrillo format specification not found!");
	sleep(2);
	return (2);
    }

    /* open logfile and create a cabrillo file */
    strcpy(cabrillo_tmp_name, call);
    g_strstrip(cabrillo_tmp_name); /* drop \n */
    strcat(cabrillo_tmp_name, ".cbr");

    if ((fp1 = fopen(logfile, "r")) == NULL) {
	info("Can't open logfile.");
	sleep(2);
	free_cabfmt(cabdesc);
	return (1);
    }
    if (cabdesc->qtc_item_array != NULL) {
	if (qtcdirection & 1) {
	    fpqtcrec = fopen(QTC_RECV_LOG, "r");
	    if (fpqtcrec == NULL) {
		info("Can't open received QTC logfile.");
		sleep(2);
		free_cabfmt(cabdesc);
		fclose(fp1);
		return (1);
	    }
	}
	if (qtcdirection & 2) {
	    fpqtcsent = fopen(QTC_SENT_LOG, "r");
	    if (fpqtcsent == NULL) {
		info("Can't open sent QTC logfile.");
		sleep(2);
		free_cabfmt(cabdesc);
		fclose(fp1);
		if (fpqtcrec != NULL) fclose(fpqtcrec);
		return (1);
	    }
	}
    }
    if ((fp2 = fopen(cabrillo_tmp_name, "w")) == NULL) {
	info("Can't create cabrillo file.");
	sleep(2);
	free_cabfmt(cabdesc);
	fclose(fp1);
	if (fpqtcsent != NULL) fclose(fpqtcsent);
	if (fpqtcrec != NULL) fclose(fpqtcrec);
	return (2);
    }


    /* ask for exchange and header information */
    ask(buffer,
	"Your exchange (e.g. State, province, age etc... (# if serial number)): ");
    strncpy(exchange, buffer, 10);
    getsummary(fp2);

    info("Writing cabrillo file");

    qsonr = 0;
    qtcrecnr = 0;
    qtcsentnr = 0;
    while ((qso = get_next_record(fp1))) {

	qsonr++;
	prepare_line(qso, cabdesc, buffer);

	if (strlen(buffer) > 5) {
	    fputs(buffer, fp2);
	}
	if (fpqtcrec != NULL && qtcrec == NULL) {
	    qtcrec = get_next_qtc_record(fpqtcrec, RECV);
	    if (qtcrec != NULL) {
		qtcrecnr = qtcrec->qso_nr;
	    }
	}
	if (fpqtcsent != NULL && qtcsent == NULL) {
	    qtcsent = get_next_qtc_record(fpqtcsent, SEND);
	    if (qtcsent != NULL) {
		qtcsentnr = qtcsent->qso_nr;
	    }
	}
	while (qtcrecnr == qsonr || qtcsentnr == qsonr) {
	    if (qtcsent == NULL || (qtcrec != NULL && qtcrec->qsots < qtcsent->qsots)) {
		prepare_line(qtcrec, cabdesc, buffer);
		if (strlen(buffer) > 5) {
		    fputs(buffer, fp2);
		    free_qso(qtcrec);
		}
		qtcrec = get_next_qtc_record(fpqtcrec, RECV);
		if (qtcrec != NULL) {
		    qtcrecnr = qtcrec->qso_nr;
		} else {
		    qtcrecnr = 0;
		}
	    } else {
		prepare_line(qtcsent, cabdesc, buffer);
		if (strlen(buffer) > 5) {
		    fputs(buffer, fp2);
		    free_qso(qtcsent);
		}
		qtcsent = get_next_qtc_record(fpqtcsent, SEND);
		if (qtcsent != NULL) {
		    qtcsentnr = qtcsent->qso_nr;
		} else {
		    qtcsentnr = 0;
		}
	    }
	}

	free_qso(qso);
    }

    fclose(fp1);

    fputs("END-OF-LOG:\n", fp2);
    fclose(fp2);
    if (fpqtcrec != NULL) {
	fclose(fpqtcrec);
    }

    free_cabfmt(cabdesc);

    return 0;
}



/*
    The ADIF function has been written according ADIF v1.00 specifications
    as shown on http://www.adif.org
    LZ3NY
*/
int write_adif(void) {

    extern char logfile[];
    extern char exchange[];
    extern char whichcontest[];
    extern int exchange_serial;
    extern char modem_mode[];

    char buf[181] = "";
    char buffer[181] = "";
    char standardexchange[70] = "";
    char adif_tmp_name[40] = "";
    char adif_tmp_call[13] = "";
    char adif_tmp_str[2] = "";
    char adif_year_check[3] = "";
    char adif_rcvd_num[16] = "";
    char resultat[16];
    char adif_tmp_rr[5] = "";
    double freq;
    char freq_buf[16];

    int adif_mode_dep = 0;

    FILE *fp1, *fp2;

    if ((fp1 = fopen(logfile, "r")) == NULL) {
	info("Opening logfile not possible.");
	sleep(2);
	return (1);
    }
    strcpy(adif_tmp_name, whichcontest);
    strcat(adif_tmp_name, ".adi");

    if ((fp2 = fopen(adif_tmp_name, "w")) == NULL) {
	info("Opening ADIF file not possible.");
	sleep(2);
	fclose(fp1);		//added by F8CFE
	return (2);
    }

    if (strlen(exchange) > 0)
	strcpy(standardexchange, exchange);

    /* in case using write_adif() without write_cabrillo() before
     * just ask for the needed information */
    if ((strlen(standardexchange) == 0) && (exchange_serial != 1)) {
	ask(buffer,
	    "Your exchange (e.g. State, province, age etc... (# if serial number)): ");
	strncpy(standardexchange, buffer, 10);
    }

    info("Writing ADIF file");

    /* write header */
    fputs
    ("################################################################################\n",
     fp2);
    fputs
    ("#                     ADIF v1.00 data file exported by TLF\n",
     fp2);
    fputs
    ("#              according to specifications on http://www.adif.org\n",
     fp2);
    fputs("#\n", fp2);
    fputs
    ("################################################################################\n",
     fp2);
    fputs("<adif_ver:4>1.00\n<eoh>\n", fp2);

    while (fgets(buf, sizeof(buf), fp1)) {

	buffer[0] = '\0';

	if ((buf[0] != ';') && ((buf[0] != ' ') || (buf[1] != ' '))
		&& (buf[0] != '#') && (buf[0] != '\n') && (buf[0] != '\r')) {

	    /* CALLSIGN */
	    strcat(buffer, "<CALL:");
	    strncpy(adif_tmp_call, buf + 29, 12);
	    strcpy(adif_tmp_call, g_strstrip(adif_tmp_call));
	    snprintf(resultat, sizeof(resultat), "%zd",
		     strlen(adif_tmp_call));
	    strcat(buffer, resultat);
	    strcat(buffer, ">");
	    strcat(buffer, adif_tmp_call);

	    /* BAND */
	    if (buf[1] == '6')
		strcat(buffer, "<BAND:4>160M");
	    else if (buf[1] == '8')
		strcat(buffer, "<BAND:3>80M");
	    else if (buf[1] == '4')
		strcat(buffer, "<BAND:3>40M");
	    else if (buf[1] == '3')
		strcat(buffer, "<BAND:3>30M");
	    else if (buf[1] == '2')
		strcat(buffer, "<BAND:3>20M");
	    else if (buf[1] == '1' && buf[2] == '5')
		strcat(buffer, "<BAND:3>15M");
	    else if (buf[1] == '1' && buf[2] == '7')
		strcat(buffer, "<BAND:3>17M");
	    else if (buf[1] == '1' && buf[2] == '2')
		strcat(buffer, "<BAND:3>12M");
	    else if (buf[1] == '1' && buf[2] == '0')
		strcat(buffer, "<BAND:3>10M");

	    /* FREQ if available */
	    if (strlen(buf) > 81) {
		freq = atof(buf + 80);
		freq_buf[0] = '\0';
		if ((freq > 1799.) && (freq < 10000.)) {
		    sprintf(freq_buf, "<FREQ:6>%.4f", freq / 1000.);
		} else if (freq >= 10000.) {
		    sprintf(freq_buf, "<FREQ:7>%.4f", freq / 1000.);
		}
		strcat(buffer, freq_buf);
	    }

	    /* QSO MODE */
	    if (buf[3] == 'C')
		strcat(buffer, "<MODE:2>CW");
	    else if (buf[3] == 'S')
		strcat(buffer, "<MODE:3>SSB");
	    else if (strcmp(modem_mode, "RTTY") == 0)
		strcat(buffer, "<MODE:4>RTTY");
	    else
		/* \todo DIGI is no allowed mode */
		strcat(buffer, "<MODE:4>DIGI");

	    /* QSO_DATE */
	    /* Y2K :) */
	    adif_year_check[0] = '\0';
	    strncpy(adif_year_check, buf + 14, 2);
	    if (atoi(adif_year_check) <= 70)
		strcat(buffer, "<QSO_DATE:8>20");
	    else
		strcat(buffer, "<QSO_DATE:8>19");

	    /* year */
	    strncat(buffer, buf + 14, 2);

	    /*month */
	    if (buf[10] == 'J' && buf[11] == 'a')
		strcat(buffer, "01");
	    if (buf[10] == 'F')
		strcat(buffer, "02");
	    if (buf[10] == 'M' && buf[12] == 'r')
		strcat(buffer, "03");
	    if (buf[10] == 'A' && buf[12] == 'r')
		strcat(buffer, "04");
	    if (buf[10] == 'M' && buf[12] == 'y')
		strcat(buffer, "05");
	    if (buf[10] == 'J' && buf[11] == 'u' && buf[12] == 'n')
		strcat(buffer, "06");
	    if (buf[10] == 'J' && buf[12] == 'l')
		strcat(buffer, "07");
	    if (buf[10] == 'A' && buf[12] == 'g')
		strcat(buffer, "08");
	    if (buf[10] == 'S')
		strcat(buffer, "09");
	    if (buf[10] == 'O')
		strcat(buffer, "10");
	    if (buf[10] == 'N')
		strcat(buffer, "11");
	    if (buf[10] == 'D')
		strcat(buffer, "12");

	    /*date */
	    strncat(buffer, buf + 7, 2);

	    /* TIME_ON */
	    strcat(buffer, "<TIME_ON:4>");
	    strncat(buffer, buf + 17, 2);
	    strncat(buffer, buf + 20, 2);

	    /* RS(T) flag */
	    if (buf[3] == 'S')		/* check for SSB */
		adif_mode_dep = 2;
	    else
		adif_mode_dep = 3;

	    /* RST_SENT */
	    strcat(buffer, "<RST_SENT:");
	    adif_tmp_str[1] = '\0';	/*       PA0R 02/10/2003  */
	    adif_tmp_str[0] = adif_mode_dep + 48;
	    strcat(buffer, adif_tmp_str);
	    strcat(buffer, ">");
	    strncat(buffer, buf + 44, adif_mode_dep);

	    /* STX - sent contest number */
	    strcat(buffer, "<STX:");

	    if ((exchange_serial == 1) || (standardexchange[0] == '#')) {
		strcat(buffer, "4>");
		strncat(buffer, buf + 23, 4);
	    } else {
		snprintf(resultat, sizeof(resultat), "%zd",
			 strlen(standardexchange));
		strcat(buffer, resultat);
		strcat(buffer, ">");
		strcat(buffer, g_strstrip(standardexchange));
	    }

	    /* RST_RCVD */
	    strncpy(adif_tmp_rr, buf + 49, 4);
	    strcpy(adif_tmp_rr, g_strstrip(adif_tmp_rr));
	    strcat(buffer, "<RST_RCVD:");
	    snprintf(resultat, sizeof(resultat), "%zd",
		     strlen(adif_tmp_rr));
	    strcat(buffer, resultat);
	    strcat(buffer, ">");
	    strncat(buffer, buf + 49, adif_mode_dep);

	    /* SRX - received contest number */
	    strncpy(adif_rcvd_num, buf + 54, 14);
	    strcpy(adif_rcvd_num, g_strstrip(adif_rcvd_num));
	    snprintf(resultat, sizeof(resultat), "%zd",
		     strlen(adif_rcvd_num));
	    strcat(buffer, "<SRX:");
	    strcat(buffer, resultat);
	    strcat(buffer, ">");
	    if (strcmp(buf + 54, " ") != 0)
		strcat(buffer, adif_rcvd_num);

	    /* <EOR> */
	    strcat(buffer, "<eor>\n");	//end of ADIF row

	    fputs(buffer, fp2);
	}
    }				// end fgets() loop

    fclose(fp1);
    fclose(fp2);

    return (0);
}				// end write_adif
