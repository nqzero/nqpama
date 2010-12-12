/*
 * pama-sink-input-widget.c: A widget to display and manipulate a sink input
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
 
#include "pama-sink-input-widget.h"
#include "widget-settings.h"

static void     pama_sink_input_widget_class_init(PamaSinkInputWidgetClass *klass);
static void     pama_sink_input_widget_init(PamaSinkInputWidget *widget);
static void     pama_sink_input_widget_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);
static GObject* pama_sink_input_widget_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties);
static void     pama_sink_input_widget_dispose(GObject *gobject);
static void     pama_sink_input_widget_weak_ref_notify(gpointer data, GObject *where_the_object_was);

static void     pama_sink_input_widget_sink_input_notify(GObject *gobject, GParamSpec *pspec, PamaSinkInputWidget *widget);

static void     pama_sink_input_widget_update_values(PamaSinkInputWidget *widget);
static void     pama_sink_input_widget_mute_toggled(GtkToggleButton *togglebutton, PamaSinkInputWidget *widget);
static void     pama_sink_input_widget_volume_changed(GtkRange *range, PamaSinkInputWidget *widget);

static void     pama_sink_input_widget_sink_button_clicked(GtkButton *button, PamaSinkInputWidget *widget);
static void     pama_sink_input_widget_sink_menu_hidden(GtkWidget *menu_window, PamaSinkInputWidget *widget);
static void     pama_sink_input_widget_sink_menu_item_toggled(GtkCheckMenuItem *item, PamaSinkInputWidget *widget);

struct _PamaSinkInputWidgetPrivate
{
	GtkWidget *icon, *name, *volume, *value, *mute, *sink_button, *sink_menu, *sink_button_image;
	GtkSizeGroup       *icon_sizegroup;
	PamaPulseContext   *context;
	PamaPulseSinkInput *sink_input;
	gboolean            updating;
	
	gulong context_notify_handler_id, sink_input_notify_handler_id;
};

G_DEFINE_TYPE(PamaSinkInputWidget, pama_sink_input_widget, GTK_TYPE_HBOX);
#define PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_SINK_INPUT_WIDGET, PamaSinkInputWidgetPrivate))

enum 
{
	PROP_0,

	PROP_SINK_INPUT,
	PROP_CONTEXT,
	PROP_ICON_SIZEGROUP
};
enum
{
	REORDER_REQUEST_SIGNAL,
	LAST_SIGNAL
};
static guint widget_signals[LAST_SIGNAL] = {0,};

static void pama_sink_input_widget_class_init(PamaSinkInputWidgetClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->set_property = pama_sink_input_widget_set_property;
	gobject_class->constructor  = pama_sink_input_widget_constructor;
	gobject_class->dispose      = pama_sink_input_widget_dispose;

	pspec = g_param_spec_object("sink-input",
	                            "Pulse sink-input",
	                            "The PamaPulseSinkInput that is to be controlled.",
	                            PAMA_TYPE_PULSE_SINK_INPUT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_SINK_INPUT, pspec);

	pspec = g_param_spec_object("context",
	                            "Pulse context",
	                            "The PamaPulseContext that is managing the current connection.",
	                            PAMA_TYPE_PULSE_CONTEXT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);

	pspec = g_param_spec_object("icon-sizegroup",
	                            "Icon sizegroup",
	                            "The GtkSizeGroup to add the sink input's icon to.",
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

	g_type_class_add_private(klass, sizeof(PamaSinkInputWidgetPrivate));
}

static void pama_sink_input_widget_init(PamaSinkInputWidget *widget)
{
}

static void pama_sink_input_widget_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaSinkInputWidget *widget = PAMA_SINK_INPUT_WIDGET(gobject);
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);

	switch(property_id)
	{
		case PROP_SINK_INPUT:
			priv->sink_input = g_value_get_object(value);
			g_object_weak_ref(G_OBJECT(priv->sink_input), pama_sink_input_widget_weak_ref_notify, widget);
			break;

		case PROP_CONTEXT:
			priv->context = g_value_get_object(value);
			g_object_weak_ref(G_OBJECT(priv->context), pama_sink_input_widget_weak_ref_notify, widget);
			break;

		case PROP_ICON_SIZEGROUP:
			priv->icon_sizegroup = g_value_dup_object(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
			break;
	}
}
static GObject* pama_sink_input_widget_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	GtkWidget *icon, *name, *alignment;
	GtkWidget *inner_box, *volume, *value, *mute, *sink_button, *sink_button_image;

	GObject *gobject = G_OBJECT_CLASS(pama_sink_input_widget_parent_class)->constructor(gtype, n_properties, properties);
	PamaSinkInputWidget *widget = PAMA_SINK_INPUT_WIDGET(gobject);
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);

	if (NULL == priv->sink_input)
		g_error("An attempt was made to create a sink_input widget with no sink_input.");
	if (NULL == priv->context)
		g_error("An attempt was made to create a sink_input widget with no context.");
	if (NULL == priv->icon_sizegroup)
		g_error("An attempt was made to create a sink_input widget with no icon sizegroup");

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
	gtk_box_pack_start(GTK_BOX(widget), alignment, FALSE, FALSE, 0);

	volume = gtk_hscale_new_with_range(0, WIDGET_VOLUME_SLIDER_DB_RANGE, 2.5); /* for scroll steps of 5dB */
	gtk_scale_set_draw_value   (GTK_SCALE(volume), FALSE);
	gtk_widget_set_size_request(volume, WIDGET_VOLUME_SLIDER_WIDTH, -1);
	gtk_container_add(GTK_CONTAINER(alignment), volume);
	priv->volume = volume;

	alignment = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(widget), alignment, FALSE, FALSE, 0);

	inner_box = gtk_hbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(alignment), inner_box);

	value = g_object_new(GTK_TYPE_LABEL,
	                     "width-chars", WIDGET_VALUE_WIDTH_IN_CHARS,
	                     "xalign", 1.0f,
	                     NULL);
	gtk_box_pack_start(GTK_BOX(inner_box), value, FALSE, FALSE, 0);
	priv->value = value;

	mute = gtk_check_button_new();
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(mute), FALSE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(mute), _("Mute this application's audio output"));
	gtk_container_add(GTK_CONTAINER(mute), gtk_image_new_from_icon_name("audio-volume-muted", GTK_ICON_SIZE_MENU));
	gtk_box_pack_start(GTK_BOX(inner_box), mute, FALSE, FALSE, 0);
	priv->mute = mute;

	sink_button = gtk_button_new();
	gtk_widget_set_tooltip_text(GTK_WIDGET(sink_button), _("Select which output device to use for this application"));
	gtk_box_pack_start(GTK_BOX(inner_box), sink_button, FALSE, FALSE, 0);
	priv->sink_button = sink_button;

	sink_button_image = gtk_image_new_from_icon_name("audio-card", GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(sink_button), sink_button_image);
	priv->sink_button_image = sink_button_image;

	pama_sink_input_widget_update_values(widget);

	g_signal_connect(volume,      "value-changed", G_CALLBACK(pama_sink_input_widget_volume_changed),      widget);
	g_signal_connect(mute,        "toggled",       G_CALLBACK(pama_sink_input_widget_mute_toggled),        widget);
	g_signal_connect(sink_button, "clicked",       G_CALLBACK(pama_sink_input_widget_sink_button_clicked), widget);

	priv->sink_input_notify_handler_id = g_signal_connect(priv->sink_input, "notify::volume", G_CALLBACK(pama_sink_input_widget_sink_input_notify), widget);

	/* We no longer need to keep a reference to the icon sizegroup */
	g_object_unref(priv->icon_sizegroup);
	priv->icon_sizegroup = NULL;

	return gobject;
}
static void pama_sink_input_widget_dispose(GObject *gobject)
{
	PamaSinkInputWidget *widget = PAMA_SINK_INPUT_WIDGET(gobject);
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);

	if (priv->sink_input)
	{
		if (priv->sink_input_notify_handler_id)
		{
			g_signal_handler_disconnect(priv->sink_input, priv->sink_input_notify_handler_id);
			priv->sink_input_notify_handler_id = 0;
		}

		g_object_weak_unref(G_OBJECT(priv->sink_input), pama_sink_input_widget_weak_ref_notify, widget);
		priv->sink_input = NULL;
	}

	if (priv->context)
	{
		g_object_weak_unref(G_OBJECT(priv->context), pama_sink_input_widget_weak_ref_notify, widget);
		priv->context = NULL;
	}

	if (priv->sink_menu)
	{
		g_object_unref(priv->sink_menu);
		priv->sink_menu = NULL;
	}

	if (priv->icon_sizegroup)
	{
		g_object_unref(priv->icon_sizegroup);
		priv->icon_sizegroup = NULL;
	}

	G_OBJECT_CLASS(pama_sink_input_widget_parent_class)->dispose(gobject);
}
static void pama_sink_input_widget_weak_ref_notify(gpointer data, GObject *where_the_object_was)
{
	PamaSinkInputWidget *widget = data;
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);

	if ((GObject *)priv->sink_input == where_the_object_was)
		priv->sink_input = NULL;

	if ((GObject *)priv->context    == where_the_object_was)
		priv->context = NULL;

	/* widget is no longer usable without these items. */
	gtk_object_destroy(GTK_OBJECT(widget));
}


