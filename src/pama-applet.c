/*
 * pama-applet.c: Main interface
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
 
#include <string.h>
#include "pama-applet.h"
#include "widget-settings.h"

/* 5% / 5dB volume steps */
#define VOLUME_SCROLL_STEP (PA_VOLUME_NORM/20)
#define VOLUME_SCROLL_STEP_DB 5.0

static gboolean pama_applet_delayed_init              (gpointer data);
static void     pama_applet_dispose                   (GObject *gobject);
static void     pama_applet_change_orient             (PanelApplet *applet, PanelAppletOrient orient);
static void     pama_applet_pulse_context_disconnected(PamaPulseContext *context, gpointer data);
static void     pama_applet_pulse_context_connected   (PamaPulseContext *context, gpointer data);
static void     pama_applet_pulse_context_notify      (PamaPulseContext *context, GParamSpec *pspec, gpointer data);
static void     pama_applet_update_default_devices    (PamaApplet *applet);
static void     pama_applet_io_devs_changed           (gpointer, guint, gpointer data);
static void     pama_applet_io_devs_notify            (gpointer, GParamSpec *, gpointer data);
static gboolean pama_applet_create_context            (gpointer data);
static void     pama_applet_update_icons              (PamaApplet *applet);

static gboolean pama_applet_icon_scroll  (GtkWidget *event_box, GdkEventScroll *event, gpointer data);
static gboolean pama_applet_icon_click   (GtkWidget *event_box, GdkEventButton *event, gpointer data);


static void pama_applet_show_about_dialog (BonoboUIComponent *uic, gpointer data, const char *cname);
static void pama_applet_launch_pavucontrol(BonoboUIComponent *uic, gpointer data, const char *cname);
static void pama_applet_toggle_sink_mute  (BonoboUIComponent *uic, const char *path, Bonobo_UIComponent_EventType type, const char *state, gpointer data);
static void pama_applet_toggle_source_mute(BonoboUIComponent *uic, const char *path, Bonobo_UIComponent_EventType type, const char *state, gpointer data);

struct _PamaAppletPrivate
{
	PanelAppletOrient orientation;

	/* pulse */
	pa_mainloop_api *api;
	PamaPulseContext *context;

	PamaPulseSink   *default_sink;
	PamaPulseSource *default_source;

	/* widgets in applet */
	GtkBox    *box;
	GtkWidget *sink_event_box, *source_event_box;
	GtkWidget *sink_icon,      *source_icon;

	gboolean updating;

	/* popups */
	PamaSinkPopup   *sink_popup;
	PamaSourcePopup *source_popup;
};

static const BonoboUIVerb pama_applet_menu_verbs[] = {
	BONOBO_UI_VERB("Mixer", pama_applet_launch_pavucontrol),
	BONOBO_UI_VERB("About", pama_applet_show_about_dialog ),
	BONOBO_UI_VERB_END
};

G_DEFINE_TYPE(PamaApplet, pama_applet, PANEL_TYPE_APPLET);
#define PAMA_APPLET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_APPLET, PamaAppletPrivate))

static void pama_applet_class_init(PamaAppletClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	PanelAppletClass *applet_class = PANEL_APPLET_CLASS(klass);

	gobject_class->dispose  = pama_applet_dispose;
	applet_class->change_orient = pama_applet_change_orient;

	g_type_class_add_private(klass, sizeof(PamaAppletPrivate));
}

