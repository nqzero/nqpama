/*
 * pama-source-output-widget.c: A widget to display and manipulate a source output
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
 
#include "pama-source-output-widget.h"
#include "widget-settings.h"

static void     pama_source_output_widget_class_init(PamaSourceOutputWidgetClass *klass);
static void     pama_source_output_widget_init(PamaSourceOutputWidget *widget);
static void     pama_source_output_widget_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);
static GObject* pama_source_output_widget_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties);
static void     pama_source_output_widget_dispose(GObject *gobject);
static void     pama_source_output_widget_weak_ref_notify(gpointer data, GObject *where_the_object_was);

static void     pama_source_output_widget_source_output_notify(GObject *gobject, GParamSpec *pspec, PamaSourceOutputWidget *widget);

static void     pama_source_output_widget_update_values(PamaSourceOutputWidget *widget);

static void     pama_source_output_widget_source_button_clicked(GtkButton *button, PamaSourceOutputWidget *widget);
static void     pama_source_output_widget_source_menu_hidden(GtkWidget *menu_window, PamaSourceOutputWidget *widget);
static void     pama_source_output_widget_source_menu_item_toggled(GtkCheckMenuItem *item, PamaSourceOutputWidget *widget);

struct _PamaSourceOutputWidgetPrivate
{
	GtkWidget *icon, *name, *source_button, *source_menu, *source_button_image;
	GtkSizeGroup *icon_sizegroup;
	PamaPulseContext      *context;
	PamaPulseSourceOutput *source_output;
	gboolean               updating;
	
	gulong context_notify_handler_id, source_output_notify_handler_id;
};

G_DEFINE_TYPE(PamaSourceOutputWidget, pama_source_output_widget, GTK_TYPE_HBOX);
#define PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_SOURCE_OUTPUT_WIDGET, PamaSourceOutputWidgetPrivate))


enum 
{
	PROP_0,

	PROP_SOURCE_OUTPUT,
	PROP_CONTEXT,
	PROP_ICON_SIZEGROUP
};
enum
{
	REORDER_REQUEST_SIGNAL,
	LAST_SIGNAL
};
static guint widget_signals[LAST_SIGNAL] = {0,};

static void pama_source_output_widget_class_init(PamaSourceOutputWidgetClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->set_property = pama_source_output_widget_set_property;
	gobject_class->constructor  = pama_source_output_widget_constructor;
	gobject_class->dispose      = pama_source_output_widget_dispose;

	pspec = g_param_spec_object("source-output",
	                            "Pulse source-output",
	                            "The PamaPulseSourceOutput that is to be controlled.",
	                            PAMA_TYPE_PULSE_SOURCE_OUTPUT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_SOURCE_OUTPUT, pspec);

	pspec = g_param_spec_object("context",
	                            "Pulse context",
	                            "The PamaPulseContext that is managing the current connection.",
	                            PAMA_TYPE_PULSE_CONTEXT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);

	pspec = g_param_spec_object("icon-sizegroup",
	                            "Icon sizegroup",
	                            "The GtkSizeGroup to add the sink output's icon to.",
	                            GTK_TYPE_SIZE_GROUP,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_ICON_SIZEGROUP, pspec);

	widget_signals[REORDER_REQUEST_SIGNAL] =
		g_signal_new("reorder-request",
		             G_TYPE_FROM_CLASS(gobject_class),
		             G_SIGNAL_RUN_LAST,
		             0,
		             NULL,
		             NULL,
		             g_cclosure_marshal_VOID__VOID,
		             G_TYPE_NONE,
		             0);

	g_type_class_add_private(klass, sizeof(PamaSourceOutputWidgetPrivate));
}

static void pama_source_output_widget_init(PamaSourceOutputWidget *widget)
{
}

static void pama_source_output_widget_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaSourceOutputWidget *widget = PAMA_SOURCE_OUTPUT_WIDGET(gobject);
	PamaSourceOutputWidgetPrivate *priv = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(widget);

	switch(property_id)
	{
		case PROP_SOURCE_OUTPUT:
			priv->source_output = g_value_get_object(value);
			g_object_weak_ref(G_OBJECT(priv->source_output), pama_source_output_widget_weak_ref_notify, widget);
			break;

		case PROP_CONTEXT:
			priv->context = g_value_get_object(value);
			g_object_weak_ref(G_OBJECT(priv->context), pama_source_output_widget_weak_ref_notify, widget);
			break;

		case PROP_ICON_SIZEGROUP:
			priv->icon_sizegroup = g_value_dup_object(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
			break;
	}
}
static GObject* pama_source_output_widget_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	GtkWidget *icon, *name, *alignment;
	GtkWidget *inner_box, *source_button, *source_button_image;

	GObject *gobject = G_OBJECT_CLASS(pama_source_output_widget_parent_class)->constructor(gtype, n_properties, properties);
	PamaSourceOutputWidget *widget = PAMA_SOURCE_OUTPUT_WIDGET(gobject);
	PamaSourceOutputWidgetPrivate *priv = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(widget);

	if (NULL == priv->source_output)
		g_error("An attempt was made to create a source_output widget with no source_output.");
	if (NULL == priv->context)
		g_error("An attempt was made to create a source_output widget with no context.");
	if (NULL == priv->icon_sizegroup)
		g_error("An attempt was made to create a source_output widget with no icon sizegroup.");

	alignment = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 0, 6);
	gtk_box_pack_start(GTK_BOX(widget), alignment, FALSE, FALSE, 0);

	icon = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(alignment), icon);
	gtk_size_group_add_widget(priv->icon_sizegroup, icon);
	priv->icon = icon;

	name = g_object_new(GTK_TYPE_LABEL,
	                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                    "wrap", FALSE,
	                    "width-chars", WIDGET_NAME_WIDTH_IN_CHARS,
	                    "xalign", 0.0f,
	                    NULL);
	gtk_box_pack_start(GTK_BOX(widget), name, FALSE, FALSE, 0);
	priv->name = name;

	alignment = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_box_pack_end(GTK_BOX(widget), alignment, FALSE, FALSE, 0);

	inner_box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(alignment), inner_box);

	source_button = gtk_button_new();
	gtk_widget_set_tooltip_text(GTK_WIDGET(source_button), _("Select which input device to use for this application"));
	gtk_box_pack_start(GTK_BOX(inner_box), source_button, FALSE, FALSE, 0);
	priv->source_button = source_button;

	source_button_image = gtk_image_new_from_icon_name("audio-input-microphone", GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(source_button), source_button_image);
	priv->source_button_image = source_button_image;

	pama_source_output_widget_update_values(widget);

	g_signal_connect(source_button, "clicked", G_CALLBACK(pama_source_output_widget_source_button_clicked), widget);

	priv->source_output_notify_handler_id = g_signal_connect(priv->source_output, "notify::source", G_CALLBACK(pama_source_output_widget_source_output_notify), widget);

	/* We no longer need to keep a reference to the icon sizegroup */
	g_object_unref(priv->icon_sizegroup);
	priv->icon_sizegroup = NULL;

	return gobject;
}
static void pama_source_output_widget_dispose(GObject *gobject)
{
	PamaSourceOutputWidget *widget = PAMA_SOURCE_OUTPUT_WIDGET(gobject);
	PamaSourceOutputWidgetPrivate *priv = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(widget);

	if (priv->source_output)
	{
		if (priv->source_output_notify_handler_id)
		{
			g_signal_handler_disconnect(priv->source_output, priv->source_output_notify_handler_id);
			priv->source_output_notify_handler_id = 0;
		}

		g_object_weak_unref(G_OBJECT(priv->source_output), pama_source_output_widget_weak_ref_notify, widget);
		priv->source_output = NULL;
	}

	if (priv->context)
	{
		g_object_weak_unref(G_OBJECT(priv->context), pama_source_output_widget_weak_ref_notify, widget);
		priv->context = NULL;
	}

	if (priv->source_menu)
	{
		g_object_unref(priv->source_menu);
		priv->source_menu = NULL;
	}

	if (priv->icon_sizegroup)
	{
		g_object_unref(priv->icon_sizegroup);
		priv->icon_sizegroup = NULL;
	}

	G_OBJECT_CLASS(pama_source_output_widget_parent_class)->dispose(gobject);
}
static void pama_source_output_widget_weak_ref_notify(gpointer data, GObject *where_the_object_was)
{
	PamaSourceOutputWidget *widget = data;
	PamaSourceOutputWidgetPrivate *priv = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(widget);

	if ((GObject *)priv->source_output    == where_the_object_was)
		priv->source_output = NULL;

	if ((GObject *)priv->context == where_the_object_was)
		priv->context = NULL;

	/* widget is no longer usable without these items. */
	gtk_object_destroy(GTK_OBJECT(widget));
}


