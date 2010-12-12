/*
 * pama-pulse-context.c: A GObject wrapper for a PulseAudio context
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
 
#include "pama-pulse-context.h"

#define PAMA_PULSE_CONTEXT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), PAMA_TYPE_PULSE_CONTEXT, PamaPulseContextPrivate))
G_DEFINE_TYPE(PamaPulseContext, pama_pulse_context, G_TYPE_OBJECT);

struct _PamaPulseContextPrivate
{
	pa_mainloop_api *api;
	pa_context      *context;
	
	GString         *default_sink_name;
	GString         *default_source_name;
	GString         *hostname;

	GSList          *sinks;
	GSList          *sources;
	GSList          *sink_inputs;
	GSList          *source_outputs;
	GSList          *clients;
};



static void pama_pulse_context_dispose(GObject *gobject);
static void pama_pulse_context_finalize(GObject *gobject);
static void pama_pulse_context_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec);
static void pama_pulse_context_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec);
static GObject* pama_pulse_context_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties);

static gboolean fail_later(gpointer data);
static void state_cb(pa_context *c, void *data);
static void subscribe_cb(pa_context *c, pa_subscription_event_type_t type, uint32_t index, void *data);
static void server_cb(pa_context *c, const pa_server_info *i, void *data);
static void sink_input_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *data);
static void source_output_cb(pa_context *c, const pa_source_output_info *i, int eol, void *data);
static void client_cb(pa_context *c, const pa_client_info *i, int eol, void *data);
static void source_cb(pa_context *c, const pa_source_info *i, int eol, void *data);
static void sink_cb(pa_context *c, const pa_sink_info *i, int eol, void *data);



enum
{
	PROP_NONE,
	
	PROP_DEFAULT_SINK,
	PROP_DEFAULT_SOURCE,
	PROP_CONTEXT,
	PROP_API,
	PROP_HOSTNAME
};
enum
{
	CONNECTED_SIGNAL,
	DISCONNECTED_SIGNAL,
	CLIENT_ADDED_SIGNAL,
	CLIENT_REMOVED_SIGNAL,
	SINK_ADDED_SIGNAL,
	SINK_REMOVED_SIGNAL,
	SINK_INPUT_ADDED_SIGNAL,
	SINK_INPUT_REMOVED_SIGNAL,
	SOURCE_ADDED_SIGNAL,
	SOURCE_REMOVED_SIGNAL,
	SOURCE_OUTPUT_ADDED_SIGNAL,
	SOURCE_OUTPUT_REMOVED_SIGNAL,
	LAST_SIGNAL
};
static guint context_signals[LAST_SIGNAL] = {0,};

static void pama_pulse_context_class_init(PamaPulseContextClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;
	
	gobject_class->dispose = pama_pulse_context_dispose;
	gobject_class->finalize = pama_pulse_context_finalize;
	gobject_class->get_property = pama_pulse_context_get_property;
	gobject_class->set_property = pama_pulse_context_set_property;
	gobject_class->constructor  = pama_pulse_context_constructor;
	
	pspec = g_param_spec_string("default-sink-name",
								"Default sink name",
								"The name of the default playback device",
								"",
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_DEFAULT_SINK, pspec);

	pspec = g_param_spec_string("default-source-name",
								"Default source name",
								"The name of the default recording device",
								"",
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_DEFAULT_SOURCE, pspec);

	pspec = g_param_spec_pointer("context",
								 "Pulse context object",
								 "The underlying pa_context structure",
								 G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_CONTEXT, pspec);	

	pspec = g_param_spec_pointer("api",
								 "Pulse api object",
								 "The pa_api reference to use when connecting to the PulseAudio server",
								 G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_API, pspec);	
	
	pspec = g_param_spec_string("hostname",
								"Host name",
								"The hostname of the machine the daemon is running on",
								"",
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(gobject_class, PROP_HOSTNAME, pspec);

	context_signals[CONNECTED_SIGNAL] =
		g_signal_new("connected",
		             G_TYPE_FROM_CLASS(gobject_class),
		             G_SIGNAL_RUN_LAST,
		             0,
		             NULL,
		             NULL,
		             g_cclosure_marshal_VOID__VOID,
		             G_TYPE_NONE,
		             0);
	
	context_signals[DISCONNECTED_SIGNAL] =
		g_signal_new("disconnected",
		             G_TYPE_FROM_CLASS(gobject_class),
		             G_SIGNAL_RUN_LAST,
		             0,
		             NULL,
		             NULL,
		             g_cclosure_marshal_VOID__VOID,
		             G_TYPE_NONE,
		             0);
	
	context_signals[CLIENT_ADDED_SIGNAL] =
		g_signal_new("client-added",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	context_signals[CLIENT_REMOVED_SIGNAL] =
		g_signal_new("client-removed",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	context_signals[SINK_ADDED_SIGNAL] =
		g_signal_new("sink-added",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	context_signals[SINK_REMOVED_SIGNAL] =
		g_signal_new("sink-removed",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	
	context_signals[SINK_INPUT_ADDED_SIGNAL] =
		g_signal_new("sink-input-added",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	context_signals[SINK_INPUT_REMOVED_SIGNAL] =
		g_signal_new("sink-input-removed",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	context_signals[SOURCE_ADDED_SIGNAL] =
		g_signal_new("source-added",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	context_signals[SOURCE_REMOVED_SIGNAL] =
		g_signal_new("source-removed",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	
	context_signals[SOURCE_OUTPUT_ADDED_SIGNAL] =
		g_signal_new("source-output-added",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	context_signals[SOURCE_OUTPUT_REMOVED_SIGNAL] =
		g_signal_new("source-output-removed",
					 G_TYPE_FROM_CLASS(gobject_class),
					 G_SIGNAL_RUN_LAST,
					 0,
					 NULL,
					 NULL,
					 g_cclosure_marshal_VOID__UINT,
					 G_TYPE_NONE,
					 1,
					 G_TYPE_UINT);
	
	g_type_class_add_private (klass, sizeof (PamaPulseContextPrivate));
}

static void pama_pulse_context_init(PamaPulseContext *self)
{
	self->priv = PAMA_PULSE_CONTEXT_GET_PRIVATE(self);
	
	self->priv->default_sink_name = g_string_new("");
	self->priv->default_source_name = g_string_new("");
	self->priv->hostname = g_string_new("");
}

static GObject* pama_pulse_context_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
	GObject *obj = G_OBJECT_CLASS(pama_pulse_context_parent_class)->constructor(gtype, n_properties, properties);
	PamaPulseContext *self = PAMA_PULSE_CONTEXT(obj);

	if (NULL == self->priv->api)
		g_error("An attempt was made to construct a PamaPulseContext without specifying its 'api' property.");
	
	pa_proplist *proplist = pa_proplist_new();
	pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, _("PulseAudio Mixer Applet"));
	pa_proplist_sets(proplist, PA_PROP_APPLICATION_ID, "net.launchpad.pama");
	pa_proplist_sets(proplist, PA_PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
	pa_proplist_sets(proplist, PA_PROP_APPLICATION_VERSION, PACKAGE_VERSION);

	self->priv->context = pa_context_new_with_proplist(self->priv->api, NULL, proplist);
	pa_context_set_state_callback(self->priv->context, state_cb, self);
	
	if (0 > pa_context_connect(self->priv->context, NULL, (pa_context_flags_t) 0, NULL))
		g_idle_add(fail_later, self);

	return obj;
}

static gboolean fail_later(gpointer data)
{
	/* Emit the 'disconnected' signal the next time the glib main loop runs */
	PamaPulseContext *self = data;
	g_signal_emit(self, context_signals[DISCONNECTED_SIGNAL], 0);
	return FALSE;
}

