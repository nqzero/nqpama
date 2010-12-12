/*
 * pama-source-widget.c: A widget to display and manipulate a source
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
 
#include "pama-source-widget.h"
#include "widget-settings.h"

static void     pama_source_widget_class_init(PamaSourceWidgetClass *klass);
static void     pama_source_widget_init(PamaSourceWidget *widget);
static void     pama_source_widget_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);
static GObject* pama_source_widget_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties);
static void     pama_source_widget_dispose(GObject *gobject);
static void     pama_source_widget_weak_ref_notify(gpointer data, GObject *where_the_object_was);

static void     pama_source_widget_source_notify   (GObject *gobject, GParamSpec *pspec, gpointer data);
static void     pama_source_widget_context_notify(GObject *gobject, GParamSpec *pspec, gpointer data);

static void     pama_source_widget_update_values  (PamaSourceWidget *widget);
static void     pama_source_widget_default_toggled(GtkToggleButton *togglebutton, gpointer data);
static void     pama_source_widget_mute_toggled   (GtkToggleButton *togglebutton, gpointer data);
static void     pama_source_widget_volume_changed (GtkRange *range, gpointer data);

struct _PamaSourceWidgetPrivate
{
	GtkWidget        *icon, *name, *volume, *value, *mute, *default_source;
	GtkSizeGroup     *icon_sizegroup;
	PamaPulseContext *context;
	PamaPulseSource  *source;
	PamaSourceWidget *group;
	gboolean          updating;
	
	gulong context_notify_handler_id, source_notify_handler_id;
};

G_DEFINE_TYPE(PamaSourceWidget, pama_source_widget, GTK_TYPE_HBOX);
#define PAMA_SOURCE_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_SOURCE_WIDGET, PamaSourceWidgetPrivate))


enum 
{
	PROP_0,

	PROP_SOURCE,
	PROP_CONTEXT,
	PROP_GROUP,
	PROP_ICON_SIZEGROUP
};
enum
{
	REORDER_REQUEST_SIGNAL,
	LAST_SIGNAL
};
static guint widget_signals[LAST_SIGNAL] = {0,};

static void pama_source_widget_class_init(PamaSourceWidgetClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->set_property = pama_source_widget_set_property;
	gobject_class->constructor  = pama_source_widget_constructor;
	gobject_class->dispose      = pama_source_widget_dispose;

	pspec = g_param_spec_object("source",
	                            "Pulse source",
	                            "The PamaPulseSource that is to be controlled.",
	                            PAMA_TYPE_PULSE_SOURCE,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_SOURCE, pspec);

	pspec = g_param_spec_object("context",
	                            "Pulse context",
	                            "The PamaPulseContext that is managing the current connection.",
	                            PAMA_TYPE_PULSE_CONTEXT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);

	pspec = g_param_spec_object("group",
	                            "Widget group",
	                            "Another source widget which this widget will be grouped with.",
	                            PAMA_TYPE_SOURCE_WIDGET,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_GROUP, pspec);

	pspec = g_param_spec_object("icon-sizegroup",
	                            "Icon sizegroup",
	                            "The GtkSizeGroup to add the sink's icon to.",
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

	g_type_class_add_private(klass, sizeof(PamaSourceWidgetPrivate));
}

static void pama_source_widget_init(PamaSourceWidget *widget)
{
}

static void pama_source_widget_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaSourceWidget *widget = PAMA_SOURCE_WIDGET(gobject);
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);

	switch(property_id)
	{
		case PROP_SOURCE:
			priv->source = g_value_get_object(value);
			g_object_weak_ref(G_OBJECT(priv->source), pama_source_widget_weak_ref_notify, widget);
			break;

		case PROP_CONTEXT:
			priv->context = g_value_get_object(value);
			g_object_weak_ref(G_OBJECT(priv->context), pama_source_widget_weak_ref_notify, widget);
			break;

		case PROP_GROUP:
			priv->group = g_value_get_object(value);
			break;

		case PROP_ICON_SIZEGROUP:
			priv->icon_sizegroup = g_value_dup_object(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
			break;
	}
}
static GObject* pama_source_widget_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	GtkWidget *icon, *name, *alignment;
	GtkWidget *inner_box, *volume, *value, *mute, *default_source;
	gboolean   decibel_volume;

	GObject *gobject = G_OBJECT_CLASS(pama_source_widget_parent_class)->constructor(gtype, n_properties, properties);
	PamaSourceWidget *widget = PAMA_SOURCE_WIDGET(gobject);
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);

	if (NULL == priv->source)
		g_error("An attempt was made to create a source widget with no source.");
	if (NULL == priv->context)
		g_error("An attempt was made to create a source widget with no context.");
	if (NULL == priv->icon_sizegroup)
		g_error("An attempt was made to create a source widget with no icon sizegroup.");

	g_object_get(priv->source,
	             "decibel-volume", &decibel_volume,
	             NULL);

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
	                    "xalign", 0.0,
	                    NULL);
	gtk_box_pack_start(GTK_BOX(widget), name, FALSE, FALSE, 0);
	priv->name = name;

	alignment = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(widget), alignment, FALSE, FALSE, 0);

	volume = gtk_hscale_new_with_range(0, decibel_volume ? WIDGET_VOLUME_SLIDER_DB_RANGE : 100, 2.5); /* for scroll steps of 5%/5dB */
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
	gtk_widget_set_tooltip_text(GTK_WIDGET(mute), _("Mute audio input from this device"));
	gtk_container_add(GTK_CONTAINER(mute), gtk_image_new_from_icon_name("audio-input-microphone-muted", GTK_ICON_SIZE_MENU));
	gtk_box_pack_start(GTK_BOX(inner_box), mute, FALSE, FALSE, 0);
	priv->mute = mute;

	if (priv->group)
	{
		PamaSourceWidgetPrivate *otherPriv = PAMA_SOURCE_WIDGET_GET_PRIVATE(priv->group);
		default_source = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(otherPriv->default_source));
	}
	else
		default_source = gtk_radio_button_new(NULL);
	gtk_widget_set_tooltip_text(GTK_WIDGET(default_source), _("Set as default"));
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(default_source), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(default_source), priv->source == pama_pulse_context_get_default_source(priv->context));
	gtk_container_add(GTK_CONTAINER(default_source), gtk_image_new_from_icon_name("emblem-default", GTK_ICON_SIZE_MENU));
	gtk_box_pack_start(GTK_BOX(inner_box), default_source, FALSE, FALSE, 0);
	priv->default_source = default_source;

	pama_source_widget_update_values(widget);

	g_signal_connect(volume,         "value-changed", G_CALLBACK(pama_source_widget_volume_changed),  widget);
	g_signal_connect(mute,           "toggled",       G_CALLBACK(pama_source_widget_mute_toggled),    widget);
	g_signal_connect(default_source, "toggled",       G_CALLBACK(pama_source_widget_default_toggled), widget);

	priv->source_notify_handler_id  = g_signal_connect(priv->source,  "notify::volume",            G_CALLBACK(pama_source_widget_source_notify),     widget);
	priv->context_notify_handler_id = g_signal_connect(priv->context, "notify::default-source-name", G_CALLBACK(pama_source_widget_context_notify),  widget);

	priv->group = NULL; /* don't need it for anything else */

	/* We no longer need to keep a reference to the icon sizegroup */
	g_object_unref(priv->icon_sizegroup);
	priv->icon_sizegroup = NULL;

	return gobject;
}
static void pama_source_widget_dispose(GObject *gobject)
{
	PamaSourceWidget *widget = PAMA_SOURCE_WIDGET(gobject);
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);

	if (priv->source)
	{
		if (priv->source_notify_handler_id)
		{
			g_signal_handler_disconnect(priv->source, priv->source_notify_handler_id);
			priv->source_notify_handler_id = 0;
		}

		g_object_weak_unref(G_OBJECT(priv->source), pama_source_widget_weak_ref_notify, widget);
		priv->source = NULL;
	}

	if (priv->context)
	{
		if (priv->context_notify_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->context_notify_handler_id);
			priv->context_notify_handler_id = 0;
		}

		g_object_weak_unref(G_OBJECT(priv->context), pama_source_widget_weak_ref_notify, widget);
		priv->context = NULL;
	}

	if (priv->icon_sizegroup)
	{
		g_object_unref(priv->icon_sizegroup);
		priv->icon_sizegroup = NULL;
	}

	G_OBJECT_CLASS(pama_source_widget_parent_class)->dispose(gobject);
}
static void pama_source_widget_weak_ref_notify(gpointer data, GObject *where_the_object_was)
{
	PamaSourceWidget *widget = data;
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);

	if ((GObject *)priv->source    == where_the_object_was)
		priv->source = NULL;

	if ((GObject *)priv->context == where_the_object_was)
		priv->context = NULL;

	/* widget is no longer usable without these items. */
	gtk_object_destroy(GTK_OBJECT(widget));
}


