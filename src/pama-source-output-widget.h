/*
 * pama-source-output-widget.h: A widget to display and manipulate a source output
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

 
#ifndef PAMA_SOURCE_OUTPUT_WIDGET_H
#define PAMA_SOURCE_OUTPUT_WIDGET_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <pulse/pulseaudio.h>
#include "pama-pulse-context.h"

G_BEGIN_DECLS

#define PAMA_TYPE_SOURCE_OUTPUT_WIDGET                  (pama_source_output_widget_get_type ())
#define PAMA_SOURCE_OUTPUT_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAMA_TYPE_SOURCE_OUTPUT_WIDGET, PamaSourceOutputWidget))
#define PAMA_IS_SOURCE_OUTPUT_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PAMA_TYPE_SOURCE_OUTPUT_WIDGET))
#define PAMA_SOURCE_OUTPUT_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PAMA_TYPE_SOURCE_OUTPUT_WIDGET, PamaSourceOutputWidgetClass))
#define PAMA_IS_SOURCE_OUTPUT_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PAMA_TYPE_SOURCE_OUTPUT_WIDGET))
#define PAMA_SOURCE_OUTPUT_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PAMA_TYPE_SOURCE_OUTPUT_WIDGET, PamaSourceOutputWidgetClass))

typedef struct _PamaSourceOutputWidget        PamaSourceOutputWidget;
typedef struct _PamaSourceOutputWidgetClass   PamaSourceOutputWidgetClass;
typedef struct _PamaSourceOutputWidgetPrivate PamaSourceOutputWidgetPrivate;

struct _PamaSourceOutputWidget
{
	GtkHBox parent_instance;
};

struct _PamaSourceOutputWidgetClass
{
	GtkHBoxClass parent_class;
};

GType pama_source_output_widget_get_type();

/* methods */
gint pama_source_output_widget_compare(gconstpointer a, gconstpointer b);

G_END_DECLS

#endif /* PAMA_SOURCE_OUTPUT_WIDGET_H */

