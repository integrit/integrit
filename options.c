/*
integrit - file integrity verification system
Copyright (C) 2006 Ed L. Cashin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include	<config.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<errno.h>
/* support platforms that don't yet conform to C99 */
#if	HAVE_STDINT_H
#include	<stdint.h>
#elif	HAVE_INTTYPES_H
#include	<inttypes.h>
#else
#error No stdint.h or inttypes.h found.
#endif
#include	"cdb.h"
#include	"cdb_make.h"
#include	"hashtbl.h"
#include	"xstrdup.h"
#include	"checkset.h"
#include	"integrit.h"
#include	"rules.h"
#include	"xml.h"
#include	"elcerror.h"
#include	"elcerror_p.h"
#include	"checkset_p.h"
#include	"options_p.h"
#include	"xml_p.h"
#ifdef		ELC_FIND_LEAKS
#include	"leakfind.h"
#endif

/* There is a thread on comp.lang.c.moderated where the gurus explain
 * how the cast on the parameters to ctype routines should be to
 * unsigned char. */
#define	EATSPACE(buf)	do {				\
    while (*(buf)					\
	   && (*(buf) != '\n')				\
	   && (isspace((unsigned char) *(buf))))	\
      ++(buf);						\
} while (0)    

inline static void remove_first_newline(char *chp)
{
    /* remove trailing newlines from a one-line string */
    for( ; *chp; ++chp)
      if(*chp == '\n')
	*chp = '\0';
}

char *options_output_str(integrit_t *it)
{
    switch (it->output) {
      case OUTPUT_LINES:
	return "human-readable";
	break;
      case OUTPUT_XML:
	return "xml";
	break;
      default:
	DIE("unknown value for output member in options");
	break;
    };
    return "lompa lompa"; /* not reached */
}

static void options_announce_lines(integrit_t *it, FILE *out)
{
    fprintf(out, PROGNAME ": ---- integrit, version %s -----------------\n",
	    INTEGRIT_VERSION);
    fprintf(out, PROGNAME ": %27s : %s\n", "output", options_output_str(it));
    fprintf(out, PROGNAME ": %27s : %s\n", "conf file", it->conffile);
    fprintf(out, PROGNAME ": %27s : %s\n", "known db", it->knowndbname);
    fprintf(out, PROGNAME ": %27s : %s\n", "current db", it->currdbname);
    fprintf(out, PROGNAME ": %27s : %s\n", "root", it->root);
    fprintf(out, PROGNAME ": %27s : %s\n", "do check",
	    it->do_check ? "yes" : "no");
    fprintf(out, PROGNAME ": %27s : %s\n", "do update",
	    it->do_update ? "yes" : "no");
}

static void options_announce_xml(integrit_t *it, FILE *out)
{
    XML_START_PRINT(out, "options");
    XML_ELEMENT_PRINT(out, "output", options_output_str(it));
    XML_ELEMENT_PRINT(out, "conffile", it->conffile);
    XML_ELEMENT_PRINT(out, "knowndb", it->knowndbname);
    XML_ELEMENT_PRINT(out, "currentdb", it->currdbname);
    XML_ELEMENT_PRINT(out, "root", it->root);
    XML_ELEMENT_PRINT(out, "check", it->do_check ? "yes" : "no");
    XML_ELEMENT_PRINT(out, "update", it->do_update ? "yes" : "no");
    XML_END_PRINT(out, "options");
    putc('\n', out);
}

void options_announce(FILE *out, integrit_t *it)
{
    switch (it->output) {
      case OUTPUT_LINES:
	if (it->verbose > 0)
	  options_announce_lines(it, out);
	break;
      case OUTPUT_XML:
	options_announce_xml(it, out);
	break;
      default:
	DIE("unknown value for output member in options");
	break;
    }
}

void options_init(integrit_t *it)
{
    it->conffile	 = NULL;
    it->knowndbname	 = NULL;
    memset(&it->knowndb, 0, sizeof(it->knowndb));
    it->currdbname	 = NULL;
    memset(&it->currdb, 0, sizeof(it->currdb));
    it->root		 = NULL;
    if (! (it->ruleset = malloc(sizeof(hashtbl_t))) )
      DIE("malloc hashtbl");
    if (hashtbl_init(it->ruleset, 20) == -1)
      DIE("initializing hashtbl");
    it->verbose		 = 1;
    it->stop_on_err	 = 1;
    it->do_check	 = 0;
    it->do_update	 = 0;
    it->default_flags	 = RULE_SUM | RULE_INODE | RULE_PERMS | 
            RULE_TYPE | RULE_DEVICETYPE | RULE_NLINK | RULE_UID | RULE_GID | 
            RULE_MTIME | RULE_CTIME;
    it->output		 = OUTPUT_LINES; /* human-readable output */
}

