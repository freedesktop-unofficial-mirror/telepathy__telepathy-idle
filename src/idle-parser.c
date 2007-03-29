/*
 * This file is part of telepathy-idle
 * 
 * Copyright (C) 2006 Nokia Corporation. All rights reserved.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License 
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "idle-parser.h"

#include "idle-connection.h"

#include <glib.h>
#include <glib-object.h>

#define __USE_GNU
#include <string.h>

#define IDLE_PARSER_GET_PRIVATE(parser) (G_TYPE_INSTANCE_GET_PRIVATE((parser), IDLE_TYPE_PARSER, IdleParserPrivate))

G_DEFINE_TYPE(IdleParser, idle_parser, G_TYPE_OBJECT);

/* signals */
enum {
	MSG_SPLIT = 0,
	LAST_SIGNAL_ENUM
};

static guint signals[LAST_SIGNAL_ENUM] = {0};

typedef struct _IdleParserPrivate IdleParserPrivate;

struct _IdleParserPrivate {
	/* continuation line buffer */
	gchar split_buf[IRC_MSG_MAXLEN+3];
};

static void idle_parser_init(IdleParser *obj) {
	IdleParserPrivate *priv = IDLE_PARSER_GET_PRIVATE(obj);

	memset(priv->split_buf, 0, IRC_MSG_MAXLEN+3);
}

static void idle_parser_class_init(IdleParserClass *klass) {
	g_type_class_add_private(klass, sizeof(IdleParserPrivate));

	signals[MSG_SPLIT] = g_signal_new("msg-split", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

void idle_parser_receive(IdleParser *parser, const gchar *msg) {
	IdleParserPrivate *priv = IDLE_PARSER_GET_PRIVATE(parser);
	int i;
	int lasti = 0;
	gchar *tmp;
	gboolean line_ends = FALSE;
	guint len;
	gchar concat_buf[2 * (IRC_MSG_MAXLEN + 3)] = {'\0'};

	g_assert(msg != NULL);

	len = strnlen(msg, IRC_MSG_MAXLEN + 3);

	for (i = 0; i < len; i++) {
		if ((msg[i] == '\n' || msg[i] == '\r')) {
			if (i > lasti) {
				if ((lasti == 0) && (priv->split_buf[0] != '\0')) {
					g_strlcpy(g_stpcpy(concat_buf, priv->split_buf), msg, i);
					tmp = concat_buf;
					memset(priv->split_buf, '\0', IRC_MSG_MAXLEN + 3);
				}	else {
					tmp = g_strndup(msg + lasti, i - lasti);
				}

				g_debug("%s: split to (%s)", G_STRFUNC, tmp);
				g_signal_emit(parser, signals[MSG_SPLIT], 0, tmp);

				if (tmp != concat_buf)
					g_free(tmp);
			}

			lasti = i+1;
			line_ends = TRUE;
		}	else {
			line_ends = FALSE;
		}
	}

	if (!line_ends)
		g_strlcpy(priv->split_buf, msg + lasti, (IRC_MSG_MAXLEN + 3) - lasti);
	else
		memset(priv->split_buf, '\0', IRC_MSG_MAXLEN + 3);
}