static void pama_applet_init(PamaApplet *applet)
{
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);
	priv->orientation = panel_applet_get_orient(PANEL_APPLET(applet));

	if (pama_applet_is_horizontal(priv->orientation))
		priv->box = GTK_BOX(gtk_hbox_new(FALSE, 2));
	else
		priv->box = GTK_BOX(gtk_vbox_new(FALSE, 2));

	gtk_container_add(GTK_CONTAINER(applet), GTK_WIDGET(priv->box));

	priv->sink_event_box   = gtk_event_box_new();
	priv->source_event_box = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(priv->sink_event_box),   FALSE);
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(priv->source_event_box), FALSE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(priv->sink_event_box),   _("Not connected to PulseAudio server"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(priv->source_event_box), _("Not connected to PulseAudio server"));
	gtk_widget_set_sensitive(GTK_WIDGET(priv->sink_event_box),   FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->source_event_box), FALSE);
	gtk_box_pack_start(priv->box, priv->sink_event_box,   FALSE, TRUE, 0);
	gtk_box_pack_start(priv->box, priv->source_event_box, FALSE, TRUE, 0);

	priv->sink_icon   = gtk_image_new_from_icon_name("audio-volume-muted",           GTK_ICON_SIZE_MENU);
	priv->source_icon = gtk_image_new_from_icon_name("audio-input-microphone-muted", GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(priv->sink_event_box),   priv->sink_icon);
	gtk_container_add(GTK_CONTAINER(priv->source_event_box), priv->source_icon);

	g_idle_add(pama_applet_delayed_init, applet);
	g_object_connect(priv->sink_event_box,
	                 "signal::scroll-event",       pama_applet_icon_scroll,    applet,
	                 "signal::button-press-event", pama_applet_icon_click,     applet,
	                 NULL);
	g_object_connect(priv->source_event_box,
	                 "signal::scroll-event",       pama_applet_icon_scroll,    applet,
	                 "signal::button-press-event", pama_applet_icon_click,     applet,
	                 NULL);

	panel_applet_set_flags(PANEL_APPLET(applet), PANEL_APPLET_EXPAND_MINOR);
	panel_applet_set_background_widget(PANEL_APPLET(applet), GTK_WIDGET(applet));
	gtk_widget_show_all(GTK_WIDGET(applet));
}

static gboolean pama_applet_delayed_init(gpointer data)
{
	PanelApplet *applet = PANEL_APPLET(data);
	BonoboUIComponent *popup;

	panel_applet_setup_menu_from_file(applet,
	                                  DATADIR,
	                                  "PulseAudioMixerApplet.xml",
	                                  NULL,
	                                  pama_applet_menu_verbs,
	                                  data);

	popup = panel_applet_get_popup_component(applet);
	bonobo_ui_component_set_prop(popup, "/commands/MuteSink",   "sensitive", "0", NULL);
	bonobo_ui_component_set_prop(popup, "/commands/MuteSource", "sensitive", "0", NULL);
	bonobo_ui_component_add_listener(popup, "MuteSink",   (BonoboUIListenerFn) pama_applet_toggle_sink_mute,   data);
	bonobo_ui_component_add_listener(popup, "MuteSource", (BonoboUIListenerFn) pama_applet_toggle_source_mute, data);

	return FALSE;
}

void pama_applet_set_api(PamaApplet *applet, pa_mainloop_api *api)
{
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);
	g_warn_if_fail(api);

	priv->api = api;
	g_idle_add(pama_applet_create_context, applet);
}

static gboolean pama_applet_create_context(gpointer data)
{
	PamaApplet *applet = data;
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);

	priv->context = pama_pulse_context_new(priv->api);
	g_object_connect(priv->context, 
	                 "signal::disconnected",   pama_applet_pulse_context_disconnected, applet,
	                 "signal::connected",      pama_applet_pulse_context_connected,    applet,
					 "signal::notify",         pama_applet_pulse_context_notify,       applet,
					 "signal::sink-added",     pama_applet_io_devs_changed,            applet,
					 "signal::sink-removed",   pama_applet_io_devs_changed,            applet,
					 "signal::source-added",   pama_applet_io_devs_changed,            applet,
					 "signal::source-removed", pama_applet_io_devs_changed,            applet,
	                 NULL);

	return FALSE;
}

static void pama_applet_pulse_context_connected(PamaPulseContext *context, gpointer data)
{
	PamaApplet *applet = data;
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);

	gtk_widget_set_sensitive(GTK_WIDGET(priv->sink_event_box),   TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->source_event_box), TRUE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(priv->sink_event_box),   _("No information received yet"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(priv->source_event_box), _("No information received yet"));
}

