/*
 * pama-source-popup.c: A popup window that displays all sources and source outputs
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
 
#include "pama-source-popup.h"
#include "pama-source-widget.h"
#include "pama-source-output-widget.h"

static void     pama_source_popup_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);
static GObject* pama_source_popup_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties);
static void     pama_source_popup_dispose(GObject *gobject);
static void     pama_source_popup_weak_ref_notify(gpointer data, GObject *where_the_object_was);

static void     pama_source_popup_source_added(PamaPulseContext *context, guint index, gpointer data);
static void     pama_source_popup_source_removed(PamaPulseContext *context, guint index, gpointer data);
static void     pama_source_popup_source_output_added(PamaPulseContext *context, guint index, gpointer data);
static void     pama_source_popup_source_output_removed(PamaPulseContext *context, guint index, gpointer data);

static void     pama_source_popup_reorder_sources       (PamaSourceWidget      *widget, gpointer data);
static void     pama_source_popup_reorder_source_outputs(PamaSourceOutputWidget *widget, gpointer data);

static void     pama_source_popup_add_source       (PamaSourcePopup *popup, PamaPulseSource      *source);
static void     pama_source_popup_add_source_output(PamaSourcePopup *popup, PamaPulseSourceOutput *source_output);

struct _PamaSourcePopupPrivate
{
	GtkBox *source_box, *stream_box;
	GtkSizeGroup *icon_sizegroup;
	GtkWidget *no_apps, *no_devices;

	PamaPulseContext *context;
	gulong source_added_handler_id;
	gulong source_removed_handler_id;
	gulong source_output_added_handler_id;
	gulong source_output_removed_handler_id;
};

G_DEFINE_TYPE(PamaSourcePopup, pama_source_popup, PAMA_TYPE_POPUP);
#define PAMA_SOURCE_POPUP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_SOURCE_POPUP, PamaSourcePopupPrivate))

enum
{
	PROP_0,

	PROP_CONTEXT
};

static void pama_source_popup_class_init(PamaSourcePopupClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->set_property = pama_source_popup_set_property;
	gobject_class->constructor  = pama_source_popup_constructor;
	gobject_class->dispose      = pama_source_popup_dispose;

	pspec = g_param_spec_object("context",
	                            "Pulse context object",
	                            "The PamaPulseContext which this source belongs to.",
	                            PAMA_TYPE_PULSE_CONTEXT,
	                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);

	g_type_class_add_private(klass, sizeof(PamaSourcePopupPrivate));
}

static void pama_source_popup_init(PamaSourcePopup *popup)
{
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);
	gchar *markup;
	GtkWidget *frame, *main_box;
	GtkWidget *source_frame,   *source_align,   *source_box;
	GtkWidget *stream_frame, *stream_align, *stream_box;
	GtkWidget *no_apps, *no_devices;
	GtkSizeGroup *icon_sizegroup;

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_container_add(GTK_CONTAINER(popup), frame);

	main_box = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(main_box), 6);
	gtk_container_add(GTK_CONTAINER(frame), main_box);


	source_frame = gtk_frame_new("");
	markup = g_markup_printf_escaped("<b>%s</b>", _("Audio Sources"));
	gtk_label_set_markup(GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(source_frame))), markup);
	gtk_frame_set_shadow_type(GTK_FRAME(source_frame), GTK_SHADOW_NONE);
	gtk_box_pack_start(GTK_BOX(main_box), source_frame, FALSE, FALSE, 0);
	g_free(markup);
	
	source_align = gtk_alignment_new(0,0,1,0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(source_align), 6, 0, 12, 0);
	gtk_container_add(GTK_CONTAINER(source_frame), source_align);

	source_box = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(source_align), source_box);
	priv->source_box = GTK_BOX(source_box);

	markup = g_markup_printf_escaped("<i>%s</i>", _("No audio sources available"));
	no_devices = g_object_new(GTK_TYPE_LABEL,
	                       "ellipsize", PANGO_ELLIPSIZE_MIDDLE, 
	                       "wrap", FALSE, 
	                       "xalign", 0.0, 
	                       "label", markup, 
	                       "use-markup", TRUE, 
	                       "no-show-all", TRUE, 
	                       NULL);
	gtk_widget_show(no_devices);
	gtk_box_pack_start(GTK_BOX(source_box), no_devices, FALSE, FALSE, 0);
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

	markup = g_markup_printf_escaped("<i>%s</i>", _("No applications recording"));
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

static GObject* pama_source_popup_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	GObject *gobject = G_OBJECT_CLASS(pama_source_popup_parent_class)->constructor(gtype, n_properties, properties);
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(gobject);
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);
	GSList *iter;

	if (NULL == priv->context)
		g_error("An attempt was made to construct a PamaSourcePopup without providing a valid PamaPulseContext.");

	for (iter = pama_pulse_context_get_sources(priv->context); iter; iter = iter->next)
	{
		PamaPulseSource *source = PAMA_PULSE_SOURCE(iter->data);

		pama_source_popup_add_source(popup, source);
	}

	for (iter = pama_pulse_context_get_source_outputs(priv->context); iter; iter = iter->next)
	{
		PamaPulseSourceOutput *source_output = PAMA_PULSE_SOURCE_OUTPUT(iter->data);

		pama_source_popup_add_source_output(popup, source_output);
	}

	pama_source_popup_reorder_sources(NULL, popup);
	priv->source_added_handler_id       = g_signal_connect(priv->context, "source-added",       G_CALLBACK(pama_source_popup_source_added),       popup);
	priv->source_removed_handler_id     = g_signal_connect(priv->context, "source-removed",     G_CALLBACK(pama_source_popup_source_removed),     popup);
	priv->source_output_added_handler_id = g_signal_connect(priv->context, "source-output-added", G_CALLBACK(pama_source_popup_source_output_added), popup);
	priv->source_output_removed_handler_id = g_signal_connect(priv->context, "source-output-removed", G_CALLBACK(pama_source_popup_source_output_removed), popup);

	gtk_widget_show_all(GTK_BIN(popup)->child);

	return gobject;
}
static void pama_source_popup_dispose(GObject *gobject)
{
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(gobject);
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);

	if (priv->context)
	{
		if (priv->source_added_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->source_added_handler_id);
			priv->source_added_handler_id = 0;
		}

		if (priv->source_removed_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->source_removed_handler_id);
			priv->source_removed_handler_id = 0;
		}

		if (priv->source_output_added_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->source_output_added_handler_id);
			priv->source_output_added_handler_id = 0;
		}

		if (priv->source_output_removed_handler_id)
		{
			g_signal_handler_disconnect(priv->context, priv->source_output_removed_handler_id);
			priv->source_output_removed_handler_id = 0;
		}

		g_object_weak_unref(G_OBJECT(priv->context), pama_source_popup_weak_ref_notify, popup);
		priv->context = NULL;
	}

	G_OBJECT_CLASS(pama_source_popup_parent_class)->dispose(gobject);
}
static void pama_source_popup_weak_ref_notify(gpointer data, GObject *where_the_object_was)
{
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(data);
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);
	if ((GObject *)priv->context == where_the_object_was)
	{
		priv->context = NULL;
	}

	gtk_object_destroy(GTK_OBJECT(popup));
}

static void pama_source_popup_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(gobject);
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);

	switch(property_id)
	{
		case PROP_CONTEXT:
			priv->context = g_value_get_object(value);
			g_object_weak_ref(G_OBJECT(priv->context), pama_source_popup_weak_ref_notify, popup);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
			break;
	}
}

static void  pama_source_popup_add_source(PamaSourcePopup *popup, PamaPulseSource *source)
{
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);
	GtkWidget *group = NULL;
	GtkWidget *source_widget;
	GList *iter;
	GList *box_children = gtk_container_get_children(GTK_CONTAINER(priv->source_box));
	
	gtk_widget_hide(priv->no_devices);

	/* If there is already a source widget, group with it */
	for (iter = box_children; iter; iter = iter->next)
	{
		if (PAMA_IS_SOURCE_WIDGET(iter->data))
		{
			group = GTK_WIDGET(iter->data);
			break;
		}
	}

	source_widget = 
		g_object_new(PAMA_TYPE_SOURCE_WIDGET,
		             "context", priv->context, 
		             "source", source,
		             "group", group,
		             "icon-sizegroup", priv->icon_sizegroup,
		             NULL);

	g_signal_connect(source_widget, "reorder-request", G_CALLBACK(pama_source_popup_reorder_sources), popup);
	
	gtk_box_pack_start(GTK_BOX(priv->source_box), source_widget, FALSE, FALSE, 0);
	pama_source_popup_reorder_sources(NULL, popup);
	gtk_widget_show_all(source_widget);
}
static void pama_source_popup_add_source_output(PamaSourcePopup *popup, PamaPulseSourceOutput *source_output)
{
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);
	GtkWidget *source_output_widget;
	
	gtk_widget_hide(priv->no_apps);

	source_output_widget = 
		g_object_new(PAMA_TYPE_SOURCE_OUTPUT_WIDGET,
		             "context", priv->context, 
		             "source-output", source_output,
		             "icon-sizegroup", priv->icon_sizegroup,
		             NULL);

	g_signal_connect(source_output_widget, "reorder-request", G_CALLBACK(pama_source_popup_reorder_source_outputs), popup);
	
	gtk_box_pack_start(GTK_BOX(priv->stream_box), source_output_widget, FALSE, FALSE, 0);
	pama_source_popup_reorder_source_outputs(NULL, popup);
	gtk_widget_show_all(source_output_widget);
}

