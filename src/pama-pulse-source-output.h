/*
 * pama-pulse-source-output.h: A GObject wrapper for a PulseAudio source output
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

 
#ifndef __PAMA_PULSE_SOURCE_OUTPUT_H__
#define __PAMA_PULSE_SOURCE_OUTPUT_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <pulse/pulseaudio.h>

#include "pama-pulse-source.h"
#include "pama-pulse-client.h"

G_BEGIN_DECLS

#define PAMA_TYPE_PULSE_SOURCE_OUTPUT            (pama_pulse_source_output_get_type())
#define PAMA_PULSE_SOURCE_OUTPUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PAMA_TYPE_PULSE_SOURCE_OUTPUT, PamaPulseSourceOutput))
#define PAMA_IS_PULSE_SOURCE_OUTPUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PAMA_TYPE_PULSE_SOURCE_OUTPUT))
#define PAMA_PULSE_SOURCE_OUTPUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PAMA_TYPE_PULSE_SOURCE_OUTPUT, PamaPulseSourceOutputClass))
#define PAMA_IS_PULSE_SOURCE_OUTPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PAMA_TYPE_PULSE_SOURCE_OUTPUT))
#define PAMA_PULSE_SOURCE_OUTPUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), PAMA_TYPE_PULSE_SOURCE_OUTPUT, PamaPulseSourceOutputClass))

typedef struct _PamaPulseSourceOutput        PamaPulseSourceOutput;
typedef struct _PamaPulseSourceOutputClass   PamaPulseSourceOutputClass;
typedef struct _PamaPulseSourceOutputPrivate PamaPulseSourceOutputPrivate;

struct _PamaPulseSourceOutput
{
	GObject parent_instance;
	
	/*< private >*/
	PamaPulseSourceOutputPrivate *priv;
};

struct _PamaPulseSourceOutputClass
{
	GObjectClass parent_class;
};

GType pama_pulse_source_output_get_type(void);

/* methods */
gint pama_pulse_source_output_compare_to_index(gconstpointer a, gconstpointer b);
gint pama_pulse_source_output_compare_by_name (gconstpointer a, gconstpointer b);

void pama_pulse_source_output_set_source(PamaPulseSourceOutput *source_output, PamaPulseSource *source);

GIcon *pama_pulse_source_output_build_gicon(const PamaPulseSourceOutput *self);

G_END_DECLS

#endif /* __PAMA_SOURCE_OUTPUT_H__ */

