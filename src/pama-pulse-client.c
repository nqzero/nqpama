/*
 * pama-pulse-client.c: A GObject wrapper for a PulseAudio client
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
#include "pama-pulse-client.h"

#define PAMA_PULSE_CLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_PULSE_CLIENT, PamaPulseClientPrivate))

struct _PamaPulseClientPrivate
{
	guint32  index;
	GString *name;
	GString *icon_name;
	GString *application_id;
	GString *hostname;
	gboolean is_local;
};

static void pama_pulse_client_init(PamaPulseClient *client);
static void pama_pulse_client_class_init(PamaPulseClientClass *klass);
static void pama_pulse_client_finalize(GObject *gobject);
static void pama_pulse_client_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec);
static void pama_pulse_client_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);

G_DEFINE_TYPE(PamaPulseClient, pama_pulse_client, G_TYPE_OBJECT);

enum
{
	PROP_0,

	PROP_INDEX,
	PROP_NAME,
	PROP_ICON_NAME,
	PROP_APPLICATION_ID,
	PROP_HOSTNAME,
	PROP_IS_LOCAL
};

static void pama_pulse_client_class_init(PamaPulseClientClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;
	
	gobject_class->finalize = pama_pulse_client_finalize;
	gobject_class->get_property = pama_pulse_client_get_property;
	gobject_class->set_property = pama_pulse_client_set_property;
	
	pspec = g_param_spec_uint("index",
	                          "Client index",
	                          "Index of this client.",
	                          0,
	                          G_MAXUINT,
	                          0,
	                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_INDEX, pspec);

	pspec = g_param_spec_string("name",
	                            "Client name",
	                            "Name of this client.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_NAME, pspec);

	pspec = g_param_spec_string("icon-name",
	                            "Client icon name",
	                            "Icon of this client.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ICON_NAME, pspec);

	pspec = g_param_spec_string("application-id",
	                            "Client application ID",
	                            "Application id of this client.",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_APPLICATION_ID, pspec);

	pspec = g_param_spec_string("hostname",
	                            "Client host name",
	                            "Name of the machine this client is running on",
	                            "",
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_HOSTNAME, pspec);

	pspec = g_param_spec_boolean("is-local",
	                             "Client is local",
	                             "Indicates whether this client is running on the same machine as the daemon",
	                             TRUE,
	                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_IS_LOCAL, pspec);

	g_type_class_add_private(klass, sizeof(PamaPulseClientPrivate));
}

static void pama_pulse_client_init(PamaPulseClient *self)
{
	PamaPulseClientPrivate *priv;
	
	self->priv = priv = PAMA_PULSE_CLIENT_GET_PRIVATE(self);
	
	priv->name = g_string_new("");
	priv->icon_name = g_string_new("");
	priv->application_id = g_string_new("");
	priv->hostname = g_string_new("");
}

static void pama_pulse_client_finalize(GObject *gobject)
{
	PamaPulseClient *self = PAMA_PULSE_CLIENT(gobject);

	g_string_free(self->priv->name, TRUE);
	g_string_free(self->priv->icon_name, TRUE);
	g_string_free(self->priv->application_id, TRUE);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (pama_pulse_client_parent_class)->finalize (gobject);
}

static void pama_pulse_client_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec)
{
	PamaPulseClient *self = PAMA_PULSE_CLIENT(gobject);

	switch(property_id)
	{
		case PROP_INDEX:
			g_value_set_uint(value, self->priv->index);
			break;

		case PROP_NAME:
			g_value_set_string(value, self->priv->name->str);
			break;

		case PROP_ICON_NAME:
			g_value_set_string(value, self->priv->icon_name->str);
			break;

		case PROP_APPLICATION_ID:
			g_value_set_string(value, self->priv->application_id->str);
			break;

		case PROP_HOSTNAME:
			g_value_set_string(value, self->priv->hostname->str);
			break;

		case PROP_IS_LOCAL:
			g_value_set_boolean(value, self->priv->is_local);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
			break;
	}
}
static void pama_pulse_client_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaPulseClient *self = PAMA_PULSE_CLIENT(gobject);

	switch(property_id)
	{
		case PROP_INDEX:
			self->priv->index = g_value_get_uint(value);
			break;

		case PROP_NAME:
			g_string_assign(self->priv->name, g_value_get_string(value));
			break;

		case PROP_ICON_NAME:
			g_string_assign(self->priv->icon_name, g_value_get_string(value));
			break;

		case PROP_APPLICATION_ID:
			g_string_assign(self->priv->application_id, g_value_get_string(value));
			break;

		case PROP_HOSTNAME:
			g_string_assign(self->priv->hostname, g_value_get_string(value));
			break;

		case PROP_IS_LOCAL:
			self->priv->is_local = g_value_get_boolean(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
			break;
	}
}

gint pama_pulse_client_compare_to_index(gconstpointer a, gconstpointer b)
{
	const PamaPulseClient *A = a;
	const guint32 *B = b;
	
	return A->priv->index - *B;
}
gint pama_pulse_client_compare_to_name (gconstpointer a, gconstpointer b)
{
	const PamaPulseClient *A = a;
	const PamaPulseClient *B = b;
	
	return strcmp(A->priv->name->str, B->priv->name->str);
}
gint pama_pulse_client_compare_by_name (gconstpointer a, gconstpointer b)
{
	const PamaPulseClient *A = a;
	const PamaPulseClient *B = b;

	return strcmp(A->priv->name->str, B->priv->name->str);
}
gint pama_pulse_client_compare_by_hostname (gconstpointer a, gconstpointer b)
{
	const PamaPulseClient *A = a;
	const PamaPulseClient *B = b;
	
	return strcmp(A->priv->hostname->str, B->priv->hostname->str);
}
gint pama_pulse_client_compare_by_is_local(gconstpointer a, gconstpointer b)
{
	const PamaPulseClient *A = a;
	const PamaPulseClient *B = b;
	
	gint result = 0;

	if (A->priv->is_local)
		result--;
	if (B->priv->is_local)
		result++;

	return result;
}