static void pama_sink_input_widget_sink_input_notify(GObject *gobject, GParamSpec *pspec, PamaSinkInputWidget *widget)
{
	pama_sink_input_widget_update_values(widget);
	g_signal_emit(widget, widget_signals[REORDER_REQUEST_SIGNAL], 0);
}

static void pama_sink_input_widget_update_values(PamaSinkInputWidget *widget)
{
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);
	gchar           *temp;
	gchar           *stream_name, *client_name;
	gchar           *hostname, *application_id;
	gboolean         is_local;
	guint            volume;
	gboolean         mute;
	gboolean         is_pulseaudio;
	PamaPulseClient *client;
	PamaPulseSink   *sink;
	GIcon           *icon;

	double volume_dB;

	priv->updating = TRUE;

	g_object_get(priv->sink_input,
	             "volume",    &volume,
	             "mute",      &mute,
	             "client",    &client,
	             "name",      &stream_name,
	             "sink",      &sink,
	             NULL);

	g_object_get(client,
	             "name",           &client_name,
	             "hostname",       &hostname,
	             "is-local",       &is_local,
	             "application-id", &application_id,
	             NULL);

	icon = pama_pulse_sink_input_build_gicon(priv->sink_input);
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

	volume_dB = pa_sw_volume_to_dB(volume);
	if (isinf(volume_dB))
		gtk_label_set_text(GTK_LABEL(priv->value), "-∞dB");
	else
	{
		temp = g_strdup_printf("%+.1fdB", volume_dB);
		gtk_label_set_text(GTK_LABEL(priv->value), temp);
		g_free(temp);
	}

	gtk_range_set_value(GTK_RANGE(priv->volume), volume_dB + WIDGET_VOLUME_SLIDER_DB_RANGE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->mute), mute);

	gtk_widget_set_sensitive(priv->volume, !mute);
	gtk_widget_set_sensitive(priv->value,  !mute);

	// Ideally, this would be done by checking the sink input's flags for
	// PA_STREAM_DONT_MOVE, but we don't have that information
	is_pulseaudio = 0 == strcmp(application_id, "org.PulseAudio.PulseAudio");
	gtk_widget_set_sensitive(priv->sink_button, !is_pulseaudio);

	g_object_set(priv->sink_button_image,
	             "gicon", pama_pulse_sink_build_gicon(sink),
	             NULL);

	g_object_unref(client);
	g_object_unref(sink);

	priv->updating = FALSE;
}

