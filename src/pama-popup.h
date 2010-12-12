/*
 * pama-popup.h: A popup window that can align itself to a widget
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

 
#ifndef PAMA_POPUP_H
#define PAMA_POPUP_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <panel-applet.h>

G_BEGIN_DECLS

#define PAMA_TYPE_POPUP                  (pama_popup_get_type ())
#define PAMA_POPUP(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAMA_TYPE_POPUP, PamaPopup))
#define PAMA_IS_POPUP(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PAMA_TYPE_POPUP))
#define PAMA_POPUP_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PAMA_TYPE_POPUP, PamaPopupClass))
#define PAMA_IS_POPUP_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PAMA_TYPE_POPUP))
#define PAMA_POPUP_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PAMA_TYPE_POPUP, PamaPopupClass))

typedef struct _PamaPopup        PamaPopup;
typedef struct _PamaPopupClass   PamaPopupClass;
typedef struct _PamaPopupPrivate PamaPopupPrivate;

struct _PamaPopup
{
	GtkWindow parent_instance;
};

struct _PamaPopupClass
{
	GtkWindowClass parent_class;
};

GType pama_popup_get_type();

void pama_popup_set_popup_alignment(PamaPopup *popup, GtkWidget *align_to, PanelAppletOrient orientation);
void pama_popup_realign(PamaPopup *popup);
void pama_popup_show(PamaPopup *popup);

G_END_DECLS

#endif /* PAMA_POPUP_H */

