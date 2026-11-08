/*
    Qalculate (GTK UI)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include <gtk/gtk.h>

#include "colorscheme.h"

#if defined(_WIN32) || defined(__APPLE__)

// No XDG desktop portal: behaves like the default theme
void color_scheme_follow_system(bool) {}

#else

#include "mainwindow.h"
#include "historyview.h"

extern bool b_busy_result;

#define PORTAL_BUS_NAME "org.freedesktop.portal.Desktop"
#define PORTAL_OBJECT_PATH "/org/freedesktop/portal/desktop"
#define PORTAL_SETTINGS_INTERFACE "org.freedesktop.portal.Settings"
#define APPEARANCE_NAMESPACE "org.freedesktop.appearance"
#define COLOR_SCHEME_KEY "color-scheme"
// Values of color-scheme: 0 = no preference, 1 = prefer dark, 2 = prefer light
#define COLOR_SCHEME_PREFER_DARK 1

static GDBusConnection *session_bus = NULL;
static guint setting_changed_id = 0;
static gboolean original_prefer_dark = FALSE;

// No preference is treated as light, like libadwaita does
static void apply_color_scheme(GVariant *value) {
	// Read wraps the value in additional variants
	GVariant *v = g_variant_ref(value);
	while(g_variant_is_of_type(v, G_VARIANT_TYPE_VARIANT)) {
		GVariant *inner = g_variant_get_variant(v);
		g_variant_unref(v);
		v = inner;
	}
	if(g_variant_is_of_type(v, G_VARIANT_TYPE_UINT32)) {
		gboolean dark = g_variant_get_uint32(v) == COLOR_SCHEME_PREFER_DARK;
		g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", dark, NULL);
	}
	g_variant_unref(v);
}

static gboolean refresh_colors(gpointer) {
	// reload_history() would invalidate the history entry of a result that is being added
	if(b_busy_result) return TRUE;
	update_colors(false);
	reload_history();
	return FALSE;
}

static void on_setting_changed(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant *parameters, gpointer) {
	if(!g_variant_is_of_type(parameters, G_VARIANT_TYPE("(ssv)"))) return;
	const gchar *name_space = NULL, *key = NULL;
	GVariant *value = NULL;
	g_variant_get(parameters, "(&s&sv)", &name_space, &key, &value);
	if(g_strcmp0(name_space, APPEARANCE_NAMESPACE) == 0 && g_strcmp0(key, COLOR_SCHEME_KEY) == 0) {
		apply_color_scheme(value);
		// refresh now, or retry until the result has been added to the history
		if(refresh_colors(NULL)) g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 100, refresh_colors, NULL, NULL);
	}
	g_variant_unref(value);
}

void color_scheme_follow_system(bool follow) {
	GtkSettings *settings = gtk_settings_get_default();
	if(!settings || follow == (setting_changed_id != 0)) return;
	if(!follow) {
		g_dbus_connection_signal_unsubscribe(session_bus, setting_changed_id);
		setting_changed_id = 0;
		g_object_set(settings, "gtk-application-prefer-dark-theme", original_prefer_dark, NULL);
		return;
	}
	if(!session_bus) session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	if(!session_bus) return;
	// restored when following stops
	g_object_get(settings, "gtk-application-prefer-dark-theme", &original_prefer_dark, NULL);
	setting_changed_id = g_dbus_connection_signal_subscribe(session_bus, PORTAL_BUS_NAME, PORTAL_SETTINGS_INTERFACE, "SettingChanged", PORTAL_OBJECT_PATH, APPEARANCE_NAMESPACE, G_DBUS_SIGNAL_FLAGS_NONE, on_setting_changed, NULL, NULL);
	// Read is deprecated in favour of ReadOne, but supported by all versions of the portal
	GVariant *reply = g_dbus_connection_call_sync(session_bus, PORTAL_BUS_NAME, PORTAL_OBJECT_PATH, PORTAL_SETTINGS_INTERFACE, "Read", g_variant_new("(ss)", APPEARANCE_NAMESPACE, COLOR_SCHEME_KEY), G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 1000, NULL, NULL);
	// without reply (no portal or no color-scheme setting) the theme is unchanged until the setting changes
	if(!reply) return;
	GVariant *value = NULL;
	g_variant_get(reply, "(v)", &value);
	apply_color_scheme(value);
	g_variant_unref(value);
	g_variant_unref(reply);
}

#endif