static void pama_sink_input_widget_mute_toggled   (GtkToggleButton *togglebutton, PamaSinkInputWidget *widget)
{
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);
	if (priv->updating)
		return;

	pama_pulse_sink_input_set_mute(priv->sink_input, gtk_toggle_button_get_active(togglebutton));
}
static void pama_sink_input_widget_volume_changed (GtkRange *range, PamaSinkInputWidget *widget)
{
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);
	if (priv->updating)
		return;

	pama_pulse_sink_input_set_volume(priv->sink_input, pa_sw_volume_from_dB(gtk_range_get_value(range) - WIDGET_VOLUME_SLIDER_DB_RANGE));
}

static gint _sink_menu_sort_function(gconstpointer a, gconstpointer b) 
{
	gint result = pama_pulse_sink_compare_by_hostname(a, b);
	if (result)
		return result;

	return pama_pulse_sink_compare_by_description(a, b);
}
static void pama_sink_input_widget_sink_button_clicked(GtkButton *button, PamaSinkInputWidget *widget)
{
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);
	GtkWidget *item, *last = NULL;
	GtkWidget *label;
	GSList *sinks, *iter;
	PamaPulseSink *sink, *current_sink;
	gchar *sink_name, *hostname;
	gboolean network;
	gchar *text;

	if (!priv->sink_menu)
	{
		priv->sink_menu = gtk_menu_new();
		g_object_ref_sink(priv->sink_menu);
		g_signal_connect(gtk_widget_get_toplevel(priv->sink_menu), "hide", G_CALLBACK(pama_sink_input_widget_sink_menu_hidden), widget);

		sinks = g_slist_copy(pama_pulse_context_get_sinks(priv->context));
		sinks = g_slist_sort(sinks, _sink_menu_sort_function);

		g_object_get(priv->sink_input, "sink", &current_sink, NULL);

		for (iter = sinks; iter; iter = iter->next)
		{
			sink = PAMA_PULSE_SINK(iter->data);
			g_object_get(sink, 
			             "description", &sink_name, 
			             "network",     &network,
			             "hostname",    &hostname,
			             NULL);

			if (network)
				text = g_markup_printf_escaped("<b>%s</b> (on %s)", sink_name, hostname);
			else
				text = g_markup_printf_escaped("<b>%s</b>", sink_name);

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
			             "active", (current_sink == sink),
			             "user-data", sink,
			             NULL);
			last = item;
			g_free(sink_name);
			g_free(hostname);
			g_free(text);

			g_signal_connect(item, "toggled", G_CALLBACK(pama_sink_input_widget_sink_menu_item_toggled), widget);
			gtk_menu_shell_append(GTK_MENU_SHELL(priv->sink_menu), item);
			gtk_widget_show_all(item);
		}

		g_slist_free(sinks);
		g_object_unref(current_sink);

		gtk_menu_popup(GTK_MENU(priv->sink_menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
	}
}
static void pama_sink_input_widget_sink_menu_hidden(GtkWidget *menu_window, PamaSinkInputWidget *widget)
{
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);
	g_object_unref(priv->sink_menu);
	priv->sink_menu = NULL;
}
static void pama_sink_input_widget_sink_menu_item_toggled(GtkCheckMenuItem *item, PamaSinkInputWidget *widget)
{
	PamaSinkInputWidgetPrivate *priv = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(widget);
	PamaPulseSink *sink;

	if (!gtk_check_menu_item_get_active(item))
		return;

	g_object_get(item, "user-data", &sink, NULL);

	pama_pulse_sink_input_set_sink(priv->sink_input, sink);
}

gint pama_sink_input_widget_compare(gconstpointer a, gconstpointer b)
{
	PamaSinkInputWidgetPrivate *A = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(a);
	PamaSinkInputWidgetPrivate *B = PAMA_SINK_INPUT_WIDGET_GET_PRIVATE(b);

	PamaPulseClient *Ac, *Bc;

	g_object_get(A->sink_input, "client", &Ac, NULL);
	g_object_get(B->sink_input, "client", &Bc, NULL);

	gint result;

	result = pama_pulse_client_compare_by_is_local(Ac, Bc);
	if (!result)
	{
		result = pama_pulse_client_compare_by_hostname(Ac, Bc);
		if (!result)
		{
			result = pama_pulse_client_compare_by_name(Ac, Bc);
			if (!result)
				result = pama_pulse_sink_input_compare_by_name(A->sink_input, B->sink_input);
		}
	}

	g_object_unref(Ac);
	g_object_unref(Bc);
	return result;
}
