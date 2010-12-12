/*
 * pama-pulse-source.c: A GObject wrapper for a PulseAudio source
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
#include "pama-pulse-context.h"
#include "pama-pulse-source.h"

#define PAMA_PULSE_SOURCE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_PULSE_SOURCE, PamaPulseSourcePrivate))

struct _PamaPulseSourcePrivate
{
	guint32           index;
	guint32           volume, base_volume;
	guint8            channels;
	gboolean          mute;
	GString *         name;
	GString *         description;
	pa_source_flags_t flags;
	PamaPulseContext *context;
	GString *         hostname;
	gboolean          hardware, network, decibel_volume;
	GString *         icon_name;
	guint32           monitored_sink_index;
	
	pa_operation *current_op;
	gboolean mute_pending;
	gboolean volume_pending;
	gboolean new_mute;
	guint32  new_volume;
};

static void pama_pulse_source_init(PamaPulseSource *source);
static void pama_pulse_source_class_init(PamaPulseSourceClass *klass);
static void pama_pulse_source_finalize(GObject *gobject);
static void pama_pulse_source_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec);
static void pama_pulse_source_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);

static void pama_pulse_source_operation_done(pa_context *c, int success, PamaPulseSource *self);

G_DEFINE_TYPE(PamaPulseSource, pama_pulse_source, G_TYPE_OBJECT);


enum
{
	PROP_0,

	PROP_INDEX,
	PROP_VOLUME,
	PROP_BASE_VOLUME,
	PROP_CHANNELS,
	PROP_MUTE,
	PROP_NAME,
	PROP_DESCRIPTION,
	PROP_CONTEXT,
	PROP_HOSTNAME,
	PROP_HARDWARE,
	PROP_NETWORK,
	PROP_DECIBEL_VOLUME,
	PROP_ICON_NAME,
	PROP_MONITORED_SINK_INDEX
};

static void pama_pulse_source_class_init(PamaPulseSourceClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;
	
	gobject_class->finalize = pama_pulse_source_finalize;
	gobject_class->get_property = pama_pulse_source_get_property;
	gobject_class->set_property = pama_pulse_source_set_property;
	
	pspec = g_param_spec_uint("index",
	                          "Index",
	                          "The index assigned to this source",
	                          0,
	                          G_MAXUINT,
	                          0,
	                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_INDEX, pspec);

	pspec = g_param_spec_uint("volume",
	                          "Volume",
	                          "The raw volume of this source",
	                          0,
	                          G_MAXUINT,
	                          0,
	                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_VOLUME, pspec);

	pspec = g_param_spec_uint("base-volume",
	                          "Base volume",
	                          "The base volume of this source",
	                          0,
	                          G_MAXUINT,
	                          0,
	                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_BASE_VOLUME, pspec);

	pspec = g_param_spec_uchar("channels",
	                           "Channel count",
							   "The number of audio channels of this source",
							   0,
							   G_MAXUINT8,
							   0,
							   G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CHANNELS, pspec);

	pspec = g_param_spec_boolean("mute",
	                             "Mute",
	                             "Indicates whether the source has been muted",
	                             FALSE,
	                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_MUTE, pspec);

	pspec = g_param_spec_string("name",
	                            "Name",
	                            "The systematic name assigned to this source.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	pspec = g_param_spec_string("description",
	                            "Source description",
	                            "The descriptive name of this source.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_DESCRIPTION, pspec);

	pspec = g_param_spec_object("context",
	                            "Pulse context object",
	                            "The PamaPulseContext which this source belongs to.",
	                            PAMA_TYPE_PULSE_CONTEXT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);

	pspec = g_param_spec_string("hostname",
	                            "Remote host name",
	                            "The name of the remote host providing this source, if it is a network source.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_HOSTNAME, pspec);

	pspec = g_param_spec_boolean("hardware",
	                             "Hardware flag",
	                             "Indicates whether the source represents a hardware device.",
	                             FALSE,
	                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_HARDWARE, pspec);

	pspec = g_param_spec_boolean("network",
	                             "Network flag",
	                             "Indicates whether the source represents a network device.",
	                             FALSE,
	                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_NETWORK, pspec);

	pspec = g_param_spec_boolean("decibel-volume",
	                             "Decibel volume flag",
	                             "Indicates whether the source volume can be expressed in decibels.",
	                             FALSE,
	                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_DECIBEL_VOLUME, pspec);

	pspec = g_param_spec_string("icon-name",
	                            "Source icon",
	                            "The name of the icon that represents this source.",
	                            "audio-card",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ICON_NAME, pspec);

	pspec = g_param_spec_uint("monitored-sink-index",
	                          "Monitored sink index",
	                          "The index of the sink that this source is monitoring, or PA_INVALID_INDEX",
	                          0,
	                          G_MAXUINT,
	                          0,
	                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_MONITORED_SINK_INDEX, pspec);


	g_type_class_add_private(klass, sizeof(PamaPulseSourcePrivate));
}

static void pama_pulse_source_init(PamaPulseSource *self)
{
	PamaPulseSourcePrivate *priv;
	
	self->priv = priv = PAMA_PULSE_SOURCE_GET_PRIVATE(self);
	
	priv->name = g_string_new("");
	priv->description = g_string_new("");
	priv->hostname    = g_string_new("");
	priv->icon_name   = g_string_new("");
}

static void pama_pulse_source_finalize(GObject *gobject)
{
	PamaPulseSource *self = PAMA_PULSE_SOURCE(gobject);

	g_string_free(self->priv->name, TRUE);
	g_string_free(self->priv->description, TRUE);
	g_string_free(self->priv->hostname, TRUE);
	g_string_free(self->priv->icon_name, TRUE);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (pama_pulse_source_parent_class)->finalize (gobject);
}

static void pama_pulse_source_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec)
{
	PamaPulseSource *self = PAMA_PULSE_SOURCE(gobject);

	switch(property_id)
	{
		case PROP_INDEX:
			g_value_set_uint(value, self->priv->index);
			break;

		case PROP_VOLUME:
			g_value_set_uint(value, self->priv->volume);
			break;

		case PROP_BASE_VOLUME:
			g_value_set_uint(value, self->priv->base_volume);
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

		case PROP_DESCRIPTION:
			g_value_set_string(value, self->priv->description->str);
			break;

		case PROP_HOSTNAME:
			g_value_set_string(value, self->priv->hostname->str);
			break;

		case PROP_HARDWARE:
			g_value_set_boolean(value, self->priv->hardware);
			break;

		case PROP_NETWORK:
			g_value_set_boolean(value, self->priv->network);
			break;

		case PROP_DECIBEL_VOLUME:
			g_value_set_boolean(value, self->priv->decibel_volume);
			break;

		case PROP_ICON_NAME:
			g_value_set_string(value, self->priv->icon_name->str);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
			break;
	}
}
static void pama_pulse_source_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaPulseSource *self = PAMA_PULSE_SOURCE(gobject);

	switch(property_id)
	{
		case PROP_INDEX:
			self->priv->index = g_value_get_uint(value);
			break;

		case PROP_VOLUME:
			self->priv->volume = g_value_get_uint(value);
			break;

		case PROP_BASE_VOLUME:
			self->priv->base_volume = g_value_get_uint(value);
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

		case PROP_DESCRIPTION:
			g_string_assign(self->priv->description, g_value_get_string(value));
			break;
			
		case PROP_CONTEXT:
			self->priv->context = g_value_get_object(value);
			break;

		case PROP_HOSTNAME:
			g_string_assign(self->priv->hostname, g_value_get_string(value));
			break;

		case PROP_HARDWARE:
			self->priv->hardware = g_value_get_boolean(value);
			break;

		case PROP_NETWORK:
			self->priv->network = g_value_get_boolean(value);
			break;

		case PROP_DECIBEL_VOLUME:
			self->priv->decibel_volume = g_value_get_boolean(value);
			break;

		case PROP_ICON_NAME:
			g_string_assign(self->priv->icon_name, g_value_get_string(value));
			break;

		case PROP_MONITORED_SINK_INDEX:
			self->priv->monitored_sink_index = g_value_get_uint(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
			break;
	}
}

gint pama_pulse_source_compare_to_index(gconstpointer a, gconstpointer b)
{
	const PamaPulseSource *A = a;
	const guint32 *B = b;
	
	return A->priv->index - *B;
}
gint pama_pulse_source_compare_to_name(gconstpointer a, gconstpointer b)
{
	const PamaPulseSource *A = a;
	const char *B = b;
	
	return strcmp(A->priv->name->str, B);
}
gint pama_pulse_source_compare_by_description(gconstpointer a, gconstpointer b)
{
	const PamaPulseSource *A = a;
	const PamaPulseSource *B = b;
	
	PamaPulseSink *Asink = pama_pulse_source_get_monitored_sink(A);
	PamaPulseSink *Bsink = pama_pulse_source_get_monitored_sink(B);

	// Compare monitors by the descriptions of their monitored sinks
	if (Asink && Bsink)
		return pama_pulse_sink_compare_by_description(Asink, Bsink);
	else
		return strcmp(A->priv->description->str, B->priv->description->str);
}
gint pama_pulse_source_compare_by_hostname(gconstpointer a, gconstpointer b)
{
	const PamaPulseSource *A = a;
	const PamaPulseSource *B = b;

	PamaPulseSink *Asink = pama_pulse_source_get_monitored_sink(A);
	PamaPulseSink *Bsink = pama_pulse_source_get_monitored_sink(B);

	// Compare monitors by the hostnames of their monitored sinks
	if (Asink && Bsink)
		return pama_pulse_sink_compare_by_hostname(Asink, Bsink);
	else
		return strcmp(A->priv->hostname->str, B->priv->hostname->str);
}

gint pama_pulse_source_compare_by_is_monitor(gconstpointer a, gconstpointer b)
{
	const PamaPulseSource *A = a;
	const PamaPulseSource *B = b;

	gint result = 0;

	if (PA_INVALID_INDEX == A->priv->monitored_sink_index)
		result--;
	if (PA_INVALID_INDEX == B->priv->monitored_sink_index)
		result++;

	return result;
}




static void pama_pulse_source_operation_done(pa_context *c, int success, PamaPulseSource *self)
{
	pa_operation_unref(self->priv->current_op);
	self->priv->current_op = NULL;
	
	if (self->priv->mute_pending)
	{
		pama_pulse_source_set_mute(self, self->priv->new_mute);
		self->priv->mute_pending = FALSE;
	}

	if (self->priv->volume_pending)
	{
		pama_pulse_source_set_volume(self, self->priv->new_volume);
		self->priv->volume_pending = FALSE;
	}
}

void pama_pulse_source_set_mute(PamaPulseSource *self, gboolean mute)
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

	pa_operation *o = pa_context_set_source_mute_by_index(c, self->priv->index, mute, (pa_context_success_cb_t)pama_pulse_source_operation_done, self);
	if (o)
		self->priv->current_op = o;
}

void pama_pulse_source_set_volume(PamaPulseSource *self, const guint32 volume)
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
	pa_operation *o = pa_context_set_source_volume_by_index(c, self->priv->index, &cvolume, (pa_context_success_cb_t)pama_pulse_source_operation_done, self);
	if (o)
		self->priv->current_op = o;
}

void pama_pulse_source_set_as_default(PamaPulseSource *self)
{
	pa_context   *c;
	pa_operation *o;
	g_object_get(self->priv->context,
	             "context", &c,
	             NULL);

	o = pa_context_set_default_source(c, self->priv->name->str, (pa_context_success_cb_t)pama_pulse_source_operation_done, self);
	if (o)
		self->priv->current_op = o;
}

GIcon *pama_pulse_source_build_gicon(const PamaPulseSource *self)
{
	GIcon *icon = NULL, *temp;
	GEmblem *shared = NULL;
	PamaPulseSink *monitored_sink = pama_pulse_source_get_monitored_sink(self);

	// Makes sure that the icon for a monitor matches the icon of the monitored
	// sink, and is also necessary because the monitor of a network sink is not
	// networked itself
	if (monitored_sink)
		return pama_pulse_sink_build_gicon(monitored_sink);

	if (self->priv->network)
	{
		temp = g_themed_icon_new("emblem-shared");
		shared = g_emblem_new_with_origin(temp, G_EMBLEM_ORIGIN_DEVICE);
		g_object_unref(temp);
	}

	temp = g_themed_icon_new_with_default_fallbacks(self->priv->icon_name->str);

	if (shared)
		icon = g_emblemed_icon_new(temp, shared);

	return icon ? icon : temp;
}

PamaPulseSink *pama_pulse_source_get_monitored_sink(const PamaPulseSource *self)
{
	if (PA_INVALID_INDEX == self->priv->monitored_sink_index)
		return NULL;

	return pama_pulse_context_get_sink_by_index(self->priv->context, self->priv->monitored_sink_index);
}