static void pama_applet_pulse_context_disconnected(PamaPulseContext *context, gpointer data)
{
	PamaApplet *applet = data;
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);

	g_object_unref(context);

	priv->default_sink = NULL;
	priv->default_source = NULL;
	priv->context = NULL;
	
	if (priv->sink_popup)
		gtk_widget_destroy(GTK_WIDGET(priv->sink_popup));
	if (priv->source_popup)
		gtk_widget_destroy(GTK_WIDGET(priv->source_popup));

	gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon),   "audio-volume-muted",           GTK_ICON_SIZE_MENU);
	gtk_image_set_from_icon_name(GTK_IMAGE(priv->source_icon), "audio-input-microphone-muted", GTK_ICON_SIZE_MENU);
	gtk_widget_set_tooltip_text(GTK_WIDGET(priv->sink_event_box),   _("Not connected to PulseAudio server"));
	gtk_widget_set_tooltip_text(GTK_WIDGET(priv->source_event_box), _("Not connected to PulseAudio server"));
	gtk_widget_set_sensitive(GTK_WIDGET(priv->sink_event_box),   FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->source_event_box), FALSE);

	g_timeout_add_seconds(10, pama_applet_create_context, applet);
}

static void pama_applet_pulse_context_notify(PamaPulseContext *context, GParamSpec *pspec, gpointer data)
{
	PamaApplet *applet = data;

	pama_applet_update_default_devices(applet);
}

static void pama_applet_io_devs_changed(gpointer blah, guint index, gpointer data)
{
	/* Handle (sink|source)-(added|removed) signals */
	PamaApplet *applet = data;

	pama_applet_update_default_devices(applet);
	pama_applet_update_icons(applet);
}

static void pama_applet_io_devs_notify(gpointer blah, GParamSpec *pspec, gpointer data)
{
	PamaApplet *applet = data;

	pama_applet_update_icons(applet);
}

static void pama_applet_update_default_devices(PamaApplet *applet)
{
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);
	PamaPulseSink    *new_default_sink   = pama_pulse_context_get_default_sink  (priv->context);
	PamaPulseSource  *new_default_source = pama_pulse_context_get_default_source(priv->context);

	if(new_default_sink != priv->default_sink)
	{
		if (priv->default_sink)
			g_signal_handlers_disconnect_by_func(priv->default_sink, pama_applet_io_devs_notify, applet);

		priv->default_sink = new_default_sink;

		if (priv->default_sink)
			g_signal_connect(priv->default_sink, "notify::volume", G_CALLBACK(pama_applet_io_devs_notify), applet);
	}

	if(new_default_source != priv->default_source)
	{
		if (priv->default_source)
			g_signal_handlers_disconnect_by_func(priv->default_source, pama_applet_io_devs_notify, applet);

		priv->default_source = new_default_source;

		if (priv->default_source)
			g_signal_connect(priv->default_source, "notify::volume", G_CALLBACK(pama_applet_io_devs_notify), applet);
	}

	pama_applet_update_icons(applet);
}

static void pama_applet_change_orient(PanelApplet *panelApplet, PanelAppletOrient applet_orientation)
{
	PamaApplet *applet = PAMA_APPLET(panelApplet);
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);

	if (pama_applet_is_horizontal(priv->orientation) == pama_applet_is_vertical(applet_orientation))
	{
		GtkOrientation box_orientation;
		if (pama_applet_is_vertical(applet_orientation))
			box_orientation = GTK_ORIENTATION_VERTICAL;
		else
			box_orientation = GTK_ORIENTATION_HORIZONTAL;

		gtk_orientable_set_orientation(GTK_ORIENTABLE(priv->box), box_orientation);
	}
	priv->orientation = applet_orientation;

	if (priv->sink_popup)
		gtk_widget_destroy(GTK_WIDGET(priv->sink_popup));
	if (priv->source_popup)
		gtk_widget_destroy(GTK_WIDGET(priv->source_popup));
}

static void pama_applet_dispose(GObject *gobject)
{
	PamaApplet *applet = PAMA_APPLET(gobject);
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);

	if (priv->context)
	{
		g_object_unref(priv->context);
		priv->context = NULL;
	}

	G_OBJECT_CLASS(pama_applet_parent_class)->dispose(gobject);
}