void *call_checkset_free(void *cset, void *data)
{
    checkset_free((checkset *) cset);
    return NULL;
}

void options_destroy(integrit_t *it)
{
    free(it->knowndbname);
    free(it->currdbname);
    free(it->root);
    hashtbl_free(it->ruleset, call_checkset_free, NULL);
    hashtbl_destroy(it->ruleset);
    free(it->ruleset);
}

/* call EATSPACE before calling this */
inline static int blank_or_comment(const char *buf)
{
    switch (*buf) {
      case '\0':		/* empty string counts as blank */
	return 1;
	break;
      case '\n':		/* blank line */
	return 1;
	break;
      case '#':			/* comment */
	return 1;
	break;
      default:
	return 0;
	break;
    }
}

inline static void die_noprop(const char *prop, const char *func)
{
    fprintf(stderr, PROGNAME " (%s) Error: no value for property: %s\n",
	    func, prop);
    exit(INTEGRIT_EXIT_FAILURE);
}

inline static void do_assignment(integrit_t *it, char *buf, char *eq)
{
    char	*property;
    char	*val	 = eq + 1;

    *eq	 = '\0';
    if ( (property = strstr(buf, "known")) ) {
      /* the commandline overrides the config file
       * (really the first one to set it wins and the rest are ignored)
       */
      if (it->knowndbname)
	return;
      EATSPACE(val);
      if (! *val)
	die_noprop(property, __FUNCTION__);
      remove_first_newline(val);
      it->knowndbname	 = xstrdup(val);
    } else if ( (property = strstr(buf, "current")) ) {
      /* the commandline overrides the config file
       * (really the first one to set it wins and the rest are ignored)
       */
      if (it->currdbname)
	return;
      EATSPACE(val);
      if (! *val)
	die_noprop(property, __FUNCTION__);
      remove_first_newline(val);
      it->currdbname	 = xstrdup(val);
    } else if ( (property = strstr(buf, "root")) ) {
      EATSPACE(val);
      if (! *val)
	die_noprop(property, __FUNCTION__);
      remove_first_newline(val);
      it->root	 = xstrdup(val);
    } else if ( (property = strstr(buf, "stop_on_err")) ) {
      EATSPACE(val);
      if (! *val)
	die_noprop(property, __FUNCTION__);
      remove_first_newline(val);
      it->stop_on_err	 = atoi(val);
    } else {
      die(__FUNCTION__, "Error: unknown property: %s", property);
    }
}

static void options_add_checkset(integrit_t *it, char *namebuf,
				 size_t namebuf_chars, checkset *cset)
{
    hashtbl_t	*h	 = it->ruleset;
    checkset	*old;
    void *replaced;

    if ( (old = hashtbl_lookup(h, namebuf, namebuf_chars)) ) {
      /* checkset merge frees the old checkset */
      cset	 = checkset_merge(cset, old, namebuf);
    }

    if (hashtbl_store(h, namebuf, namebuf_chars, cset, &replaced) == -1)
      DIE("storing checkset in table");
}

/* this function is made more complex by the fact that is supports both
 * non-inheriting and (regular) cascading checksets.  The cset pointer
 * is overloaded to point to a non-inheriting checkset (ni_checkset)
 * when necessary.
 */