static void pama_source_output_widget_source_output_notify(GObject *gobject, GParamSpec *pspec, PamaSourceOutputWidget *widget)
{
	pama_source_output_widget_update_values(widget);
	g_signal_emit(widget, widget_signals[REORDER_REQUEST_SIGNAL], 0);
}

static void pama_source_output_widget_update_values(PamaSourceOutputWidget *widget)
{
	PamaSourceOutputWidgetPrivate *priv = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(widget);
	gchar           *temp;
	gchar           *stream_name, *client_name;
	gchar           *hostname, *application_id;
	gboolean         is_local;
	gboolean         is_pulseaudio;
	PamaPulseClient *client;
	PamaPulseSource *source;
	GIcon           *icon;

	priv->updating = TRUE;

	g_object_get(priv->source_output,
	             "client", &client,
	             "name",   &stream_name,
	             "source", &source,
	             NULL);

	g_object_get(client,
	             "name",     &client_name, 
	             "hostname", &hostname,
	             "is-local", &is_local,
	             "application-id", &application_id,
	             NULL);

	icon = pama_pulse_source_output_build_gicon(priv->source_output);
	g_object_set(priv->icon,
	             "gicon", icon,
	             "pixel-size", 32,
	             NULL);
	g_object_unref(icon);

	if (is_local)
		temp = g_markup_printf_escaped("<b>%s</b>\n%s", client_name, stream_name);
	else
		temp = g_markup_printf_escaped("<b>%s</b> (on %s)\n%s", client_name, hostname, stream_name);
	gtk_label_set_markup(GTK_LABEL(priv->name), temp);
	gtk_widget_set_tooltip_markup(GTK_WIDGET(priv->name), temp);
	g_free(temp);
	g_free(stream_name);
	g_free(client_name);

	// Ideally, this would be done by checking the source output's flags for
	// PA_STREAM_DONT_MOVE, but we don't have that information
	is_pulseaudio = 0 == strcmp(application_id, "org.PulseAudio.PulseAudio");
	gtk_widget_set_sensitive(priv->source_button, !is_pulseaudio);

	g_object_set(priv->source_button_image,
	             "gicon", pama_pulse_source_build_gicon(source),
	             NULL);

	g_object_unref(client);
	g_object_unref(source);

	priv->updating = FALSE;
}

