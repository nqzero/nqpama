/*
 * pama-popup.c: A popup window that can align itself to a widget
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
#include <gdk/gdkkeysyms.h>

#include "pama-popup.h"

struct _PamaPopupPrivate
{
	GtkWidget *align_widget;
	PanelAppletOrient orientation;
};

G_DEFINE_TYPE(PamaPopup, pama_popup, GTK_TYPE_WINDOW);
#define PAMA_POPUP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), PAMA_TYPE_POPUP, PamaPopupPrivate))

static void pama_popup_map_event(GtkWidget *widget, GdkEvent *event, gpointer data);
static void pama_popup_size_allocated(GtkWidget *widget, GtkAllocation *allocation, gpointer data);
static void pama_popup_child_menu_hidden(GtkWidget *menu, gpointer data);
static void pama_popup_child_button_clicked(GtkButton *button, gpointer data);
static gboolean pama_popup_restore_grabs(PamaPopup *popup);
static gboolean pama_popup_grab_broken(GtkWidget *widget, GdkEventGrabBroken *event, gpointer data);
static gboolean pama_popup_button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean pama_popup_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void pama_popup_dispose(GObject *object);

static void pama_popup_class_init(PamaPopupClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->dispose  = pama_popup_dispose;

	g_type_class_add_private(klass, sizeof(PamaPopupPrivate));
}


static void pama_popup_init(PamaPopup *popup)
{
        gtk_widget_set_can_focus( GTK_WIDGET(popup), TRUE );
        gtk_widget_add_events( GTK_WIDGET(popup), GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK);
	g_object_connect(popup, 
	                 "signal::size-allocate",      G_CALLBACK(pama_popup_size_allocated), NULL,
	                 "signal::button-press-event", G_CALLBACK(pama_popup_button_pressed), NULL,
	                 "signal::key-press-event",    G_CALLBACK(pama_popup_key_pressed), NULL,
	                 "signal::grab-broken-event",  G_CALLBACK(pama_popup_grab_broken), NULL,
	                 NULL);
}




static void pama_popup_dispose(GObject *gobject)
{
	/*PamaPopup *popup = PAMA_POPUP(object);*/
	PamaPopupPrivate *priv = PAMA_POPUP_GET_PRIVATE(gobject);

	if (priv->align_widget)
	{
		g_object_unref(priv->align_widget);
		priv->align_widget = NULL;
	}

	G_OBJECT_CLASS(pama_popup_parent_class)->dispose(gobject);
}

void pama_popup_show(PamaPopup *popup)
{
	gtk_widget_show_all(GTK_WIDGET(popup));
	g_signal_connect_after(popup, "map-event", G_CALLBACK(pama_popup_map_event), NULL);
}

// Grab the pointer and keyboard.
// Returns true if the grabs succeeded; otherwise, false.
static gboolean pama_popup_restore_grabs(PamaPopup *popup)
{
	GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(popup));

        if (1) return TRUE;

	return (GDK_GRAB_SUCCESS == gdk_pointer_grab(window, TRUE, GDK_BUTTON_PRESS_MASK, NULL, NULL, GDK_CURRENT_TIME) &&
	        GDK_GRAB_SUCCESS == gdk_keyboard_grab(window, TRUE, GDK_CURRENT_TIME));
}

// Grab the pointer and keyboard when the popup becomes visible.
static void pama_popup_map_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	// Grab the pointer. If that fails, close the popup.
	if (!pama_popup_restore_grabs(PAMA_POPUP(widget)))
		gtk_widget_destroy(widget);
        gtk_widget_grab_focus( widget );
}

void pama_popup_set_popup_alignment(PamaPopup *popup, GtkWidget *align_widget, PanelAppletOrient orientation)
{
	PamaPopupPrivate *priv = PAMA_POPUP_GET_PRIVATE(popup);

	if (align_widget != priv->align_widget)
	{
		/* Unref the old alignment widget, if there was one */
		if (priv->align_widget)
		{
			g_object_unref(priv->align_widget);
		}

		priv->align_widget = align_widget;
		g_object_ref(priv->align_widget);
	}
	priv->orientation = orientation;

	pama_popup_realign(popup);
}

