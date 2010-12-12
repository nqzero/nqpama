/*
 * pama-pulse-context.h: A GObject wrapper for a PulseAudio context
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

 
#ifndef PAMA_PULSE_CONTEXT_H
#define PAMA_PULSE_CONTEXT_H

#include <glib.h>
#include <glib-object.h>
#include <pulse/pulseaudio.h>

#include "pama-pulse-sink.h"
#include "pama-pulse-source.h"
#include "pama-pulse-client.h"
#include "pama-pulse-sink-input.h"
#include "pama-pulse-source-output.h"

G_BEGIN_DECLS

#define PAMA_TYPE_PULSE_CONTEXT                  (pama_pulse_context_get_type ())
#define PAMA_PULSE_CONTEXT(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAMA_TYPE_PULSE_CONTEXT, PamaPulseContext))
#define PAMA_IS_PULSE_CONTEXT(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PAMA_TYPE_PULSE_CONTEXT))
#define PAMA_PULSE_CONTEXT_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PAMA_TYPE_PULSE_CONTEXT, PamaPulseContextClass))
#define PAMA_IS_PULSE_CONTEXT_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PAMA_TYPE_PULSE_CONTEXT))
#define PAMA_PULSE_CONTEXT_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PAMA_TYPE_PULSE_CONTEXT, PamaPulseContextClass))

typedef struct _PamaPulseContext        PamaPulseContext;
typedef struct _PamaPulseContextClass   PamaPulseContextClass;
typedef struct _PamaPulseContextPrivate PamaPulseContextPrivate;

struct _PamaPulseContext
{
	GObject parent_instance;
	
	PamaPulseContextPrivate *priv;
};

struct _PamaPulseContextClass
{
	GObjectClass parent_class;
};

GType pama_pulse_context_get_type();

/* methods */

PamaPulseContext*       pama_pulse_context_new(pa_mainloop_api *api);

GSList*                 pama_pulse_context_get_sinks(PamaPulseContext *context);
PamaPulseSink*          pama_pulse_context_get_sink_by_index(PamaPulseContext *context, const guint32 index);
PamaPulseSink*          pama_pulse_context_get_sink_by_name(PamaPulseContext *context, const char *name);
PamaPulseSink*          pama_pulse_context_get_default_sink(PamaPulseContext *context);

GSList*                 pama_pulse_context_get_sources(PamaPulseContext *context);
PamaPulseSource*        pama_pulse_context_get_source_by_index(PamaPulseContext *context, const guint32 index);
PamaPulseSource*        pama_pulse_context_get_source_by_name(PamaPulseContext *context, const char *name);
PamaPulseSource*        pama_pulse_context_get_default_source(PamaPulseContext *context);

GSList*                 pama_pulse_context_get_sink_inputs(PamaPulseContext *context);
PamaPulseSinkInput*     pama_pulse_context_get_sink_input_by_index(PamaPulseContext *context, const guint32 index);

GSList*                 pama_pulse_context_get_source_outputs(PamaPulseContext *context);
PamaPulseSourceOutput*  pama_pulse_context_get_source_output_by_index(PamaPulseContext *context, const guint32 index);

GSList*                 pama_pulse_context_get_clients(PamaPulseContext *context);
PamaPulseClient*        pama_pulse_context_get_client_by_index(PamaPulseContext *context, const guint32 index);

G_END_DECLS

#endif /* PAMA_PULSE_CONTEXT_H */

