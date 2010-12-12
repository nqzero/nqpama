/*
 * pama-pulse-sink-input.h: A GObject wrapper for a PulseAudio sink input
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

 
#ifndef __PAMA_PULSE_SINK_INPUT_H__
#define __PAMA_PULSE_SINK_INPUT_H__

#include <glib.h>
#include <glib-object.h>
#include <pulse/pulseaudio.h>

#include "pama-pulse-sink.h"
#include "pama-pulse-client.h"

G_BEGIN_DECLS

#define PAMA_TYPE_PULSE_SINK_INPUT            (pama_pulse_sink_input_get_type())
#define PAMA_PULSE_SINK_INPUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PAMA_TYPE_PULSE_SINK_INPUT, PamaPulseSinkInput))
#define PAMA_IS_PULSE_SINK_INPUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PAMA_TYPE_PULSE_SINK_INPUT))
#define PAMA_PULSE_SINK_INPUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PAMA_TYPE_PULSE_SINK_INPUT, PamaPulseSinkInputClass))
#define PAMA_IS_PULSE_SINK_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PAMA_TYPE_PULSE_SINK_INPUT))
#define PAMA_PULSE_SINK_INPUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), PAMA_TYPE_PULSE_SINK_INPUT, PamaPulseSinkInputClass))

typedef struct _PamaPulseSinkInput        PamaPulseSinkInput;
typedef struct _PamaPulseSinkInputClass   PamaPulseSinkInputClass;
typedef struct _PamaPulseSinkInputPrivate PamaPulseSinkInputPrivate;

struct _PamaPulseSinkInput
{
	GObject parent_instance;
	
	/*< private >*/
	PamaPulseSinkInputPrivate *priv;
};

struct _PamaPulseSinkInputClass
{
	GObjectClass parent_class;
};

GType pama_pulse_sink_input_get_type(void);

/* methods */
gint pama_pulse_sink_input_compare_to_index           (gconstpointer a, gconstpointer b);
gint pama_pulse_sink_input_compare_by_name            (gconstpointer a, gconstpointer b);

void pama_pulse_sink_input_set_mute(PamaPulseSinkInput *sink_input, gboolean mute);
void pama_pulse_sink_input_set_volume(PamaPulseSinkInput *sink_input, const guint32 volume);
void pama_pulse_sink_input_set_sink(PamaPulseSinkInput *sink_input, PamaPulseSink *sink);

GIcon *pama_pulse_sink_input_build_gicon(const PamaPulseSinkInput *sink_input);

G_END_DECLS

#endif /* __PAMA_SINK_INPUT_H__ */