static void pama_pulse_context_get_property(GObject *gobject, guint property_id,       GValue *value, GParamSpec *pspec)
{
	PamaPulseContext *self = PAMA_PULSE_CONTEXT(gobject);
	
	switch (property_id)
	{
		case PROP_DEFAULT_SOURCE:
			g_value_set_string(value, self->priv->default_source_name->str);
			break;
		
		case PROP_DEFAULT_SINK:
			g_value_set_string(value, self->priv->default_sink_name->str);
			break;
		
		case PROP_CONTEXT:
			g_value_set_pointer(value, self->priv->context);
			break;
			
		case PROP_HOSTNAME:
			g_value_set_string(value, self->priv->hostname->str);
			break;
		
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
			break;
	}
}
static void pama_pulse_context_set_property(GObject *gobject, guint property_id, const GValue *value, GParamSpec *pspec)
{
	PamaPulseContext *self = PAMA_PULSE_CONTEXT(gobject);
	
	switch (property_id)
	{
		case PROP_DEFAULT_SOURCE:
			g_string_assign(self->priv->default_source_name, g_value_get_string(value));
			break;
			
		case PROP_DEFAULT_SINK:
			g_string_assign(self->priv->default_sink_name, g_value_get_string(value));
			break;
		
		case PROP_API:
			self->priv->api = g_value_get_pointer(value);
			break;
			
		case PROP_HOSTNAME:
			g_string_assign(self->priv->hostname, g_value_get_string(value));
			break;
		
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, property_id, pspec);
			break;
	}
}