static void pama_applet_update_icons(PamaApplet *applet)
{
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);
	guint32 volume;
	double  volume_dB;
	gboolean mute, decibel_volume, network;
	gchar *description;
	gchar *hostname;
	gchar *tooltip;
	gchar *description_markup;
	BonoboUIComponent *popup;

	priv->updating = TRUE;
	popup = panel_applet_get_popup_component(PANEL_APPLET(applet));

	if (priv->default_sink)
	{
		bonobo_ui_component_set_prop(popup, "/commands/MuteSink", "sensitive", "1", NULL);
		
		g_object_get(priv->default_sink,
		             "mute", &mute,
		             "volume", &volume,
		             "decibel-volume", &decibel_volume,
		             "description", &description,
		             "hostname", &hostname,
		             "network", &network,
		             NULL);

		if (network)
			description_markup = g_markup_printf_escaped("<b>%s</b> (on %s)", description, hostname);
		else
			description_markup = g_markup_printf_escaped("<b>%s</b>", description);

		if (mute)
		{
			bonobo_ui_component_set_prop(popup, "/commands/MuteSink", "state", "1", NULL);
			gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-volume-muted", GTK_ICON_SIZE_MENU);
			tooltip = g_strdup_printf("%s: %s", description_markup, _("muted"));
		}
		else
		{
			bonobo_ui_component_set_prop(popup, "/commands/MuteSink", "state", "0", NULL);
			if (decibel_volume)
			{
				volume_dB = pa_sw_volume_to_dB(volume);

				if (volume_dB < -(WIDGET_VOLUME_SLIDER_DB_RANGE * 2.0 / 3.0))
					gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-volume-low", GTK_ICON_SIZE_MENU);
				else if (volume_dB < -(WIDGET_VOLUME_SLIDER_DB_RANGE / 3.0))
					gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-volume-medium", GTK_ICON_SIZE_MENU);
				else
					gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-volume-high", GTK_ICON_SIZE_MENU);

				if (isinf(volume_dB))
					tooltip = g_strdup_printf("%s: -∞dB", description_markup);
				else
					tooltip = g_strdup_printf("%s: %+.1fdB", description_markup, volume_dB);
			}
			else
			{
				if (volume < (PA_VOLUME_NORM/3))
					gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-volume-low", GTK_ICON_SIZE_MENU);
				else if (volume < (2 * PA_VOLUME_NORM / 3))
					gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-volume-medium", GTK_ICON_SIZE_MENU);
				else
					gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-volume-high", GTK_ICON_SIZE_MENU);

				tooltip = g_strdup_printf("%s: %.0f%%", description_markup, 100.0 * volume / PA_VOLUME_NORM);
			}
		}
		gtk_widget_set_tooltip_markup(GTK_WIDGET(priv->sink_event_box), tooltip);
		g_free(tooltip);
		g_free(description);
		g_free(hostname);
		g_free(description_markup);
	}
	else
	{
		bonobo_ui_component_set_prop(popup, "/commands/MuteSink", "sensitive", "0", NULL);
		gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-volume-muted", GTK_ICON_SIZE_MENU);
		gtk_widget_set_tooltip_text(GTK_WIDGET(priv->sink_event_box), _("No information for the default output device is available"));
	}

	if (priv->default_source)
	{
		bonobo_ui_component_set_prop(popup, "/commands/MuteSource", "sensitive", "1", NULL);
		g_object_get(priv->default_source,
		             "mute", &mute,
		             "volume", &volume,
		             "decibel-volume", &decibel_volume,
		             "description", &description,
		             "hostname", &hostname,
		             "network", &network,
		             NULL);

		if (network)
			description_markup = g_markup_printf_escaped("<b>%s</b> (on %s)", description, hostname);
		else
			description_markup = g_markup_printf_escaped("<b>%s</b>", description);

		if (mute)
		{
			bonobo_ui_component_set_prop(popup, "/commands/MuteSource", "state", "1", NULL);
			gtk_image_set_from_icon_name(GTK_IMAGE(priv->source_icon), "audio-input-microphone-muted", GTK_ICON_SIZE_MENU);
			tooltip = g_strdup_printf("%s: %s", description_markup, _("muted"));
		}
		else
		{
			bonobo_ui_component_set_prop(popup, "/commands/MuteSource", "state", "0", NULL);
			gtk_image_set_from_icon_name(GTK_IMAGE(priv->source_icon), "audio-input-microphone", GTK_ICON_SIZE_MENU);

			if (decibel_volume)
			{
				volume_dB = pa_sw_volume_to_dB(volume);

				if (isinf(volume_dB))
					tooltip = g_strdup_printf("%s: -∞dB", description_markup);
				else
					tooltip = g_strdup_printf("%s: %+.1fdB", description_markup, volume_dB);
			}
			else
			{
				tooltip = g_strdup_printf("%s: %.0f%%", description_markup, 100.0 * volume / PA_VOLUME_NORM);
			}
		}
		gtk_widget_set_tooltip_markup(GTK_WIDGET(priv->source_event_box), tooltip);
		g_free(tooltip);
		g_free(description);
		g_free(hostname);
		g_free(description_markup);
	}
	else
	{
		bonobo_ui_component_set_prop(popup, "/commands/MuteSource", "sensitive", "0", NULL);
		gtk_image_set_from_icon_name(GTK_IMAGE(priv->sink_icon), "audio-input-microphone-muted", GTK_ICON_SIZE_MENU);
		gtk_widget_set_tooltip_text(GTK_WIDGET(priv->sink_event_box), _("No information for the default input device is available"));
	}

	gtk_widget_trigger_tooltip_query(GTK_WIDGET(applet));

	priv->updating = FALSE;
}

