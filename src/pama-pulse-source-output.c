/*
 * pama-pulse-source-output.c: A GObject wrapper for a PulseAudio source output
 * Part of PulseAudio Mixer Applet
 * Copyright © Vassili Geronimos 2009 <v.geronimos@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

 
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <glib.h>
#include <glib/gi18n.h>
 
#include <string.h>
#include "pama-pulse-source-output.h"
#include "pama-pulse-context.h"

#define PAMA_PULSE_SOURCE_OUTPUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_PULSE_SOURCE_OUTPUT, PamaPulseSourceOutputPrivate))

struct _PamaPulseSourceOutputPrivate
{
	guint32           index;
	GString          *name;
	PamaPulseClient  *client;
	PamaPulseSource  *source;
	PamaPulseContext *context;
	GString          *icon_name;
	GString          *role;
	GString          *stream_restore_id;
};

static void pama_pulse_source_output_init(PamaPulseSourceOutput *source_output);
static void pama_pulse_source_output_class_init(PamaPulseSourceOutputClass *klass);
static void pama_pulse_source_output_finalize(GObject *gobject);
static void pama_pulse_source_output_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec);
static void pama_pulse_source_output_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);

G_DEFINE_TYPE(PamaPulseSourceOutput, pama_pulse_source_output, G_TYPE_OBJECT);

enum
{
	PROP_0,

	PROP_INDEX,
	PROP_NAME,
	PROP_CLIENT,
	PROP_SOURCE,
	PROP_CONTEXT,

	PROP_ICON_NAME,
	PROP_ROLE,
	PROP_STREAM_RESTORE_ID

};

static void pama_pulse_source_output_class_init(PamaPulseSourceOutputClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec   *pspec;
	
	gobject_class->finalize     = pama_pulse_source_output_finalize;
	gobject_class->get_property = pama_pulse_source_output_get_property;
	gobject_class->set_property = pama_pulse_source_output_set_property;

	pspec = g_param_spec_uint("index",
	                          "Source input index",
	                          "The index assigned to this source input",
	                          0,
	                          G_MAXUINT,
	                          0,
	                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_INDEX, pspec);

	pspec = g_param_spec_string("name",
	                            "Source input identifier",
	                            "The systematic name assigned to this source input.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	pspec = g_param_spec_object("client",
	                            "Source input owner",
	                            "The PamaPulseClient that created this source input.",
	                            PAMA_TYPE_PULSE_CLIENT,
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CLIENT, pspec);

	pspec = g_param_spec_object("source",
	                            "Source input playback device",
	                            "The PamaPulseSource that this source input sends its audio to.",
	                            PAMA_TYPE_PULSE_SOURCE,
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_SOURCE, pspec);

	pspec = g_param_spec_object("context",
	                            "Pulse context object",
	                            "The PamaPulseContext which this source input belongs to.",
	                            PAMA_TYPE_PULSE_CONTEXT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);

	pspec = g_param_spec_string("icon-name",
	                            "Source output icon",
	                            "The name of the icon for this source output",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ICON_NAME, pspec);

	pspec = g_param_spec_string("role",
	                            "Source output role",
	                            "The role of this source output.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ROLE, pspec);

	pspec = g_param_spec_string("stream-restore-id",
	                            "Source output stream restore identifier",
	                            "The identifier assigned to this source output by module-stream-restore.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_STREAM_RESTORE_ID, pspec);

	g_type_class_add_private(klass, sizeof(PamaPulseSourceOutputPrivate));
}

static void pama_pulse_source_output_init(PamaPulseSourceOutput *self)
{
	PamaPulseSourceOutputPrivate *priv;
	
	self->priv = priv = PAMA_PULSE_SOURCE_OUTPUT_GET_PRIVATE(self);
	
	priv->name = g_string_new("");
	priv->icon_name = g_string_new("");
	priv->role = g_string_new("");
	priv->stream_restore_id = g_string_new("");
}

static void pama_pulse_source_output_finalize(GObject *gobject)
{
	PamaPulseSourceOutput *self = PAMA_PULSE_SOURCE_OUTPUT(gobject);

	g_string_free(self->priv->name, TRUE);
	g_string_free(self->priv->icon_name, TRUE);
	g_string_free(self->priv->role, TRUE);
	g_string_free(self->priv->stream_restore_id, TRUE);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (pama_pulse_source_output_parent_class)->finalize (gobject);
}

static void pama_pulse_source_output_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec)
{
	PamaPulseSourceOutput *self = PAMA_PULSE_SOURCE_OUTPUT(gobject);

	switch(property_id)
	{
		case PROP_INDEX:
			g_value_set_uint(value, self->priv->index);
			break;

		case PROP_NAME:
			g_value_set_string(value, self->priv->name->str);
			break;

		case PROP_SOURCE:
			g_value_set_object(value, self->priv->source);
			break;

		case PROP_CLIENT:
			g_value_set_object(value, self->priv->client);
			break;

		case PROP_ICON_NAME:
			g_value_set_string(value, self->priv->icon_name->str);
			break;

		case PROP_ROLE:
			g_value_set_string(value, self->priv->role->str);
			break;

		case PROP_STREAM_RESTORE_ID:
			g_value_set_string(value, self->priv->stream_restore_id->str);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
			break;
	}
}
static void pama_pulse_source_output_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaPulseSourceOutput *self = PAMA_PULSE_SOURCE_OUTPUT(gobject);

	switch(property_id)
	{
		case PROP_INDEX:
			self->priv->index = g_value_get_uint(value);
			break;

		case PROP_NAME:
			g_string_assign(self->priv->name, g_value_get_string(value));
			break;

		case PROP_CLIENT:
			self->priv->client = g_value_get_object(value);
			break;
			
		case PROP_SOURCE:
			self->priv->source = g_value_get_object(value);
			break;
			
		case PROP_CONTEXT:
			self->priv->context = g_value_get_object(value);
			break;

		case PROP_ICON_NAME:
			g_string_assign(self->priv->icon_name, g_value_get_string(value));
			break;

		case PROP_ROLE:
			g_string_assign(self->priv->role, g_value_get_string(value));
			break;

		case PROP_STREAM_RESTORE_ID:
			g_string_assign(self->priv->stream_restore_id, g_value_get_string(value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
			break;
	}
}


gint pama_pulse_source_output_compare_to_index(gconstpointer a, gconstpointer b)
{
	const PamaPulseSourceOutput *A = a;
	const guint32 *B = b;
	
	return A->priv->index - *B;
}
gint pama_pulse_source_output_compare_by_name(gconstpointer a, gconstpointer b)
{
	const PamaPulseSourceOutput *A = a;
	const PamaPulseSourceOutput *B = b;
	
	return strcmp(A->priv->name->str, B->priv->name->str);
}

void pama_pulse_source_output_set_source(PamaPulseSourceOutput *self, PamaPulseSource *source)
{
	pa_context *c;
	guint source_index;
	g_object_get(self->priv->context, "context", &c, NULL);
	g_object_get(source, "index", &source_index, NULL);

	pa_operation *o = pa_context_move_source_output_by_index(c, self->priv->index, source_index, NULL, NULL);
	if (o)
		pa_operation_unref(o);
}

GIcon *pama_pulse_source_output_build_gicon(const PamaPulseSourceOutput *self)
{
	GIcon *icon = NULL, *temp;
	GEmblem *shared = NULL;
	gchar *client_icon_name;
	gboolean is_local;

	g_object_get(self->priv->client, "is-local", &is_local, NULL);

	if (!is_local)
	{
		temp = g_themed_icon_new("emblem-shared");
		shared = g_emblem_new_with_origin(temp, G_EMBLEM_ORIGIN_DEVICE);
		g_object_unref(temp);
	}

	// Use the first icon available from:
	// 1. The stream's icon
	// 2. The client's icon
	// 3. A fallback icon
	if (0 < strlen(self->priv->icon_name->str))
	{
		temp = g_themed_icon_new_with_default_fallbacks(self->priv->icon_name->str);
	}
	else
	{
		g_object_get(self->priv->client, "icon-name", &client_icon_name, NULL);

		if (0 < strlen(client_icon_name))
			temp = g_themed_icon_new_with_default_fallbacks(client_icon_name);
		else
			temp = g_themed_icon_new_with_default_fallbacks("application-x-executable");
		
		g_free(client_icon_name);
	}

	if (shared)
		icon = g_emblemed_icon_new(temp, shared);

	return icon ? icon : temp;
}