static void pama_source_widget_source_notify(GObject *gobject, GParamSpec *pspec, gpointer data)
{
	PamaSourceWidget *widget = data;
	pama_source_widget_update_values(widget);
	g_signal_emit(widget, widget_signals[REORDER_REQUEST_SIGNAL], 0);
}
static void pama_source_widget_context_notify(GObject *gobject, GParamSpec *pspec, gpointer data)
{
	PamaSourceWidget *widget = data;
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);

	priv->updating = TRUE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->default_source), priv->source == pama_pulse_context_get_default_source(priv->context));
	priv->updating = FALSE;
}

static void pama_source_widget_update_values(PamaSourceWidget *widget)
{
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);
	gchar         *temp;
	gchar         *description, *hostname;
	guint          volume, base_volume;
	double         volume_dB, base_volume_dB;
	gboolean       mute;
	PamaPulseSink *monitored_sink;
	gboolean       network;
	gboolean       decibel_volume;
	GIcon         *icon;

	priv->updating = TRUE;

	g_object_get(priv->source,
	             "description",    &description,
	             "hostname",       &hostname,
	             "volume",         &volume,
	             "base-volume",    &base_volume,
	             "mute",           &mute,
	             "network",        &network,
	             "decibel-volume", &decibel_volume,
	             NULL);

	monitored_sink = pama_pulse_source_get_monitored_sink(priv->source);
	if (monitored_sink)
	{
		g_free(description);
		g_free(hostname);

		g_object_get(monitored_sink, 
		             "description", &temp,
		             "hostname",    &hostname,
		             "network",     &network,
		             NULL);

		description = g_markup_printf_escaped(_("Monitor of %s"), temp);
		g_free(temp);
	}

	icon = pama_pulse_source_build_gicon(priv->source);
	g_object_set(priv->icon,
	             "gicon", icon,
	             "pixel-size", 32,
	             NULL);
	g_object_unref(icon);

	if (network)
		temp = g_markup_printf_escaped("<b>%s</b>\nOn %s", description, hostname);
	else
		temp = g_markup_printf_escaped("<b>%s</b>", description);
	gtk_label_set_markup(GTK_LABEL(priv->name), temp);
	gtk_widget_set_tooltip_markup(GTK_WIDGET(priv->name), temp);
	g_free(temp);
	g_free(description);

	if (decibel_volume)
	{
		volume_dB = pa_sw_volume_to_dB(volume);
		base_volume_dB = pa_sw_volume_to_dB(base_volume);

		gtk_scale_clear_marks(GTK_SCALE(priv->volume));
		
		temp = g_markup_printf_escaped("<span size='smaller'>%s</span>", _("Normal"));
		gtk_scale_add_mark(GTK_SCALE(priv->volume), WIDGET_VOLUME_SLIDER_DB_RANGE + base_volume_dB, GTK_POS_BOTTOM, temp);
		g_free(temp);

		gtk_range_set_value(GTK_RANGE(priv->volume), WIDGET_VOLUME_SLIDER_DB_RANGE + volume_dB);

		if (isinf(volume_dB))
			temp = g_strdup("-∞dB");
		else
			temp = g_strdup_printf("%+.1fdB", volume_dB);
		gtk_label_set_text(GTK_LABEL(priv->value), temp);
		g_free(temp);
	}
	else
	{
		gtk_scale_clear_marks(GTK_SCALE(priv->volume));
		
		temp = g_markup_printf_escaped("<span size='smaller'>%s</span>", _("Normal"));
		gtk_scale_add_mark(GTK_SCALE(priv->volume), 100.0 * base_volume / PA_VOLUME_NORM, GTK_POS_BOTTOM, temp);
		g_free(temp);

		gtk_range_set_value(GTK_RANGE(priv->volume), 100.0 * volume / PA_VOLUME_NORM);

		temp = g_strdup_printf("%.0f%%", 100.0 * volume / PA_VOLUME_NORM);
		gtk_label_set_text(GTK_LABEL(priv->value), temp);
		g_free(temp);
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->mute), mute);

	gtk_widget_set_sensitive(priv->volume, !mute);
	gtk_widget_set_sensitive(priv->value,  !mute);

	priv->updating = FALSE;
}

