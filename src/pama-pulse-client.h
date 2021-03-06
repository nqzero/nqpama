/*
 * pama-pulse-client.h: A GObject wrapper for a PulseAudio client
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

 
#ifndef __PAMA_PULSE_CLIENT_H__
#define __PAMA_PULSE_CLIENT_H__

#include <glib.h>
#include <glib-object.h>
#include <pulse/pulseaudio.h>

G_BEGIN_DECLS

#define PAMA_TYPE_PULSE_CLIENT            (pama_pulse_client_get_type())
#define PAMA_PULSE_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PAMA_TYPE_PULSE_CLIENT, PamaPulseClient))
#define PAMA_IS_PULSE_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PAMA_TYPE_PULSE_CLIENT))
#define PAMA_PULSE_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PAMA_TYPE_PULSE_CLIENT, PamaPulseClientClass))
#define PAMA_IS_PULSE_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PAMA_TYPE_PULSE_CLIENT))
#define PAMA_PULSE_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), PAMA_TYPE_PULSE_CLIENT, PamaPulseClientClass))

typedef struct _PamaPulseClient        PamaPulseClient;
typedef struct _PamaPulseClientClass   PamaPulseClientClass;
typedef struct _PamaPulseClientPrivate PamaPulseClientPrivate;

struct _PamaPulseClient
{
	GObject parent_instance;
	
	/*< private >*/
	PamaPulseClientPrivate *priv;
};

struct _PamaPulseClientClass
{
	GObjectClass parent_class;
};

GType pama_pulse_client_get_type(void);

/* methods */
gint pama_pulse_client_compare_to_index   (gconstpointer a, gconstpointer b);
gint pama_pulse_client_compare_by_name    (gconstpointer a, gconstpointer b);
gint pama_pulse_client_compare_by_hostname(gconstpointer a, gconstpointer b);
gint pama_pulse_client_compare_by_is_local(gconstpointer a, gconstpointer b);

G_END_DECLS

#endif /* __PAMA_CLIENT_H__ */

