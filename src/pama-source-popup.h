/*
 * pama-source-popup.h: A popup window that displays all sources and source outputs
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

 
#ifndef PAMA_SOURCE_POPUP_H
#define PAMA_SOURCE_POPUP_H

#include <glib.h>
#include <glib-object.h>
#include "pama-popup.h"
#include "pama-pulse-context.h"

G_BEGIN_DECLS

#define PAMA_TYPE_SOURCE_POPUP                  (pama_source_popup_get_type ())
#define PAMA_SOURCE_POPUP(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAMA_TYPE_SOURCE_POPUP, PamaSourcePopup))
#define PAMA_IS_SOURCE_POPUP(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PAMA_TYPE_SOURCE_POPUP))
#define PAMA_SOURCE_POPUP_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), PAMA_TYPE_SOURCE_POPUP, PamaSourcePopupClass))
#define PAMA_IS_SOURCE_POPUP_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), PAMA_TYPE_SOURCE_POPUP))
#define PAMA_SOURCE_POPUP_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), PAMA_TYPE_SOURCE_POPUP, PamaSourcePopupClass))

typedef struct _PamaSourcePopup        PamaSourcePopup;
typedef struct _PamaSourcePopupClass   PamaSourcePopupClass;
typedef struct _PamaSourcePopupPrivate PamaSourcePopupPrivate;

struct _PamaSourcePopup
{
	PamaPopup parent_instance;
};

struct _PamaSourcePopupClass
{
	PamaPopupClass parent_class;
};

GType pama_source_popup_get_type();


/* methods */
PamaSourcePopup *pama_source_popup_new(PamaPulseContext *context);

G_END_DECLS

#endif /* PAMA_SOURCE_POPUP_H */