static void pama_source_widget_default_toggled(GtkToggleButton *togglebutton, gpointer data)
{
	PamaSourceWidget *widget = data;
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);

	if (priv->updating || !gtk_toggle_button_get_active(togglebutton))
		return;

	pama_pulse_source_set_as_default(priv->source);
}
static void pama_source_widget_mute_toggled   (GtkToggleButton *togglebutton, gpointer data)
{
	PamaSourceWidget *widget = data;
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);

	if (priv->updating)
		return;

	pama_pulse_source_set_mute(priv->source, gtk_toggle_button_get_active(togglebutton));
}
static void pama_source_widget_volume_changed (GtkRange *range, gpointer data)
{
	PamaSourceWidget *widget = data;
	PamaSourceWidgetPrivate *priv = PAMA_SOURCE_WIDGET_GET_PRIVATE(widget);

	if (priv->updating)
		return;

	guint32 new_volume;
	gboolean decibel_volume;
	g_object_get(priv->source, "decibel-volume", &decibel_volume, NULL);

	if (decibel_volume)
		new_volume = pa_sw_volume_from_dB(gtk_range_get_value(range) - WIDGET_VOLUME_SLIDER_DB_RANGE);
	else
		new_volume = gtk_range_get_value(range) * PA_VOLUME_NORM / 100;

	pama_pulse_source_set_volume(priv->source, new_volume);
}


gint pama_source_widget_compare(gconstpointer a, gconstpointer b)
{
	PamaSourceWidgetPrivate *A = PAMA_SOURCE_WIDGET_GET_PRIVATE(a);
	PamaSourceWidgetPrivate *B = PAMA_SOURCE_WIDGET_GET_PRIVATE(b);

	gint result = pama_pulse_source_compare_by_is_monitor(A->source, B->source);
	if (result)
		return result;

	result = pama_pulse_source_compare_by_hostname(A->source, B->source);
	if (result)
		return result;

	return pama_pulse_source_compare_by_description(A->source, B->source);
}