static void pama_pulse_context_dispose(GObject *gobject)
{
	GSList *iter;
	PamaPulseContext *self = PAMA_PULSE_CONTEXT(gobject);
	
	if (self->priv->clients)
	{
		for (iter = self->priv->clients; iter; iter = iter->next)
			g_object_unref(iter->data);
		g_slist_free(self->priv->clients);
		self->priv->clients = NULL;
	}
	
	if (self->priv->sources)
	{
		for (iter = self->priv->sources; iter; iter = iter->next)
			g_object_unref(iter->data);
		g_slist_free(self->priv->sources);
		self->priv->sources = NULL;
	}
	
	if (self->priv->sinks)
	{
		for (iter = self->priv->sinks; iter; iter = iter->next)
			g_object_unref(iter->data);
		g_slist_free(self->priv->sinks);
		self->priv->sinks = NULL;
	}
	
	if (self->priv->sink_inputs)
	{
		for (iter = self->priv->sink_inputs; iter; iter = iter->next)
			g_object_unref(iter->data);
		g_slist_free(self->priv->sink_inputs);
		self->priv->sink_inputs = NULL;
	}
	
	if (self->priv->source_outputs)
	{
		for (iter = self->priv->source_outputs; iter; iter = iter->next)
			g_object_unref(iter->data);
		g_slist_free(self->priv->source_outputs);
		self->priv->source_outputs = NULL;
	}
	
	
	
	G_OBJECT_CLASS(pama_pulse_context_parent_class)->dispose(gobject);
}

static void pama_pulse_context_finalize(GObject *gobject)
{
	PamaPulseContext *self = PAMA_PULSE_CONTEXT(gobject);
	
	pa_context_unref(self->priv->context);
	g_string_free(self->priv->default_sink_name, TRUE);
	g_string_free(self->priv->default_source_name, TRUE);
	g_string_free(self->priv->hostname, TRUE);
	
	G_OBJECT_CLASS(pama_pulse_context_parent_class)->finalize(gobject);
}


/* Methods */
PamaPulseContext* pama_pulse_context_new(pa_mainloop_api *api)
{
	g_warn_if_fail(api);

	return g_object_new(PAMA_TYPE_PULSE_CONTEXT, 
	                    "api", api, 
	                    NULL);
}


