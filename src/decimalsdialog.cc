/*
    Qalculate (GTK UI)

    Copyright (C) 2003-2007, 2008, 2016-2024  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef _MSC_VER
#	include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "support.h"
#include "settings.h"
#include "util.h"
#include "decimalsdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *decimals_builder = NULL;

void on_decimals_dialog_spinbutton_max_value_changed(GtkSpinButton *w, gpointer) {
	printops.max_decimals = gtk_spin_button_get_value_as_int(w);
	result_format_updated();
}
void on_decimals_dialog_spinbutton_min_value_changed(GtkSpinButton *w, gpointer) {
	printops.min_decimals = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));
	result_format_updated();
}
void on_decimals_dialog_checkbutton_max_toggled(GtkToggleButton *w, gpointer) {
	printops.use_max_decimals = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max")), printops.use_max_decimals);
	result_format_updated();
}
void on_decimals_dialog_checkbutton_min_toggled(GtkToggleButton *w, gpointer) {
	printops.use_min_decimals = gtk_toggle_button_get_active(w);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min")), printops.use_min_decimals);
	result_format_updated();
}

GtkWidget* get_decimals_dialog(void) {
	if(!decimals_builder) {

		decimals_builder = getBuilder("decimals.ui");
		g_assert(decimals_builder != NULL);

		g_assert(gtk_builder_get_object(decimals_builder, "decimals_dialog") != NULL);

		gtk_builder_add_callback_symbols(decimals_builder, "on_decimals_dialog_checkbutton_min_toggled", G_CALLBACK(on_decimals_dialog_checkbutton_min_toggled), "on_decimals_dialog_checkbutton_max_toggled", G_CALLBACK(on_decimals_dialog_checkbutton_max_toggled), "on_decimals_dialog_spinbutton_min_value_changed", G_CALLBACK(on_decimals_dialog_spinbutton_min_value_changed), "on_decimals_dialog_spinbutton_max_value_changed", G_CALLBACK(on_decimals_dialog_spinbutton_max_value_changed), NULL);
		gtk_builder_connect_signals(decimals_builder, NULL);

	}

	if(!enable_tooltips || toe_changed) set_tooltips_enabled(GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog")), enable_tooltips);
	if(always_on_top || aot_changed) gtk_window_set_keep_above(GTK_WINDOW(gtk_builder_get_object(decimals_builder, "decimals_dialog")), always_on_top);
	return GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog"));
}

void decimals_updated() {
	if(decimals_builder) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_max"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_checkbutton_max_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_checkbutton_min_toggled, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_spinbutton_max_value_changed, NULL);
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_spinbutton_min_value_changed, NULL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min")), printops.use_min_decimals);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_max")), printops.use_max_decimals);
		gtk_widget_set_sensitive (GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min")), printops.use_min_decimals);
		gtk_widget_set_sensitive (GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max")), printops.use_max_decimals);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min")), printops.min_decimals);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max")), printops.max_decimals);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_max"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_checkbutton_max_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_checkbutton_min_toggled, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_max"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_spinbutton_max_value_changed, NULL);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(decimals_builder, "decimals_dialog_spinbutton_min"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_decimals_dialog_spinbutton_min_value_changed, NULL);
	}
}

void open_decimals(GtkWindow *parent) {
	GtkWidget *dialog = get_decimals_dialog();
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(decimals_builder, "decimals_dialog_checkbutton_min")));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	decimals_updated();
	gtk_widget_show(dialog);
}
