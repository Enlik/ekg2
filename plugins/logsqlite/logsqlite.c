/* $Id$ */

/*
 *  (C) Copyright 2005 Leszek Krupi�ski <leafnode@wafel.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License Version
 *  2.1 as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdint.h>
#include <stdlib.h>

#include "ekg2-config.h"

#include <ekg/debug.h>
#include <ekg/dynstuff.h>
#include <ekg/log.h>
#include <ekg/plugins.h>
#include <ekg/protocol.h>
#include <ekg/sessions.h>
#include <ekg/stuff.h>
#include <ekg/themes.h>
#include <ekg/vars.h>
#include <ekg/windows.h>
#include <ekg/userlist.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <string.h>

#include <sqlite.h>

#include "logsqlite.h"


PLUGIN_DEFINE(logsqlite, PLUGIN_LOG, logsqlite_theme_init);


/*
 * last log
 */
COMMAND(logsqlite_cmd_last)
{
	sqlite * db = logsqlite_open_db();
	char sql[100];
	char buf[100];
	const char ** results;
	const char ** fields;
	sqlite_vm * vm;
	struct tm *tm;
	char * errors;
	time_t ts;
	int count;
	int count2 = 0;
	char * gotten_uid;
	char * nick = NULL;

	if (params[0]) {
		nick = xstrdup(params[0]);
		nick = strip_quotes(nick);
		if (!session) {
			if (session_current) {
				session = session_current;
			} else {
				return -1;
			}
		}
		gotten_uid = get_uid(session, nick);
		if (! gotten_uid) {
			gotten_uid = nick;
		}
		sprintf(sql, "select uid, nick, ts, body, sent from log_msg where uid = '%s' order by ts desc limit %i", gotten_uid, config_logsqlite_last_limit);
	} else {
		sprintf(sql, "select uid, nick, ts, body, sent from log_msg order by ts desc limit %i", config_logsqlite_last_limit);
	}
	sqlite_compile(db, sql, NULL, &vm, &errors);
	while (sqlite_step(vm, &count, &results, &fields) == SQLITE_ROW) {
		count2++;
		ts = (time_t) atoi(results[2]);
		tm = localtime(&ts);
		strftime(buf, sizeof(buf), format_find("last_list_timestamp"), tm);
		if (!xstrcmp(results[4], "0")) {
			printq("last_list_in", buf, results[1], results[3]);
		} else {
			printq("last_list_out", buf, results[1], results[3]);
		}
	}
	if (count2 == 0) {
		if (nick) {
			printq("last_list_empty_nick", nick);
		} else {
			printq("last_list_empty");
		}
	}
	sqlite_finalize(vm, &errors);
	logsqlite_close_db(db);
	return 0;
}

/*
 * set default configuration options
 */
void logsqlite_setvar_default()
{
	xfree(config_logsqlite_path);
	config_logsqlite_path = xstrdup("~/.ekg2/logsqlite.db");
}

/*
 * prepare path to database
 */
char *logsqlite_prepare_path()
{
	char *path, *tmp;
	string_t buf = string_init(NULL);

	if (!(tmp = config_logsqlite_path))
		return NULL;

	if (*tmp == '~' && (*(tmp+1) == '/' || *(tmp+1) == '\0')) {
		const char *home = getenv("HOME");
		string_append_n(buf, (home ? home : "."), -1);
		tmp++;
	}
	string_append_n(buf, tmp, -1);

	path = string_free(buf, 0);

	return path;
}

/*
 * open db
 */
sqlite * logsqlite_open_db()
{
	FILE * testFile;
	char * path;
	sqlite * db;
	char * errormsg = NULL;

	if (!(path = logsqlite_prepare_path()))
		return 0;

	debug("[logsqlite] opening database %s\n", path);

	testFile = fopen(path, "r");
	if (!testFile) {
		debug("[logsqlite] creating database %s\n", path);
		db = sqlite_open(path, 0, &errormsg);
		sqlite_exec(db, "CREATE TABLE log_msg (session text, uid text, nick text, type text, sent boolean, ts timestamp, sentts timestamp, body text)", NULL, NULL, NULL);
		sqlite_exec(db, "CREATE TABLE log_status (session text, uid text, nick text, ts timestamp, status text, desc text)", NULL, NULL, NULL);
	} else {
		fclose(testFile);
		db = sqlite_open(path, 0, &errormsg);
	}

	if (!db) {
		debug("[logsqlite] error opening database - %s\n", *errormsg);
		print("logsqlite_open_error", errormsg);
		xfree(path);
		return 0;
	}
	return db;
}

/*
 * close db
 */
void logsqlite_close_db(sqlite * db)
{
	debug("[logsqlite] close db\n");
	sqlite_close(db);
}

/**
 * handler wiadomo�ci
 */