GSList*        pama_pulse_context_get_sinks(PamaPulseContext *context)
{
	return context->priv->sinks;
}
PamaPulseSink* pama_pulse_context_get_sink_by_index(PamaPulseContext *context, const uint32_t index)
{
	GSList *match = g_slist_find_custom(context->priv->sinks, &index, pama_pulse_sink_compare_to_index);
	if (match)
		return PAMA_PULSE_SINK(match->data);
	else
		return NULL;
}
PamaPulseSink* pama_pulse_context_get_sink_by_name(PamaPulseContext *context, const char *name)
{
	GSList *match = g_slist_find_custom(context->priv->sinks, name, pama_pulse_sink_compare_to_name);
	if (match)
		return PAMA_PULSE_SINK(match->data);
	else
		return NULL;
}
PamaPulseSink* pama_pulse_context_get_default_sink(PamaPulseContext *context)
{
	return pama_pulse_context_get_sink_by_name(context, context->priv->default_sink_name->str);
}

GSList*          pama_pulse_context_get_sources(PamaPulseContext *context)
{
	return context->priv->sources;
}
PamaPulseSource* pama_pulse_context_get_source_by_index(PamaPulseContext *context, const uint32_t index)
{
	GSList *match = g_slist_find_custom(context->priv->sources, &index, pama_pulse_source_compare_to_index);
	if (match)
		return PAMA_PULSE_SOURCE(match->data);
	else
		return NULL;
}
PamaPulseSource* pama_pulse_context_get_source_by_name(PamaPulseContext *context, const char *name)
{
	GSList *match = g_slist_find_custom(context->priv->sources, name, pama_pulse_source_compare_to_name);
	if (match)
		return PAMA_PULSE_SOURCE(match->data);
	else
		return NULL;
}
PamaPulseSource* pama_pulse_context_get_default_source(PamaPulseContext *context)
{
	return pama_pulse_context_get_source_by_name(context, context->priv->default_source_name->str);
}

GSList*             pama_pulse_context_get_sink_inputs(PamaPulseContext *context)
{
	return context->priv->sink_inputs;
}
PamaPulseSinkInput* pama_pulse_context_get_sink_input_by_index(PamaPulseContext *context, const uint32_t index)
{
	GSList *match = g_slist_find_custom(context->priv->sink_inputs, &index, pama_pulse_sink_input_compare_to_index);
	if (match)
		return PAMA_PULSE_SINK_INPUT(match->data);
	else
		return NULL;
}


GSList*                pama_pulse_context_get_source_outputs(PamaPulseContext *context)
{
	return context->priv->source_outputs;
}
PamaPulseSourceOutput* pama_pulse_context_get_source_output_by_index(PamaPulseContext *context, const uint32_t index)
{
	GSList *match = g_slist_find_custom(context->priv->source_outputs, &index, pama_pulse_source_output_compare_to_index);
	if (match)
		return PAMA_PULSE_SOURCE_OUTPUT(match->data);
	else
		return NULL;
}

GSList*          pama_pulse_context_get_clients(PamaPulseContext *context)
{
	return context->priv->clients;
}
PamaPulseClient* pama_pulse_context_get_client_by_index(PamaPulseContext *context, const guint32 index)
{
	GSList *match = g_slist_find_custom(context->priv->clients, &index, pama_pulse_client_compare_to_index);
	if (match)
		return PAMA_PULSE_CLIENT(match->data);
	else
		return NULL;
}



/* Pulse Audio callbacks */

