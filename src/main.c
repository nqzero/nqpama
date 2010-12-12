/*
 * main.c: Bonobo object factory 
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

#include "pama-applet.h"
#include <pulse/glib-mainloop.h>
 
static pa_glib_mainloop *mainloop;
static pa_mainloop_api  *api;
 
static gboolean pama_create(PanelApplet *applet, const gchar *iid, gpointer data)
{
	// One-time applet configuration
	if (NULL == api)
	{
		g_set_application_name(_("PulseAudio Mixer Applet"));
		gtk_window_set_default_icon_name("multimedia-volume-control");

		mainloop = pa_glib_mainloop_new(g_main_context_default());
		api = pa_glib_mainloop_get_api(mainloop);
	}
	
	pama_applet_set_api(PAMA_APPLET(applet), api);
	return TRUE;
}

PANEL_APPLET_BONOBO_FACTORY("OAFIID:PulseAudio-Mixer-Applet_Factory",
                            PAMA_TYPE_APPLET,
                            _("PulseAudio Mixer Applet"),
                            "0.1",
                            pama_create,
                            NULL);
