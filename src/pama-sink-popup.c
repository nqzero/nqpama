/*
 * pama-sink-popup.c: A popup window that displays all sinks and sink inputs
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
#include <gtk-2.0/gdk/gdkwindow.h>
 
#include "pama-sink-popup.h"
#include "pama-sink-widget.h"
#include "pama-sink-input-widget.h"

static void     pama_sink_popup_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);
static GObject* pama_sink_popup_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties);
static void     pama_sink_popup_dispose(GObject *gobject);
static void     pama_sink_popup_weak_ref_notify(gpointer data, GObject *where_the_object_was);

static void     pama_sink_popup_sink_added(PamaPulseContext *context, guint index, gpointer data);
static void     pama_sink_popup_sink_removed(PamaPulseContext *context, guint index, gpointer data);
static void     pama_sink_popup_sink_input_added(PamaPulseContext *context, guint index, gpointer data);
static void     pama_sink_popup_sink_input_removed(PamaPulseContext *context, guint index, gpointer data);

static void     pama_sink_popup_reorder_sinks      (PamaSinkWidget      *widget, gpointer data);
static void     pama_sink_popup_reorder_sink_inputs(PamaSinkInputWidget *widget, gpointer data);

static void     pama_sink_popup_add_sink      (PamaSinkPopup *popup, PamaPulseSink      *sink);
static void     pama_sink_popup_add_sink_input(PamaSinkPopup *popup, PamaPulseSinkInput *sink_input);

struct _PamaSinkPopupPrivate
{
	GtkBox *sink_box, *stream_box;
	GtkSizeGroup *icon_sizegroup;
	GtkWidget *no_apps, *no_devices;

	PamaPulseContext *context;
	gulong sink_added_handler_id;
	gulong sink_removed_handler_id;
	gulong sink_input_added_handler_id;
	gulong sink_input_removed_handler_id;
};


G_DEFINE_TYPE(PamaSinkPopup, pama_sink_popup, PAMA_TYPE_POPUP);
#define PAMA_SINK_POPUP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_SINK_POPUP, PamaSinkPopupPrivate))


enum
{
	PROP_0,

	PROP_CONTEXT
};

static void pama_sink_popup_class_init(PamaSinkPopupClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->set_property = pama_sink_popup_set_property;
	gobject_class->constructor  = pama_sink_popup_constructor;
	gobject_class->dispose      = pama_sink_popup_dispose;

	pspec = g_param_spec_object("context",
	                            "Pulse context object",
	                            "The PamaPulseContext which this sink belongs to.",
	                            PAMA_TYPE_PULSE_CONTEXT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);

	g_type_class_add_private(klass, sizeof(PamaSinkPopupPrivate));
}

static void pama_sink_popup_init(PamaSinkPopup *popup)
{
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);
	gchar *markup;
	GtkWidget *frame, *main_box;
	GtkWidget *sink_frame,   *sink_align,   *sink_box;
	GtkWidget *stream_frame, *stream_align, *stream_box;
	GtkWidget *no_devices, *no_apps;
	GtkSizeGroup *icon_sizegroup;

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_container_add(GTK_CONTAINER(popup), frame);

	main_box = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(main_box), 6);
	gtk_container_add(GTK_CONTAINER(frame), main_box);


	sink_frame = gtk_frame_new("");
	markup = g_markup_printf_escaped("<b>%s</b>", _("Playback Devices"));
	gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(sink_frame))), markup);
	gtk_frame_set_shadow_type(GTK_FRAME(sink_frame), GTK_SHADOW_NONE);
	gtk_box_pack_start(GTK_BOX(main_box), sink_frame, FALSE, FALSE, 0);
	g_free(markup);
	
	sink_align = gtk_alignment_new(0,0,1,0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(sink_align), 6, 0, 12, 0);
	gtk_container_add(GTK_CONTAINER(sink_frame), sink_align);

	sink_box = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(sink_align), sink_box);
	priv->sink_box = GTK_BOX(sink_box);

	markup = g_markup_printf_escaped("<i>%s</i>", _("No playback devices available"));
	no_devices = g_object_new(GTK_TYPE_LABEL,
	                       "ellipsize", PANGO_ELLIPSIZE_MIDDLE, 
	                       "wrap", FALSE, 
	                       "xalign", 0.0, 
	                       "label", markup, 
	                       "use-markup", TRUE, 
	                       "no-show-all", TRUE, 
	                       NULL);
	gtk_widget_show(no_devices);
	gtk_box_pack_start(GTK_BOX(sink_box), no_devices, FALSE, FALSE, 0);
	priv->no_devices = no_devices;
	g_free(markup);

	
	
	stream_frame = gtk_frame_new("");
	markup = g_markup_printf_escaped("<b>%s</b>", _("Applications"));
	gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(stream_frame))), markup);
	gtk_frame_set_shadow_type(GTK_FRAME(stream_frame), GTK_SHADOW_NONE);
	gtk_box_pack_start(GTK_BOX(main_box), stream_frame, FALSE, FALSE, 0);
	g_free(markup);
	
	stream_align = gtk_alignment_new(0,0,1,0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(stream_align), 6, 0, 12, 0);
	gtk_container_add(GTK_CONTAINER(stream_frame), stream_align);

	stream_box = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(stream_align), stream_box);
	priv->stream_box = GTK_BOX(stream_box);

	markup = g_markup_printf_escaped("<i>%s</i>", _("No applications playing"));
	no_apps = g_object_new(GTK_TYPE_LABEL,
	                       "ellipsize", PANGO_ELLIPSIZE_MIDDLE, 
	                       "wrap", FALSE, 
	                       "xalign", 0.0, 
	                       "label", markup, 
	                       "use-markup", TRUE, 
	                       "no-show-all", TRUE, 
	                       NULL);
	gtk_widget_show(no_apps);
	gtk_box_pack_start(GTK_BOX(stream_box), no_apps, FALSE, FALSE, 0);
	priv->no_apps = no_apps;
	g_free(markup);


	icon_sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
	priv->icon_sizegroup = icon_sizegroup;
}

static GObject* pama_sink_popup_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	GObject *gobject = G_OBJECT_CLASS(pama_sink_popup_parent_class)->constructor(gtype, n_properties, properties);
	PamaSinkPopup *popup = PAMA_SINK_POPUP(gobject);
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);
	GSList *iter;

	if (NULL == priv->context)
		g_error("An attempt was made to construct a PamaSinkPopup without providing a valid PamaPulseContext.");

	for (iter = pama_pulse_context_get_sinks(priv->context); iter; iter = iter->next)
	{
		PamaPulseSink *sink = PAMA_PULSE_SINK(iter->data);

		pama_sink_popup_add_sink(popup, sink);
	}

	for (iter = pama_pulse_context_get_sink_inputs(priv->context); iter; iter = iter->next)
	{
		PamaPulseSinkInput *sink_input = PAMA_PULSE_SINK_INPUT(iter->data);

		pama_sink_popup_add_sink_input(popup, sink_input);
	}

	pama_sink_popup_reorder_sinks(NULL, popup);
	priv->sink_added_handler_id         = g_signal_connect(priv->context, "sink-added",       G_CALLBACK(pama_sink_popup_sink_added),       popup);
	priv->sink_removed_handler_id       = g_signal_connect(priv->context, "sink-removed",     G_CALLBACK(pama_sink_popup_sink_removed),     popup);
	priv->sink_input_added_handler_id   = g_signal_connect(priv->context, "sink-input-added", G_CALLBACK(pama_sink_popup_sink_input_added), popup);
	priv->sink_input_removed_handler_id = g_signal_connect(priv->context, "sink-input-removed", G_CALLBACK(pama_sink_popup_sink_input_removed), popup);

	gtk_widget_show_all(GTK_BIN(popup)->child);

	return gobject;
}
static void pama_sink_popup_dispose(GObject *gobject)
{
	PamaSinkPopup *popup = PAMA_SINK_POPUP(gobject);
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);

	if (priv->context)
	{
		if (priv->sink_added_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->sink_added_handler_id);
			priv->sink_added_handler_id = 0;
		}

		if (priv->sink_removed_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->sink_removed_handler_id);
			priv->sink_removed_handler_id = 0;
		}

		if (priv->sink_input_added_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->sink_input_added_handler_id);
			priv->sink_input_added_handler_id = 0;
		}

		if (priv->sink_input_removed_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->sink_input_removed_handler_id);
			priv->sink_input_removed_handler_id = 0;
		}

		g_object_weak_unref(G_OBJECT(priv->context), pama_sink_popup_weak_ref_notify, popup);
		priv->context = NULL;
	}

	G_OBJECT_CLASS(pama_sink_popup_parent_class)->dispose(gobject);
}
static void pama_sink_popup_weak_ref_notify(gpointer data, GObject *where_the_object_was)
{
	PamaSinkPopup *popup = PAMA_SINK_POPUP(data);
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);

	if ((GObject *)priv->context == where_the_object_was)
	{
		priv->context = NULL;
	}

	gtk_object_destroy(GTK_OBJECT(popup));
}

static void pama_sink_popup_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaSinkPopup *popup = PAMA_SINK_POPUP(gobject);
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);

	switch(property_id)
	{
		case PROP_CONTEXT:
			priv->context = g_value_get_object(value);
			g_object_weak_ref(G_OBJECT(priv->context), pama_sink_popup_weak_ref_notify, popup);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
			break;
	}
}

static void  pama_sink_popup_add_sink(PamaSinkPopup *popup, PamaPulseSink *sink)
{
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);
	GtkWidget *group = NULL;
	GtkWidget *sink_widget;
	GList *iter;
	GList *box_children = gtk_container_get_children(GTK_CONTAINER(priv->sink_box));

	gtk_widget_hide(priv->no_devices);
	
	/* If there is already a source widget, group with it */
	for (iter = box_children; iter; iter = iter->next)
	{
		if (PAMA_IS_SINK_WIDGET(iter->data))
		{
			group = GTK_WIDGET(iter->data);
			break;
		}
	}

	sink_widget = 
		g_object_new(PAMA_TYPE_SINK_WIDGET,
		             "context", priv->context, 
		             "sink", sink,
		             "group", group,
		             "icon-sizegroup", priv->icon_sizegroup,
		             NULL);

	g_signal_connect(sink_widget, "reorder-request", G_CALLBACK(pama_sink_popup_reorder_sinks), popup);
	
	gtk_box_pack_start(GTK_BOX(priv->sink_box), sink_widget, FALSE, FALSE, 0);
	pama_sink_popup_reorder_sinks(NULL, popup);
	gtk_widget_show_all(sink_widget);
}
static void pama_sink_popup_add_sink_input(PamaSinkPopup *popup, PamaPulseSinkInput *sink_input)
{
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);
	GtkWidget *sink_input_widget;
	
	gtk_widget_hide(priv->no_apps);
	
	sink_input_widget = 
		g_object_new(PAMA_TYPE_SINK_INPUT_WIDGET,
		             "context", priv->context, 
		             "sink-input", sink_input,
		             "icon-sizegroup", priv->icon_sizegroup,
		             NULL);

	g_signal_connect(sink_input_widget, "reorder-request", G_CALLBACK(pama_sink_popup_reorder_sink_inputs), popup);
	
	gtk_box_pack_start(GTK_BOX(priv->stream_box), sink_input_widget, FALSE, FALSE, 0);
	pama_sink_popup_reorder_sink_inputs(NULL, popup);
	gtk_widget_show_all(sink_input_widget);
}