static void state_cb        (pa_context *c, void *data)
{
	PamaPulseContext *self = data;
	pa_operation *o;
	
	switch (pa_context_get_state(c))
	{
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
			
		case PA_CONTEXT_READY: 
			g_signal_emit(self, context_signals[CONNECTED_SIGNAL], 0);
			
			pa_context_set_subscribe_callback(c, subscribe_cb, self);
			
			o = pa_context_subscribe(c, 
									  PA_SUBSCRIPTION_MASK_SINK |
			                          PA_SUBSCRIPTION_MASK_SOURCE |
			                          PA_SUBSCRIPTION_MASK_SINK_INPUT |
			                          PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT |
			                          PA_SUBSCRIPTION_MASK_CLIENT |
			                          PA_SUBSCRIPTION_MASK_SERVER,
			                         NULL, 
									 NULL);
			if (o)
				pa_operation_unref(o);
			
			o = pa_context_get_server_info(c, server_cb, self);
			if (o)
				pa_operation_unref(o);
			
			o = pa_context_get_client_info_list(c, client_cb, self);
			if (o)
				pa_operation_unref(o);
			
			o = pa_context_get_source_info_list(c, source_cb, self);
			if (o)
				pa_operation_unref(o);
			
			o = pa_context_get_sink_info_list(c, sink_cb, self);
			if (o)
				pa_operation_unref(o);
			
			o = pa_context_get_sink_input_info_list(c, sink_input_cb, self);
			if (o)
				pa_operation_unref(o);
			
			o = pa_context_get_source_output_info_list(c, source_output_cb, self);
			if (o)
				pa_operation_unref(o);
			
			break;
		
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			
			g_signal_emit(self, context_signals[DISCONNECTED_SIGNAL], 0);
			break;
	}
}
static void subscribe_cb    (pa_context *c, pa_subscription_event_type_t type, uint32_t index, void *data)
{
	PamaPulseContext *self = data;
	
	switch (type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)
	{
		case PA_SUBSCRIPTION_EVENT_SERVER:
		{
			pa_operation *o = pa_context_get_server_info(c, server_cb, self);
		
			if (o)
				pa_operation_unref(o);
			
			break;			
		}
			
		case PA_SUBSCRIPTION_EVENT_SINK:
			if (PA_SUBSCRIPTION_EVENT_REMOVE & type)
			{
				PamaPulseSink *sink = pama_pulse_context_get_sink_by_index(self, index);
				if (sink)
				{
					self->priv->sinks = g_slist_remove(self->priv->sinks, sink);
					g_signal_emit(self, context_signals[SINK_REMOVED_SIGNAL], 0, (guint)index);
					g_object_unref(sink);
				}
			}
			else
			{
				pa_operation *o = pa_context_get_sink_info_by_index(c, index, sink_cb, self);
			
				if (o)
					pa_operation_unref(o);
			}
			
			break;
			
		case PA_SUBSCRIPTION_EVENT_SOURCE:
			if (PA_SUBSCRIPTION_EVENT_REMOVE & type)
			{
				PamaPulseSource *source = pama_pulse_context_get_source_by_index(self, index);
				if (source)
				{
					self->priv->sources = g_slist_remove(self->priv->sources, source);
					g_signal_emit(self, context_signals[SOURCE_REMOVED_SIGNAL], 0, (guint)index);
					g_object_unref(source);
				}
			}
			else
			{
				pa_operation *o = pa_context_get_source_info_by_index(c, index, source_cb, self);
			
				if (o)
					pa_operation_unref(o);
			}
			
			break;
	
		case PA_SUBSCRIPTION_EVENT_CLIENT:
			if (PA_SUBSCRIPTION_EVENT_REMOVE & type)
			{
				PamaPulseClient *client = pama_pulse_context_get_client_by_index(self, index);
				if (client)
				{
					self->priv->clients = g_slist_remove(self->priv->clients, client);
					g_signal_emit(self, context_signals[CLIENT_REMOVED_SIGNAL], 0, (guint)index);
					g_object_unref(client);
				}
			}
			else
			{
				pa_operation *o = pa_context_get_client_info(c, index, client_cb, self);
			
				if (o)
					pa_operation_unref(o);
			}

			break;
		
		case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
			if (PA_SUBSCRIPTION_EVENT_REMOVE & type)
			{
				PamaPulseSinkInput *sink_input = pama_pulse_context_get_sink_input_by_index(self, index);
				if (sink_input)
				{
					self->priv->sink_inputs = g_slist_remove(self->priv->sink_inputs, sink_input);
					g_signal_emit(self, context_signals[SINK_INPUT_REMOVED_SIGNAL], 0, (guint)index);
					g_object_unref(sink_input);
				}
			}
			else
			{
				pa_operation *o = pa_context_get_sink_input_info(c, index, sink_input_cb, self);
				
				if (o)
					pa_operation_unref(o);
			}
			
			break;
			
		case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
			if (PA_SUBSCRIPTION_EVENT_REMOVE & type)
			{
				PamaPulseSourceOutput *source_output = pama_pulse_context_get_source_output_by_index(self, index);
				if (source_output)
				{
					self->priv->source_outputs = g_slist_remove(self->priv->source_outputs, source_output);
					g_signal_emit(self, context_signals[SOURCE_OUTPUT_REMOVED_SIGNAL], 0, (guint)index);
					g_object_unref(source_output);
				}
			}
			else
			{
				pa_operation *o = pa_context_get_source_output_info(c, index, source_output_cb, self);
			
				if (o)
					pa_operation_unref(o);
			}
			
			break;
	
		default:
		{
			g_warning("subscribe callback (unhandled; type=%d)\n", type);
			break;
		}
	}
}