int logsqlite_msg_handler(void *data, va_list ap)
{
	char    **__session = va_arg(ap, char**),    *session = *__session;
	char        **__uid = va_arg(ap, char**),        *uid = *__uid;
	char     ***__rcpts = va_arg(ap, char***),    **rcpts = *__rcpts;
	char       **__text = va_arg(ap, char**),       *text = *__text;
	uint32_t **__format = va_arg(ap, uint32_t**), *format = *__format;
	time_t      *__sent = va_arg(ap, time_t*),       sent = *__sent;
	int        *__class = va_arg(ap, int*),         class = *__class;
	session_t *s = session_find((const char*)session);
	char * gotten_uid = get_uid(s, uid);
	char * gotten_nickname = get_nickname(s, uid);
	char * type = xmalloc(10);
	sqlite * db;
	int is_sent;

	if (config_logsqlite_log == 0)
		return 0;

	format = NULL;

	if (!session)
		return 0;

	db = logsqlite_open_db();
	if (!db) {
		xfree(type);
		return 0;
	}

	switch ((enum msgclass_t)class) {
		case EKG_MSGCLASS_MESSAGE:
			xstrcpy(type, "msg");
			is_sent = 0;
			break;
		case EKG_MSGCLASS_CHAT:
			xstrcpy(type, "chat");
			is_sent = 0;
			break;
		case EKG_MSGCLASS_SENT:
			xstrcpy(type, "msg");
			is_sent = 1;
			break;
		case EKG_MSGCLASS_SENT_CHAT:
			xstrcpy(type, "chat");
			is_sent = 1;
			break;
		case EKG_MSGCLASS_SYSTEM:
			xstrcpy(type, "system");
			is_sent = 0;
			break;
		default:
			xstrcpy(type, "chat");
			is_sent = 0;
			break;
	};

	if (is_sent) {
		gotten_uid = (rcpts) ? rcpts[0] : NULL;
		if (rcpts) {
			gotten_uid      = get_uid(s, rcpts[0]);
			gotten_nickname = get_nickname(s, rcpts[0]);
		}

		if ( gotten_uid == NULL )
			gotten_uid = rcpts[0];

		if ( gotten_nickname == NULL )
			gotten_nickname = rcpts[0];
	} else {
		if ( gotten_uid == NULL )
			gotten_uid = uid;

		if ( gotten_nickname == NULL )
			gotten_nickname = uid;

	}

	debug("[logsqlite] opening transaction\n");
	sqlite_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

	debug("[logsqlite] running msg query\n");
	sqlite_exec_printf(db, "INSERT INTO log_msg VALUES(%Q, %Q, %Q, %Q, %i, %i, %i, %Q)", 0, 0, 0,
			session,
			gotten_uid,
			gotten_nickname,
			type,
			is_sent,
			time(0),
			sent,
			text);

	debug("[logsqlite] commiting\n");
	sqlite_exec(db, "COMMIT", NULL, NULL, NULL);

	xfree(type);
	logsqlite_close_db(db);

	return 0;
};

/**
 * handler status�w
 */
int logsqlite_status_handler(void *data, va_list ap)
{
        char **__session = va_arg(ap, char**), *session = *__session;
        char **__uid = va_arg(ap, char**), *uid = *__uid;
        char **__status = va_arg(ap, char**), *status = *__status;
        char **__descr = va_arg(ap, char**), *descr = *__descr;
	session_t *s = session_find((const char*)session);
	char * gotten_uid = get_uid(s, uid);
	char * gotten_nickname = get_nickname(s, uid);
	sqlite * db;

	if (!config_logsqlite_log_status)
		return 0;

	if (!session)
		return 0;

	db = logsqlite_open_db();
	if (!db) {
		return 0;
	}

	if ( gotten_uid == NULL )
		gotten_uid = uid;

	if ( gotten_nickname == NULL )
		gotten_nickname = uid;

	if ( descr == NULL )
		descr = "";

	debug("[logsqlite] opening transaction\n");
	sqlite_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

	debug("[logsqlite] running status query\n");
	sqlite_exec_printf(db, "INSERT INTO log_status VALUES(%Q, %Q, %Q, %i, %Q, %Q)", 0, 0, 0,
			session,
			gotten_uid,
			gotten_nickname,
			time(0),
			status,
			descr);

	debug("[logsqlite] commiting\n");
	sqlite_exec(db, "COMMIT", NULL, NULL, NULL);

	logsqlite_close_db(db);

	return 0;
}

int logsqlite_theme_init()
{
	format_add("logsqlite_open_error", "%! Can't open database: %1\n", 1);
	return 0;
}

static int logsqlite_plugin_destroy()
{
	plugin_unregister(&logsqlite_plugin);

	debug("[logs] plugin unregistered\n");

	return 0;
}

int logsqlite_plugin_init(int prio)
{
	plugin_register(&logsqlite_plugin, prio);

	logsqlite_setvar_default();

	command_add(&logsqlite_plugin, "logsqlite:last", "u", logsqlite_cmd_last, 0, NULL);

	query_connect(&logsqlite_plugin, "protocol-message", logsqlite_msg_handler, NULL);
	query_connect(&logsqlite_plugin, "protocol-status", logsqlite_status_handler, NULL);
	variable_add(&logsqlite_plugin, "log", VAR_BOOL, 1, &config_logsqlite_log, NULL, NULL, NULL);
	variable_add(&logsqlite_plugin, "log_ignored", VAR_BOOL, 1, &config_logsqlite_log_ignored, NULL, NULL, NULL);
	variable_add(&logsqlite_plugin, "log_status", VAR_BOOL, 1, &config_logsqlite_log_status, NULL, NULL, NULL);
	variable_add(&logsqlite_plugin, "last_limit", VAR_BOOL, 1, &config_logsqlite_last_limit, NULL, NULL, NULL);
	variable_add(&logsqlite_plugin, "path", VAR_DIR, 1, &config_logsqlite_path, NULL, NULL, NULL);

	debug("[logsqlite] plugin registered\n");

	return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-file-style: "k&r"
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 * vim: sts=8 sw=8 noexpandtab
 */