static void pama_sink_popup_sink_added(PamaPulseContext *context, guint index, gpointer data)
{
	PamaSinkPopup *popup = PAMA_SINK_POPUP(data);
	PamaPulseSink *sink = pama_pulse_context_get_sink_by_index(context, index);

	pama_sink_popup_add_sink(popup, sink);
	pama_sink_popup_reorder_sinks(NULL, popup);
}
static void pama_sink_popup_sink_removed(PamaPulseContext *context, guint index, gpointer data)
{
	PamaSinkPopup *popup = PAMA_SINK_POPUP(data);
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);

	if (!pama_pulse_context_get_sinks(context))
		gtk_widget_show(priv->no_devices);
}
static void pama_sink_popup_sink_input_added(PamaPulseContext *context, guint index, gpointer data)
{
	PamaSinkPopup      *popup       = PAMA_SINK_POPUP(data);
	PamaPulseSinkInput *sink_input = pama_pulse_context_get_sink_input_by_index(context, index);

	pama_sink_popup_add_sink_input(popup, sink_input);
	pama_sink_popup_reorder_sink_inputs(NULL, popup);
}
static void pama_sink_popup_sink_input_removed(PamaPulseContext *context, guint index, gpointer data)
{
	PamaSinkPopup        *popup = PAMA_SINK_POPUP(data);
	PamaSinkPopupPrivate *priv  = PAMA_SINK_POPUP_GET_PRIVATE(popup);

	if (! pama_pulse_context_get_sink_inputs(context))
		gtk_widget_show(priv->no_apps);
}