inline static void do_rule(integrit_t *it, char *buf)
{
    checkset	*cset;
    char	namebuf[BUFSIZ];
    int		namebuf_chars	 = 0;
    int		n_switches;
    char	ignore		 = 0;
    char	nochildren	 = 0;
    char	noinherit	 = 0;

    EATSPACE(buf);
    switch (*buf) {
      case '!':
	ignore		 = 1;
	++buf;
	break;
      case '=':
	nochildren	 = 1;
	++buf;
	break;
      case '$':
	noinherit	 = 1;
	++buf;
	break;
      default:
	break;	
    }

    EATSPACE(buf);
    for ( ; *buf && (*buf != '\n'); ++buf) {
      if (namebuf_chars == (BUFSIZ - 3)) /* space for two new chars,
					  * plus null char */
	die(__FUNCTION__, "Error: filename too long in config");

      if (*buf == '\\') {
	if (isspace((unsigned char) *(buf + 1)))
	  namebuf[namebuf_chars++]	 = *(++buf);
	else
	  namebuf[namebuf_chars++]	 = '\\';
      } else if (isspace((unsigned char) *buf)) {
	break;			/* end of the name */
      } else {
	namebuf[namebuf_chars++]	 = *buf;
      }
    }
    namebuf[namebuf_chars]	 = '\0';

    EATSPACE(buf);
    n_switches	 = strlen(buf);
    /* trim trailing whitespace */
    while (n_switches && isspace((unsigned char) buf[n_switches - 1])) {
      buf[n_switches - 1]	 = '\0';
      --n_switches;
    }
    if (strspn(buf, "SsIiPpTtDdLlUuGgZzAaMmCcRr") != n_switches)
      die(__FUNCTION__,
	  "Error: unrecognized check switch in conf file rule for %s",
	  namebuf);

    if (noinherit)
      cset	 = (checkset *) ni_checkset_new();
    else
      cset	 = checkset_new();

    CHECKSET_SETBOOLEAN(cset, RULE_IGNORE, ignore);
    CHECKSET_SETBOOLEAN(cset, RULE_NOCHILDREN, nochildren);
    CHECKSET_SETBOOLEAN(cset, RULE_NOINHERIT, noinherit);

    if (noinherit)
      ((ni_checkset *) cset)->ni_switches	 = xstrdup(buf);
    else
      cset->switches	 = xstrdup(buf);
    
    remove_first_newline(namebuf);
#ifdef	DEBUG
    fprintf(stderr, "debug: path (%s) checkset (", namebuf);
    checkset_show(stderr, cset);
    fputs(")\n", stderr);
#endif
    
    options_add_checkset(it, namebuf, namebuf_chars, cset);
}

static void usage(void)
{
    fputs("Usage:\n"
	  "\n"
	  "      integrit -C conffile [options]\n"
	  "      integrit -V\n"
	  "      integrit -h\n"
	  "\n"
	  "Options:\n"
	  "\n"
	  "      -C	specify configuration file\n"
	  "      -x	use XML output instead of abbreviated output\n"
	  "      -u	do update: create current state database\n"
	  "      -c	do check: verify current state against known db\n"
	  "      -q	lower verbosity\n"
	  "      -v	raise verbosity\n"
	  "      -N	specify the current (New) database, overriding conf file\n"
	  "      -O	specify the known (Old) database, overriding conf file\n"
	  "      -V	show integrit version info and exit\n"
	  "      -h	show this help      \n\n", stderr);
}

void parse_args(integrit_t *it, int argc, char *argv[])
{
    int		c;
    opterr	 = 0;

    while ( (c = getopt(argc, argv, "hVvqC:cuxN:O:")) != -1) {
      switch (c) {
	case 'h':
	  usage();
	  exit(EXIT_SUCCESS);
	  break;
	case 'V':
	  puts(PROGNAME " version " INTEGRIT_VERSION);
	  exit(EXIT_SUCCESS);
	  break;
	case 'v':
	  ++it->verbose;
	  break;
	case 'q':
	  --(it->verbose);
	  break;
	case 'C':
	  it->conffile	 = optarg;
	  break;
	case 'c':
	  it->do_check	 = 1;
	  break;
	case 'N':
	  it->currdbname = xstrdup(optarg);
	  break;
	case 'O':
	  it->knowndbname = xstrdup(optarg);
	  break;
	case 'u':
	  it->do_update	 = 1;
	  break;
	case 'x':
	  it->output	 = OUTPUT_XML;
	  break;
	case '?':
	  if (isprint(optopt))
	    warn(__FUNCTION__, "Error: unknown option `-%c'.\n", optopt);
	  else
	    warn(__FUNCTION__,
		"Error: unknown option character `\\x%x'.\n", optopt);
	  usage();
	  exit(INTEGRIT_EXIT_FAILURE);
	  break;
	default:
	  abort();		/* this shouldn't happen */
	  break;
      }	/* end switch */
    } /* end while getopt */

    if (! it->conffile)
      die(__FUNCTION__, "Error: no conffile on command line");
}

void options_set(integrit_t *it, int argc, char *argv[])
{
    char	buf[BUFSIZ];
    char	*cp;
    char	*equalsign;
    FILE	*conf;

    parse_args(it, argc, argv);
    if (! (conf = fopen(it->conffile, "r")) )
      DIE("opening conf file");

    while (fgets(buf, BUFSIZ, conf)) {
      cp	 = buf;
      EATSPACE(cp);
      if (blank_or_comment(cp))
	continue;
      else if (*cp == '=')
	do_rule(it, cp);
      else if ( (equalsign = strchr(cp, '=')) )
	do_assignment(it, cp, equalsign);
      else
	do_rule(it, cp);
    }
    if (! it->knowndbname)
      die(__FUNCTION__, "Error: known database unspecified");
    if (! it->currdbname)
      die(__FUNCTION__, "Error: current database unspecified");
    if (! it->root)
      die(__FUNCTION__, "Error: root search directory unspecified");
}