static gboolean pama_applet_icon_scroll(GtkWidget *event_box, GdkEventScroll *event, gpointer data)
{
	PamaApplet *applet = data;
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);
	gboolean decibel_volume;
	guint volume;
	double volume_dB;

	if (NULL == event)
		return FALSE;

	switch (event->direction)
	{
		case GDK_SCROLL_UP:
		case GDK_SCROLL_RIGHT:
			if (event_box == priv->sink_event_box && priv->default_sink)
			{
				g_object_get(priv->default_sink, 
				             "volume", &volume, 
				             "decibel-volume", &decibel_volume,
				             NULL);

				if (decibel_volume)
				{
					volume_dB = pa_sw_volume_to_dB(volume);
					volume_dB += VOLUME_SCROLL_STEP_DB;
					volume = pa_sw_volume_from_dB(volume_dB);
				}
				else
				{
					volume += VOLUME_SCROLL_STEP;
				}
				if (volume > PA_VOLUME_NORM)
					volume = PA_VOLUME_NORM;

				pama_pulse_sink_set_volume(priv->default_sink, volume);
			}
			else if (event_box == priv->source_event_box && priv->default_source)
			{
				g_object_get(priv->default_source,
				             "volume", &volume, 
				             "decibel-volume", &decibel_volume,
							 NULL);

				if (decibel_volume)
				{
					volume_dB = pa_sw_volume_to_dB(volume);
					volume_dB += VOLUME_SCROLL_STEP_DB;
					volume = pa_sw_volume_from_dB(volume_dB);
				}
				else
				{
					volume += VOLUME_SCROLL_STEP;
				}

				if (volume > PA_VOLUME_NORM)
					volume = PA_VOLUME_NORM;

				pama_pulse_source_set_volume(priv->default_source, volume);
			}
			break;

		case GDK_SCROLL_DOWN:
		case GDK_SCROLL_LEFT:
			if (event_box == priv->sink_event_box)
			{
				g_object_get(priv->default_sink, 
				             "volume", &volume, 
				             "decibel-volume", &decibel_volume,
				             NULL);

				if (decibel_volume)
				{
					volume_dB = pa_sw_volume_to_dB(volume);
					volume_dB -= VOLUME_SCROLL_STEP_DB;
					volume = pa_sw_volume_from_dB(volume_dB);
				}
				else
				{
					if (volume < VOLUME_SCROLL_STEP)
						volume = 0;
					else
						volume -= VOLUME_SCROLL_STEP;
				}

				pama_pulse_sink_set_volume(priv->default_sink, volume);
			}
			else if (event_box == priv->source_event_box && priv->default_source)
			{
				g_object_get(priv->default_source, 
				             "volume", &volume, 
				             "decibel-volume", &decibel_volume,
				             NULL);

				if (decibel_volume)
				{
					volume_dB = pa_sw_volume_to_dB(volume);
					volume_dB -= VOLUME_SCROLL_STEP_DB;
					volume = pa_sw_volume_from_dB(volume_dB);
				}
				else
				{
					if (volume < VOLUME_SCROLL_STEP)
						volume = 0;
					else
						volume -= VOLUME_SCROLL_STEP;
				}

				pama_pulse_source_set_volume(priv->default_source, volume);
			}
			break;
	}

	return TRUE;
}
static gboolean pama_applet_icon_click (GtkWidget *event_box, GdkEventButton *event, gpointer data)
{
	PamaApplet *applet = data;
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);

	if (NULL == event)
		return FALSE;

	if (GDK_BUTTON_PRESS == event->type)
	{
		if (1 == event->button)
		{
			if (! priv->context)
				return TRUE;

			if (event_box == priv->sink_event_box)
			{
				if (priv->sink_popup)
					gtk_widget_destroy(GTK_WIDGET(priv->sink_popup));
				else
				{
					if (priv->source_popup)
						gtk_widget_destroy(GTK_WIDGET(priv->source_popup));

					priv->sink_popup = pama_sink_popup_new(priv->context);
					g_signal_connect(priv->sink_popup, "destroy", G_CALLBACK(gtk_widget_destroyed),   &priv->sink_popup);
					pama_popup_set_popup_alignment(PAMA_POPUP(priv->sink_popup), priv->sink_event_box, priv->orientation);
					pama_popup_show(PAMA_POPUP(priv->sink_popup));
				}
			}
			else
			{
				if (priv->source_popup)
					gtk_widget_destroy(GTK_WIDGET(priv->source_popup));
				else
				{
					if (priv->sink_popup)
						gtk_widget_destroy(GTK_WIDGET(priv->sink_popup));

					priv->source_popup = pama_source_popup_new(priv->context);
					g_signal_connect(priv->source_popup, "destroy", G_CALLBACK(gtk_widget_destroyed),   &priv->source_popup);
					pama_popup_set_popup_alignment(PAMA_POPUP(priv->source_popup), priv->source_event_box, priv->orientation);
					pama_popup_show(PAMA_POPUP(priv->source_popup));
				}
			}

			return TRUE;
		}
	}
	
	return FALSE;
}


