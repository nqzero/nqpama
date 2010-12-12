/*
 * pama-pulse-sink.h: A GObject wrapper for a PulseAudio sink
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

 
#ifndef __PAMA_PULSE_SINK_H__
#define __PAMA_PULSE_SINK_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <pulse/pulseaudio.h>

G_BEGIN_DECLS

#define PAMA_TYPE_PULSE_SINK            (pama_pulse_sink_get_type())
#define PAMA_PULSE_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PAMA_TYPE_PULSE_SINK, PamaPulseSink))
#define PAMA_IS_PULSE_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PAMA_TYPE_PULSE_SINK))
#define PAMA_PULSE_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PAMA_TYPE_PULSE_SINK, PamaPulseSinkClass))
#define PAMA_IS_PULSE_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PAMA_TYPE_PULSE_SINK))
#define PAMA_PULSE_SINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), PAMA_TYPE_PULSE_SINK, PamaPulseSinkClass))

typedef struct _PamaPulseSink        PamaPulseSink;
typedef struct _PamaPulseSinkClass   PamaPulseSinkClass;
typedef struct _PamaPulseSinkPrivate PamaPulseSinkPrivate;

struct _PamaPulseSink
{
	GObject parent_instance;
	
	/*< private >*/
	PamaPulseSinkPrivate *priv;
};

struct _PamaPulseSinkClass
{
	GObjectClass parent_class;
};

GType pama_pulse_sink_get_type(void);

/* methods */
gint pama_pulse_sink_compare_to_index      (gconstpointer a, gconstpointer b);
gint pama_pulse_sink_compare_to_name       (gconstpointer a, gconstpointer b);
gint pama_pulse_sink_compare_by_description(gconstpointer a, gconstpointer b);
gint pama_pulse_sink_compare_by_hostname   (gconstpointer a, gconstpointer b);

void pama_pulse_sink_set_mute(PamaPulseSink *sink, gboolean mute);
void pama_pulse_sink_set_volume(PamaPulseSink *sink, const guint32 volume);
void pama_pulse_sink_set_as_default(PamaPulseSink *sink);

GIcon *pama_pulse_sink_build_gicon(const PamaPulseSink *sink);

G_END_DECLS

#endif /* __PAMA_PULSE_SINK_H__ */