static void pama_source_popup_source_added(PamaPulseContext *context, guint index, gpointer data)
{
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(data);
	PamaPulseSource *source = pama_pulse_context_get_source_by_index(context, index);

	pama_source_popup_add_source(popup, source);
	pama_source_popup_reorder_sources(NULL, popup);
}
static void pama_source_popup_source_removed(PamaPulseContext *context, guint index, gpointer data)
{
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(data);
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);

	if (!pama_pulse_context_get_sources(context))
		gtk_widget_show(priv->no_devices);
}
static void pama_source_popup_source_output_added(PamaPulseContext *context, guint index, gpointer data)
{
	PamaSourcePopup      *popup       = PAMA_SOURCE_POPUP(data);
	PamaPulseSourceOutput *source_output = pama_pulse_context_get_source_output_by_index(context, index);

	pama_source_popup_add_source_output(popup, source_output);
	pama_source_popup_reorder_source_outputs(NULL, popup);
}
static void pama_source_popup_source_output_removed(PamaPulseContext *context, guint index, gpointer data)
{
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(data);
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);

	if (!pama_pulse_context_get_source_outputs(context))
		gtk_widget_show(priv->no_apps);
}

static gint pama_source_popup_reorder_sources__compare_sources(gconstpointer a, gconstpointer b)
{
	const GtkBoxChild *A = a, *B = b;

	if (!PAMA_IS_SOURCE_WIDGET(A->widget))
		return -1;
	if (!PAMA_IS_SOURCE_WIDGET(B->widget))
		return +1;

	return pama_source_widget_compare(PAMA_SOURCE_WIDGET(A->widget), PAMA_SOURCE_WIDGET(B->widget));
}
static void pama_source_popup_reorder_sources(PamaSourceWidget *widget, gpointer data)
{
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(data);
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);
	GList *children = g_list_copy(GTK_BOX(priv->source_box)->children);
	GList *iter;
	guint position;

	children = g_list_sort(children, pama_source_popup_reorder_sources__compare_sources);

	for (iter = children, position = 0; iter; iter = iter->next, position++)
	{
		GtkBoxChild *child = iter->data;
		gtk_box_reorder_child(priv->source_box, child->widget, position);
	}

	g_list_free(children);
}
static gint pama_source_popup_reorder_source_outputs__compare_source_outputs(gconstpointer a, gconstpointer b)
{
	const GtkBoxChild *A = a, *B = b;

	if (!PAMA_IS_SOURCE_OUTPUT_WIDGET(A->widget))
		return -1;
	if (!PAMA_IS_SOURCE_OUTPUT_WIDGET(B->widget))
		return +1;

	return pama_source_output_widget_compare(PAMA_SOURCE_OUTPUT_WIDGET(A->widget), PAMA_SOURCE_OUTPUT_WIDGET(B->widget));
}
static void pama_source_popup_reorder_source_outputs(PamaSourceOutputWidget *widget, gpointer data)
{
	PamaSourcePopup *popup = PAMA_SOURCE_POPUP(data);
	PamaSourcePopupPrivate *priv = PAMA_SOURCE_POPUP_GET_PRIVATE(popup);
	GList *children = g_list_copy(GTK_BOX(priv->stream_box)->children);
	GList *iter;
	guint position;

	children = g_list_sort(children, pama_source_popup_reorder_source_outputs__compare_source_outputs);

	for (iter = children, position = 0; iter; iter = iter->next, position++)
	{
		GtkBoxChild *child = iter->data;
		gtk_box_reorder_child(priv->stream_box, child->widget, position);
	}

	g_list_free(children);
}

PamaSourcePopup* pama_source_popup_new(PamaPulseContext *context)
{
	return g_object_new(PAMA_TYPE_SOURCE_POPUP,
	                    "type", GTK_WINDOW_TOPLEVEL,
	                    "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
	                    "decorated", FALSE,
	                    "resizable", FALSE,
	                    "skip-pager-hint", TRUE,
	                    "skip-taskbar-hint", TRUE,
	                    "title", "Playback Volume Widget",
	                    "icon-name", "multimedia-volume-widget",
	                    "context", context,
	                    NULL);
}
