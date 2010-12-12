/*
 * pama-pulse-sink-input.c: A GObject wrapper for a PulseAudio sink input
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
#include "pama-pulse-sink-input.h"
#include "pama-pulse-context.h"

#define PAMA_PULSE_SINK_INPUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_PULSE_SINK_INPUT, PamaPulseSinkInputPrivate))

struct _PamaPulseSinkInputPrivate
{
	guint32           index;
	guint32           volume;
	guint8            channels;
	gboolean          mute;
	GString          *name;
	PamaPulseClient  *client;
	PamaPulseSink    *sink;
	PamaPulseContext *context;
	GString          *icon_name;
	GString          *role;
	GString          *stream_restore_id;
	
	pa_operation *current_op;
	gboolean mute_pending;
	gboolean volume_pending;
	gboolean move_pending;
	gboolean new_mute;
	guint32  new_volume;
	PamaPulseSink *new_sink;
};

static void pama_pulse_sink_input_init(PamaPulseSinkInput *sink_input);
static void pama_pulse_sink_input_class_init(PamaPulseSinkInputClass *klass);
static void pama_pulse_sink_input_finalize(GObject *gobject);
static void pama_pulse_sink_input_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec);
static void pama_pulse_sink_input_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);

static void pama_pulse_sink_input_operation_done(pa_context *c, int success, PamaPulseSinkInput *self);

G_DEFINE_TYPE(PamaPulseSinkInput, pama_pulse_sink_input, G_TYPE_OBJECT);

enum
{
	PROP_0,

	PROP_INDEX,
	PROP_VOLUME,
	PROP_CHANNELS,
	PROP_MUTE,
	PROP_NAME,
	PROP_CLIENT,
	PROP_SINK,
	PROP_CONTEXT,

	PROP_ICON_NAME,
	PROP_ROLE,
	PROP_STREAM_RESTORE_ID
};

static void pama_pulse_sink_input_class_init(PamaPulseSinkInputClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec   *pspec;
	
	gobject_class->finalize     = pama_pulse_sink_input_finalize;
	gobject_class->get_property = pama_pulse_sink_input_get_property;
	gobject_class->set_property = pama_pulse_sink_input_set_property;

	pspec = g_param_spec_uint("index",
	                          "Sink input index",
	                          "The index assigned to this sink input",
	                          0,
	                          G_MAXUINT,
	                          0,
	                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_INDEX, pspec);

	pspec = g_param_spec_uint("volume",
	                          "Sink input volume",
	                          "The raw volume of this sink input",
	                          0,
	                          G_MAXUINT,
	                          0,
	                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_VOLUME, pspec);

	pspec = g_param_spec_uchar("channels",
	                           "Channel count",
							   "The number of audio channels of this sink input",
							   0,
							   G_MAXUINT8,
							   0,
							   G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CHANNELS, pspec);

	pspec = g_param_spec_boolean("mute",
	                             "Sink input mute flag",
	                             "Indicates whether the sink input has been muted",
	                             FALSE,
	                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_MUTE, pspec);

	pspec = g_param_spec_string("name",
	                            "Sink input identifier",
	                            "The systematic name assigned to this sink input.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	pspec = g_param_spec_object("client",
	                            "Sink input owner",
	                            "The PamaPulseClient that created this sink input.",
	                            PAMA_TYPE_PULSE_CLIENT,
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CLIENT, pspec);

	pspec = g_param_spec_object("sink",
	                            "Sink input playback device",
	                            "The PamaPulseSink that this sink input sends its audio to.",
	                            PAMA_TYPE_PULSE_SINK,
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_SINK, pspec);

	pspec = g_param_spec_object("context",
	                            "Pulse context object",
	                            "The PamaPulseContext which this sink input belongs to.",
	                            PAMA_TYPE_PULSE_CONTEXT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);
	
	pspec = g_param_spec_string("icon-name",
	                            "Sink input icon",
	                            "The name of the icon for this sink input",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ICON_NAME, pspec);

	pspec = g_param_spec_string("role",
	                            "Sink input role",
	                            "The role of this sink input.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ROLE, pspec);

	pspec = g_param_spec_string("stream-restore-id",
	                            "Sink input stream restore identifier",
	                            "The identifier assigned to this sink input by module-stream-restore.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_STREAM_RESTORE_ID, pspec);

	g_type_class_add_private(klass, sizeof(PamaPulseSinkInputPrivate));
}

static void pama_pulse_sink_input_init(PamaPulseSinkInput *self)
{
	PamaPulseSinkInputPrivate *priv;
	
	self->priv = priv = PAMA_PULSE_SINK_INPUT_GET_PRIVATE(self);
	
	priv->name = g_string_new("");
	priv->icon_name = g_string_new("");
	priv->role = g_string_new("");
	priv->stream_restore_id = g_string_new("");
}

static void pama_pulse_sink_input_finalize(GObject *gobject)
{
	PamaPulseSinkInput *self = PAMA_PULSE_SINK_INPUT(gobject);

	g_string_free(self->priv->name, TRUE);
	g_string_free(self->priv->icon_name, TRUE);
	g_string_free(self->priv->role, TRUE);
	g_string_free(self->priv->stream_restore_id, TRUE);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (pama_pulse_sink_input_parent_class)->finalize (gobject);
}

static void pama_pulse_sink_input_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec)
{
	PamaPulseSinkInput *self = PAMA_PULSE_SINK_INPUT(gobject);

	switch(property_id)
	{
		case PROP_INDEX:
			g_value_set_uint(value, self->priv->index);
			break;

		case PROP_VOLUME:
			g_value_set_uint(value, self->priv->volume);
			break;

		case PROP_CHANNELS:
			g_value_set_uchar(value, self->priv->channels);
			break;

		case PROP_MUTE:
			g_value_set_boolean(value, self->priv->mute);
			break;

		case PROP_NAME:
			g_value_set_string(value, self->priv->name->str);
			break;

		case PROP_SINK:
			g_value_set_object(value, self->priv->sink);
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
static void pama_pulse_sink_input_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaPulseSinkInput *self = PAMA_PULSE_SINK_INPUT(gobject);

	switch(property_id)
	{
		case PROP_INDEX:
			self->priv->index = g_value_get_uint(value);
			break;

		case PROP_VOLUME:
			self->priv->volume = g_value_get_uint(value);
			break;

		case PROP_CHANNELS:
			self->priv->channels = g_value_get_uchar(value);
			break;

		case PROP_MUTE:
			self->priv->mute = g_value_get_boolean(value);
			break;

		case PROP_NAME:
			g_string_assign(self->priv->name, g_value_get_string(value));
			break;

		case PROP_CLIENT:
			self->priv->client = g_value_get_object(value);
			break;
			
		case PROP_SINK:
			self->priv->sink = g_value_get_object(value);
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


gint pama_pulse_sink_input_compare_to_index          (gconstpointer a, gconstpointer b)
{
	const PamaPulseSinkInput *A = a;
	const guint32 *B = b;
	
	return A->priv->index - *B;
}
gint pama_pulse_sink_input_compare_by_name           (gconstpointer a, gconstpointer b)
{
	const PamaPulseSinkInput *A = a;
	const PamaPulseSinkInput *B = b;
	
	return strcmp(A->priv->name->str, B->priv->name->str);
}

static void pama_pulse_sink_input_operation_done(pa_context *c, int success, PamaPulseSinkInput *self)
{
	pa_operation_unref(self->priv->current_op);
	self->priv->current_op = NULL;
	
	if (self->priv->mute_pending)
	{
		pama_pulse_sink_input_set_mute(self, self->priv->new_mute);
		self->priv->mute_pending = FALSE;
		return;
	}

	if (self->priv->move_pending)
	{
		pama_pulse_sink_input_set_sink(self, self->priv->new_sink);
		self->priv->move_pending = FALSE;
		return;
	}

	if (self->priv->volume_pending)
	{
		pama_pulse_sink_input_set_volume(self, self->priv->new_volume);
		self->priv->volume_pending = FALSE;
		return;
	}
}

void pama_pulse_sink_input_set_mute(PamaPulseSinkInput *self, gboolean mute)
{
	pa_context *c;
	
	if (self->priv->current_op)
	{
		self->priv->mute_pending = TRUE;
		self->priv->new_mute = mute;
		return;
	}
	
	g_object_get(self->priv->context,
	             "context", &c, 
				 NULL);

	pa_operation *o = pa_context_set_sink_input_mute(c, self->priv->index, mute, (pa_context_success_cb_t)pama_pulse_sink_input_operation_done, self);
	if (o)
		self->priv->current_op = o;
}

void pama_pulse_sink_input_set_volume(PamaPulseSinkInput *self, const guint32 volume)
{
	pa_context *c;
	pa_cvolume cvolume;
	
	if (self->priv->current_op)
	{
		self->priv->volume_pending = TRUE;
		self->priv->new_volume = volume;
		return;
	}
	
	g_object_get(self->priv->context,
	             "context", &c, 
				 NULL);

	pa_cvolume_set(&cvolume, self->priv->channels, volume);
	pa_operation *o = pa_context_set_sink_input_volume(c, self->priv->index, &cvolume, (pa_context_success_cb_t)pama_pulse_sink_input_operation_done, self);
	if (o)
		self->priv->current_op = o;
}

void pama_pulse_sink_input_set_sink(PamaPulseSinkInput *self, PamaPulseSink *sink)
{
	pa_context *c;
	guint sink_index;
	
	if (self->priv->current_op)
	{
		self->priv->move_pending = TRUE;
		self->priv->new_sink = sink;
		return;
	}
	
	g_object_get(self->priv->context, "context", &c, NULL);
	g_object_get(sink, "index", &sink_index, NULL);

	pa_operation *o = pa_context_move_sink_input_by_index(c, self->priv->index, sink_index, (pa_context_success_cb_t)pama_pulse_sink_input_operation_done, self);
	if (o)
		self->priv->current_op = o;
}

GIcon *pama_pulse_sink_input_build_gicon(const PamaPulseSinkInput *self)
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