static void pama_applet_show_about_dialog(BonoboUIComponent *uic, gpointer data, const char *cname)
{
	const gchar *authors[] = {
		"Vassili Geronimos <v.geronimos@gmail.com>",
		NULL
	};
	const gchar *translator_credits = _("translator-credits");
	if (0 == strcmp(translator_credits, "translator-credits"))
		translator_credits = NULL;

	gtk_show_about_dialog(NULL,
	                      "program-name", "Pulseaudio Mixer Applet",
	                      "version", PACKAGE_VERSION,
	                      "title", _("About Pulseaudio Mixer Applet"),
	                      "logo-icon-name", "multimedia-volume-control",
	                      "authors", authors,
	                      "comments", _("Per-application volume control for your GNOME panel."), 
	                      "copyright", _("© 2009 Vassili Geronimos"),
	                      "translator-credits", translator_credits,
	                      NULL);
}
static void pama_applet_launch_pavucontrol(BonoboUIComponent *uic, gpointer data, const char *cname)
{
	GError *error;
	if (!g_spawn_command_line_async("pavucontrol", &error))
	{
		GtkWidget *dialog = gtk_message_dialog_new_with_markup(NULL,
		                                                       GTK_DIALOG_NO_SEPARATOR,
		                                                       GTK_MESSAGE_ERROR,
		                                                       GTK_BUTTONS_OK,
		                                                       "<span weight='bold' size='larger'>Unable to launch full mixer</span>\n\n%s",
		                                                       error->message);

		g_signal_connect_swapped(dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);
	}
}
static void pama_applet_toggle_sink_mute  (BonoboUIComponent *uic, const char *path, Bonobo_UIComponent_EventType type, const char *state, gpointer data)
{
	PamaApplet *applet = data;
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);
	gboolean mute;
	
	if (priv->updating)
		return;

	mute = strcmp(state, "0") != 0;
	
	if (priv->default_sink)
		pama_pulse_sink_set_mute(priv->default_sink, mute);
}
static void pama_applet_toggle_source_mute(BonoboUIComponent *uic, const char *path, Bonobo_UIComponent_EventType type, const char *state, gpointer data)
{
	PamaApplet *applet = data;
	PamaAppletPrivate *priv = PAMA_APPLET_GET_PRIVATE(applet);
	gboolean mute;
	
	if (priv->updating)
		return;

	mute = strcmp(state, "0") != 0;
	
	if (priv->default_source)
		pama_pulse_source_set_mute(priv->default_source, mute);
}