void pama_popup_realign(PamaPopup *popup)
{
	GtkRequisition window_req;
	int x, y;
	GdkScreen *screen;
	GdkRectangle monitor;
	int monitor_idx;
	PamaPopupPrivate *priv = PAMA_POPUP_GET_PRIVATE(popup);

	if (!priv->align_widget)
		return;
	
	gdk_window_get_origin(priv->align_widget->window, &x, &y);
	x += priv->align_widget->allocation.x;
	y += priv->align_widget->allocation.y;
	gtk_widget_size_request(GTK_WIDGET(popup), &window_req);
	
	screen = gtk_window_get_screen(GTK_WINDOW(popup));
	monitor_idx = gdk_screen_get_monitor_at_window(screen, priv->align_widget->window);
	gdk_screen_get_monitor_geometry(screen, monitor_idx, &monitor);
	
	switch (priv->orientation)
	{
		case PANEL_APPLET_ORIENT_UP:
			y -= window_req.height - 2;
			
			if (x + window_req.width > monitor.x + monitor.width)
				x = monitor.x + monitor.width - window_req.width;
			
			break;
			
		case PANEL_APPLET_ORIENT_DOWN:
			y += priv->align_widget->allocation.height + 2;
			
			if (x + window_req.width > monitor.x + monitor.width)
				x = monitor.x + monitor.width - window_req.width;
						
			break;
			
		case PANEL_APPLET_ORIENT_RIGHT:
			x += priv->align_widget->allocation.width + 2;
			
			if (y + window_req.height > monitor.y + monitor.height)
				y = monitor.y + monitor.height - window_req.height;
			
			break;
			
		case PANEL_APPLET_ORIENT_LEFT:
			x -= window_req.width - 2;
			
			if (y + window_req.height > monitor.y + monitor.height)
				y = monitor.y + monitor.height - window_req.height;
			
			break;
	}
	
	gtk_window_move(GTK_WINDOW(popup), x, y);
}

static void pama_popup_size_allocated(GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
	pama_popup_realign(PAMA_POPUP(widget));
}

// Add grab-broken listeners to children when they take one of the grabs.
static gboolean pama_popup_grab_broken(GtkWidget *widget, GdkEventGrabBroken *event, gpointer data)
{	
	if (event->grab_window)
	{
		// Listen for events that signal the child object's grab is no longer needed. 
		gpointer child_widget;
		gdk_window_get_user_data(event->grab_window, &child_widget);
		if (GTK_IS_BUTTON(child_widget) && event->keyboard)
		{
			// Is a button; restore grabs when the key is released (buttons seem not to affect the pointer grab)
			//g_signal_connect(child_widget, "key-release-event", G_CALLBACK(pama_popup_child_button_key_released), widget);
			g_signal_connect(child_widget, "clicked", G_CALLBACK(pama_popup_child_button_clicked), widget);
		}
		else if (GTK_IS_MENU(child_widget))
		{
			// Is a menu; restore grabs when it is unmapped
			g_signal_connect(child_widget, "hide", G_CALLBACK(pama_popup_child_menu_hidden), widget);
		}
	}
	else
	{
		// Grab was taken by another application. If we can't get the grab back, then close the popup.
		if (!pama_popup_restore_grabs(PAMA_POPUP(widget)))
			gtk_widget_destroy(widget);
	}

	return TRUE;
}

// Restore grabs when a menu opened by a child widget is closed.
static void pama_popup_child_menu_hidden(GtkWidget *menu, gpointer data)
{
	if (!pama_popup_restore_grabs(PAMA_POPUP(data)))
		gtk_widget_destroy(GTK_WIDGET(data));
}

// Restore grabs when a button in the popup was pressed with the keyboard.
static void pama_popup_child_button_clicked(GtkButton *button, gpointer data)
{
	if (!pama_popup_restore_grabs(PAMA_POPUP(data)))
		gtk_widget_destroy(GTK_WIDGET(data));
}


// Check whether the mouse was clicked outside the popup, and close it if it was.
static gboolean pama_popup_button_pressed(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkAllocation popupAllocation;
	gtk_widget_get_allocation(widget, &popupAllocation);

        if (TRUE) {
            gtk_widget_destroy( widget );
            return TRUE;
        }

	if (event->x < 0 || popupAllocation.width  < event->x ||
	    event->y < 0 || popupAllocation.height < event->y  )
	{
		// Button pressed outside the popup, so destroy it.
		gtk_widget_destroy(widget);
		return TRUE;
	}

	return FALSE;
}

// Allow closing the popup by pressing ESC
static gboolean pama_popup_key_pressed(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_Escape)
	{
		gtk_widget_destroy(widget);
		return TRUE;
	}

	return FALSE;
}