static void sink_cb         (pa_context *c, const pa_sink_info          *i, int eol, void *data)
{
	if (eol)
		return;

	PamaPulseContext *self = data;
	PamaPulseSink    *sink = pama_pulse_context_get_sink_by_index(self, i->index);

	/* Networked devices have the remote username/hostname appended to them, which I don't want */
	gchar *description = NULL;
	gchar *hostname = NULL;
	if (i->flags & PA_SINK_NETWORK)
	{
		description = (gchar *) pa_proplist_gets(i->proplist, "tunnel.remote.description");
		hostname = (gchar *) pa_proplist_gets(i->proplist, "tunnel.remote.fqdn");
	}
	if (! description)
		description = (gchar *) i->description;

	if (sink)
	{
		g_object_set(sink, 
		             "volume",      (guint)    pa_cvolume_avg(&i->volume),
		             "base-volume", (guint)    i->base_volume,
		             "channels",    (guchar)   i->channel_map.channels,
		             "mute",        (gboolean) i->mute,
		             "description",            description,
		             "hostname",               (NULL != hostname) ? hostname : "",
		             "icon-name",   (gchar *)  pa_proplist_gets(i->proplist, "device.icon_name"),
		             NULL);
	}
	else
	{
		PamaPulseSource  *monitor = pama_pulse_context_get_source_by_index(self, i->monitor_source);

		sink = g_object_new(PAMA_TYPE_PULSE_SINK, 
	                        "context",                self,
	                        "index",       (guint)    i->index,
	                        "volume",      (guint)    pa_cvolume_avg(&i->volume),
		                    "base-volume", (guint)    i->base_volume,
	                        "channels",    (guchar)   i->channel_map.channels,
	                        "mute",        (gboolean) i->mute,
	                        "name",                   i->name,
	                        "description",            description,
	                        "hostname",               (NULL != hostname) ? hostname : "",
	                        "monitor",                monitor,
	                        "hardware",               i->flags & PA_SINK_HARDWARE       ? TRUE : FALSE,
	                        "network",                i->flags & PA_SINK_NETWORK        ? TRUE : FALSE,
	                        "decibel-volume",         i->flags & PA_SINK_DECIBEL_VOLUME ? TRUE : FALSE,
		                    "icon-name",   (gchar *)  pa_proplist_gets(i->proplist, "device.icon_name"),
	                        NULL);

		self->priv->sinks = g_slist_prepend(self->priv->sinks, sink);
		g_signal_emit(self, context_signals[SINK_ADDED_SIGNAL], 0, i->index);
	}
}
static void source_cb       (pa_context *c, const pa_source_info        *i, int eol, void *data)
{
	if (eol)
		return;

	PamaPulseContext *self = data;
	PamaPulseSource *source = pama_pulse_context_get_source_by_index(self, i->index);
	
	/* Networked devices have the remote username/hostname appended to them, which I don't want */
	gchar *description = NULL;
	gchar *hostname = NULL;
	if (i->flags & PA_SOURCE_NETWORK)
	{
		description = (gchar *) pa_proplist_gets(i->proplist, "tunnel.remote.description");
		hostname = (gchar *) pa_proplist_gets(i->proplist, "tunnel.remote.fqdn");
	}
	if (! description)
		description = (gchar *) i->description;

	if (source)
	{
		g_object_set(source,
		             "volume",      (guint)    pa_cvolume_avg(&i->volume),
		             "base-volume", (guint)    i->base_volume,
		             "channels",    (guchar)   i->channel_map.channels,
		             "mute",        (gboolean) i->mute,
		             "description",            description,
	                 "hostname",               (NULL != hostname) ? hostname : "",
		             "icon-name",   (gchar *)  pa_proplist_gets(i->proplist, "device.icon_name"),
		             NULL);
	}
	else
	{
		source = g_object_new(PAMA_TYPE_PULSE_SOURCE, 
	                          "context",     self,
	                          "index",       (guint)    i->index,
	                          "volume",      (guint)    pa_cvolume_avg(&i->volume),
		                      "base-volume", (guint)    i->base_volume,
	                          "channels",    (guchar)   i->channel_map.channels,
	                          "mute",        (gboolean) i->mute,
	                          "name",                   i->name,
	                          "description",            description,
	                          "hostname",               (NULL != hostname) ? hostname : "",
	                          "hardware",               i->flags & PA_SOURCE_HARDWARE       ? TRUE : FALSE,
	                          "network",                i->flags & PA_SOURCE_NETWORK        ? TRUE : FALSE,
	                          "decibel-volume",         i->flags & PA_SOURCE_DECIBEL_VOLUME ? TRUE : FALSE,
		                      "icon-name",   (gchar *)  pa_proplist_gets(i->proplist, "device.icon_name"),
		                      "monitored-sink-index",   i->monitor_of_sink,
	                          NULL);
		self->priv->sources = g_slist_insert_sorted(self->priv->sources, source, pama_pulse_source_compare_by_description);
		g_signal_emit(self, context_signals[SOURCE_ADDED_SIGNAL], 0, i->index);
	}
}
static void client_cb       (pa_context *c, const pa_client_info        *i, int eol, void *data)
{
	if (eol)
		return;

	PamaPulseContext *self = data;
	PamaPulseClient *client = pama_pulse_context_get_client_by_index(self, i->index);

	gboolean is_local;
	gchar *application_id = (gchar *)pa_proplist_gets(i->proplist, "application.id");
	gchar *hostname = (gchar *) pa_proplist_gets(i->proplist, "application.process.host");

	if (!application_id)
		application_id = "";
	if (!hostname)
		hostname = self->priv->hostname->str;
	
	is_local = !strcmp(hostname, self->priv->hostname->str);

	if (client)
	{
		g_object_set(client,
		             "name", i->name,
		             "hostname", hostname,
		             "is-local", is_local,
		             "application-id", application_id,
		             NULL);
	}
	else
	{
		client = g_object_new(PAMA_TYPE_PULSE_CLIENT,
		                      "index", (guint) i->index,
		                      "name",          i->name,
		                      "hostname",      hostname,
		                      "is-local",      is_local,
		                      "application-id", application_id,
		                      NULL);
		self->priv->clients = g_slist_prepend(self->priv->clients, client);
		g_signal_emit(self, context_signals[CLIENT_ADDED_SIGNAL], 0, i->index);
	}
}
static void sink_input_cb   (pa_context *c, const pa_sink_input_info    *i, int eol, void *data)
{
	if (eol || i->client == PA_INVALID_INDEX)
		return;

	PamaPulseContext   *self       = data;
	PamaPulseSinkInput *sink_input = pama_pulse_context_get_sink_input_by_index(self, i->index);
	PamaPulseSink      *sink       = pama_pulse_context_get_sink_by_index      (self, i->sink);

	gchar *icon_name  = (gchar *)  pa_proplist_gets(i->proplist, "application.icon_name");
	gchar *role       = (gchar *)  pa_proplist_gets(i->proplist, "media.role");
	gchar *restore_id = (gchar *)  pa_proplist_gets(i->proplist, "module-stream-restore.id");
	
	if (!icon_name)  icon_name  = "";
	if (!role)       role       = "";
	if (!restore_id) restore_id = "";

	if (sink_input)
	{
		g_object_set(sink_input,
		             "volume",    (guint)    pa_cvolume_avg(&i->volume),
		             "channels",  (guchar)   i->channel_map.channels,
		             "mute",      (gboolean) i->mute,
		             "name",                 i->name,
		             "sink",                 sink,
		             "icon-name",            icon_name,
		             "role",                 role,
		             "stream-restore-id",    restore_id,
		             NULL);
	}
	else
	{
		PamaPulseClient *client = pama_pulse_context_get_client_by_index(self, i->client);
		sink_input = g_object_new(PAMA_TYPE_PULSE_SINK_INPUT, 
		                          "context",              self,
		                          "index",     (guint)    i->index,
		                          "volume",    (guint)    pa_cvolume_avg(&i->volume),
		                          "channels",  (guchar)   i->channel_map.channels,
		                          "mute",      (gboolean) i->mute,
		                          "name",                 i->name,
		                          "client",               client,
		                          "sink",                 sink,
		                          "icon-name",            icon_name,
		                          "role",                 role,
		                          "stream-restore-id",    restore_id,
		                          NULL);
		self->priv->sink_inputs = g_slist_prepend(self->priv->sink_inputs, sink_input);
		g_signal_emit(self, context_signals[SINK_INPUT_ADDED_SIGNAL], 0, i->index);
	}
}
static void source_output_cb(pa_context *c, const pa_source_output_info *i, int eol, void *data)
{
	if (eol || i->client == PA_INVALID_INDEX)
		return;

	// Discard peak-detect streams
	if (i->resample_method && 0 == strcmp(i->resample_method, "peaks"))
		return;

	PamaPulseContext      *self          = data;
	PamaPulseSourceOutput *source_output = pama_pulse_context_get_source_output_by_index(self, i->index);
	PamaPulseSource       *source        = pama_pulse_context_get_source_by_index       (self, i->source);
	
	gchar *icon_name  = (gchar *)  pa_proplist_gets(i->proplist, "application.icon_name");
	gchar *role       = (gchar *)  pa_proplist_gets(i->proplist, "media.role");
	gchar *restore_id = (gchar *)  pa_proplist_gets(i->proplist, "module-stream-restore.id");
	
	if (!icon_name)  icon_name  = "";
	if (!role)       role       = "";
	if (!restore_id) restore_id = "";

	if (source_output)
		g_object_set(source_output,
		             "name",   i->name,
		             "source", source,
		             "icon-name",            icon_name,
		             "role",                 role,
		             "stream-restore-id",    restore_id,
		             NULL);
	else
	{
		PamaPulseClient *client = pama_pulse_context_get_client_by_index(self, i->client);
		source_output = g_object_new(PAMA_TYPE_PULSE_SOURCE_OUTPUT, 
		                             "context",  self,
		                             "index",    (guint)    i->index,
		                             "name",                i->name,
		                             "client",              client,
		                             "source",              source,
		                             "icon-name",           icon_name,
		                             "role",                role,
		                             "stream-restore-id",   restore_id,
		                             NULL);
		self->priv->source_outputs = g_slist_insert_sorted(self->priv->source_outputs, source_output, pama_pulse_source_output_compare_by_name);
		g_signal_emit(self, context_signals[SOURCE_OUTPUT_ADDED_SIGNAL], 0, i->index);
	}
}
static void server_cb       (pa_context *c, const pa_server_info        *i,          void *data)
{
	PamaPulseContext *self = data;
	
	g_object_set(self, 
				 "default-sink-name", i->default_sink_name,
				 "default-source-name", i->default_source_name,
				 "hostname",            i->host_name,
				 NULL);
}


