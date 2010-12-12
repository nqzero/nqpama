/*
 * pama-applet.h: Main interface
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

 
#ifndef PAMA_APPLET_H
#define PAMA_APPLET_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <panel-applet.h>
#include <pulse/pulseaudio.h>

#include "pama-pulse-context.h"

#include "pama-sink-popup.h"
#include "pama-source-popup.h"

G_BEGIN_DECLS

#define PAMA_TYPE_APPLET                  (pama_applet_get_type ())
#define PAMA_APPLET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAMA_TYPE_APPLET, PamaApplet))
#define PAMA_IS_APPLET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PAMA_TYPE_APPLET))
#define PAMA_APPLET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PAMA_TYPE_APPLET, PamaAppletClass))
#define PAMA_IS_APPLET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PAMA_TYPE_APPLET))
#define PAMA_APPLET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PAMA_TYPE_APPLET, PamaAppletClass))

typedef struct _PamaApplet          PamaApplet;
typedef struct _PamaAppletClass     PamaAppletClass;
typedef struct _PamaAppletPrivate   PamaAppletPrivate;

struct _PamaApplet
{
	PanelApplet parent_instance;

};

struct _PamaAppletClass
{
	PanelAppletClass parent_class;
};

GType pama_applet_get_type();

/* methods */
void pama_applet_set_api(PamaApplet *applet, pa_mainloop_api *api);

/* defines */
#define pama_applet_is_horizontal(o) ((PANEL_APPLET_ORIENT_UP   == o) || (PANEL_APPLET_ORIENT_DOWN  == o))
#define pama_applet_is_vertical(o)   ((PANEL_APPLET_ORIENT_LEFT == o) || (PANEL_APPLET_ORIENT_RIGHT == o))

G_END_DECLS

#endif /* PAMA_APPLET_H */