static gint _source_menu_sort_function(gconstpointer a, gconstpointer b) 
{
	gint result = pama_pulse_source_compare_by_is_monitor(a, b);
	if (result)
		return result;

	result = pama_pulse_source_compare_by_hostname(a, b);
	if (result)
		return result;

	return pama_pulse_source_compare_by_description(a, b);

}
static void pama_source_output_widget_source_button_clicked(GtkButton *button, PamaSourceOutputWidget *widget)
{
	PamaSourceOutputWidgetPrivate *priv = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(widget);
	GtkWidget *item, *last = NULL;
	GtkWidget *label;
	GSList *sources, *iter;
	PamaPulseSource *source, *current_source;
	PamaPulseSink *monitored_sink;
	gchar *source_name, *hostname;
	gboolean network;
	gchar *text;

	if (!priv->source_menu)
	{
		priv->source_menu = gtk_menu_new();
		g_object_ref_sink(priv->source_menu);
		g_signal_connect(gtk_widget_get_toplevel(priv->source_menu), "hide", G_CALLBACK(pama_source_output_widget_source_menu_hidden), widget);

		sources = g_slist_copy(pama_pulse_context_get_sources(priv->context));
		sources = g_slist_sort(sources, _source_menu_sort_function);

		g_object_get(priv->source_output, "source", &current_source, NULL);

		for (iter = sources; iter; iter = iter->next)
		{
			source = PAMA_PULSE_SOURCE(iter->data);
			g_object_get(source, 
			             "description", &source_name, 
			             "hostname",    &hostname,
			             "network",     &network,
			             NULL);
			monitored_sink = pama_pulse_source_get_monitored_sink(source);
			if (monitored_sink)
			{
				g_free(source_name);
				g_free(hostname);

				g_object_get(monitored_sink, 
		 		            "description", &text,
		 		            "hostname",    &hostname,
		  		            "network",     &network,
				             NULL);

				source_name = g_markup_printf_escaped(_("Monitor of %s"), text);
				g_free(text);
			}

			if (network)
				text = g_markup_printf_escaped("<b>%s</b> (on %s)", source_name, hostname);
			else
				text = g_markup_printf_escaped("<b>%s</b>", source_name);

			label = g_object_new(GTK_TYPE_LABEL,
			                     "label", text,
			                     "use-markup", TRUE,
			                     "xalign", 0.0,
			                     NULL);

			if (last)
				item = gtk_radio_menu_item_new_from_widget(GTK_RADIO_MENU_ITEM(last));
			else
				item = gtk_radio_menu_item_new(NULL);

			gtk_container_add(GTK_CONTAINER(item), label);

			g_object_set(item,
			             "active", (current_source == source),
			             "user-data", source,
			             NULL);
			last = item;
			g_free(source_name);
			g_free(hostname);
			g_free(text);

			g_signal_connect(item, "toggled", G_CALLBACK(pama_source_output_widget_source_menu_item_toggled), widget);
			gtk_menu_shell_append(GTK_MENU_SHELL(priv->source_menu), item);
			gtk_widget_show_all(item);
		}

		g_slist_free(sources);
		g_object_unref(current_source);

		gtk_menu_popup(GTK_MENU(priv->source_menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	}
}
static void pama_source_output_widget_source_menu_hidden(GtkWidget *menu_window, PamaSourceOutputWidget *widget)
{
	PamaSourceOutputWidgetPrivate *priv = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(widget);
	g_object_unref(priv->source_menu);
	priv->source_menu = NULL;
}
static void pama_source_output_widget_source_menu_item_toggled(GtkCheckMenuItem *item, PamaSourceOutputWidget *widget)
{
	PamaSourceOutputWidgetPrivate *priv = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(widget);
	PamaPulseSource *source;

	if (!gtk_check_menu_item_get_active(item))
		return;

	g_object_get(item, "user-data", &source, NULL);

	pama_pulse_source_output_set_source(priv->source_output, source);
}

gint pama_source_output_widget_compare(gconstpointer a, gconstpointer b)
{
	PamaSourceOutputWidgetPrivate *A = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(a);
	PamaSourceOutputWidgetPrivate *B = PAMA_SOURCE_OUTPUT_WIDGET_GET_PRIVATE(b);

	PamaPulseClient *Ac, *Bc;

	g_object_get(A->source_output, "client", &Ac, NULL);
	g_object_get(B->source_output, "client", &Bc, NULL);

	gint result;

	result = pama_pulse_client_compare_by_is_local(Ac, Bc);
	if (!result)
	{
		result = pama_pulse_client_compare_by_hostname(Ac, Bc);
		if (!result)
		{
			result = pama_pulse_client_compare_by_name(Ac, Bc);
			if (!result)
				result = pama_pulse_source_output_compare_by_name(A->source_output, B->source_output);
		}
	}

	g_object_unref(Ac);
	g_object_unref(Bc);
	return result;
}