static gint pama_sink_popup_reorder_sinks__compare_sinks(gconstpointer a, gconstpointer b)
{
	const GtkBoxChild *A = a, *B = b;

	if (!PAMA_IS_SINK_WIDGET(A->widget))
		return -1;
	if (!PAMA_IS_SINK_WIDGET(B->widget))
		return +1;

	return pama_sink_widget_compare(PAMA_SINK_WIDGET(A->widget), PAMA_SINK_WIDGET(B->widget));
}
static void pama_sink_popup_reorder_sinks(PamaSinkWidget *widget, gpointer data)
{
	PamaSinkPopup *popup = PAMA_SINK_POPUP(data);
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);
	GList *children = g_list_copy(GTK_BOX(priv->sink_box)->children);
	GList *iter;
	guint position;

	children = g_list_sort(children, pama_sink_popup_reorder_sinks__compare_sinks);

	for (iter = children, position = 0; iter; iter = iter->next, position++)
	{
		GtkBoxChild *child = iter->data;
		gtk_box_reorder_child(priv->sink_box, child->widget, position);
	}

	g_list_free(children);
}
static gint pama_sink_popup_reorder_sink_inputs__compare_sink_inputs(gconstpointer a, gconstpointer b)
{
	const GtkBoxChild *A = a, *B = b;

	if (!PAMA_IS_SINK_INPUT_WIDGET(A->widget))
		return -1;
	if (!PAMA_IS_SINK_INPUT_WIDGET(B->widget))
		return +1;

	return pama_sink_input_widget_compare(PAMA_SINK_INPUT_WIDGET(A->widget), PAMA_SINK_INPUT_WIDGET(B->widget));
}
static void pama_sink_popup_reorder_sink_inputs(PamaSinkInputWidget *widget, gpointer data)
{
	PamaSinkPopup *popup = PAMA_SINK_POPUP(data);
	PamaSinkPopupPrivate *priv = PAMA_SINK_POPUP_GET_PRIVATE(popup);
	GList *children = g_list_copy(GTK_BOX(priv->stream_box)->children);
	GList *iter;
	guint position;

	children = g_list_sort(children, pama_sink_popup_reorder_sink_inputs__compare_sink_inputs);

	for (iter = children, position = 0; iter; iter = iter->next, position++)
	{
		GtkBoxChild *child = iter->data;
		gtk_box_reorder_child(priv->stream_box, child->widget, position);
	}

	g_list_free(children);
}

PamaSinkPopup* pama_sink_popup_new(PamaPulseContext *context)
{
	return g_object_new(PAMA_TYPE_SINK_POPUP,
	                    "type", GTK_WINDOW_TOPLEVEL,
	                    "type-hint", GDK_WINDOW_TYPE_HINT_NORMAL,
	                    "decorated", FALSE,
	                    "resizable", FALSE,
	                    "skip-pager-hint", TRUE,
	                    "skip-taskbar-hint", TRUE,
	                    "title", "Playback Volume Widget",
	                    "icon-name", "multimedia-volume-widget",
	                    "context", context,
	                    NULL);
}
