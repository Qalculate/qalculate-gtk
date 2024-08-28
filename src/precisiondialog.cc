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
#include "mainwindow.h"
#include "precisiondialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *precision_builder = NULL;

void on_precision_dialog_spinbutton_precision_value_changed(GtkSpinButton *w, gpointer) {
	set_precision(gtk_spin_button_get_value_as_int(w), 0);
}
void on_precision_dialog_button_recalculate_clicked(GtkButton*, gpointer) {
	set_precision(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision"))), 1);
}

GtkWidget* get_precision_dialog(void) {
	if(!precision_builder) {

		precision_builder = getBuilder("precision.ui");
		g_assert(precision_builder != NULL);

		g_assert(gtk_builder_get_object(precision_builder, "precision_dialog") != NULL);

		gtk_builder_add_callback_symbols(precision_builder, "on_precision_dialog_button_recalculate_clicked", G_CALLBACK(on_precision_dialog_button_recalculate_clicked), "on_precision_dialog_spinbutton_precision_value_changed", G_CALLBACK(on_precision_dialog_spinbutton_precision_value_changed), NULL);
		gtk_builder_connect_signals(precision_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(precision_builder, "precision_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(precision_builder, "precision_dialog"));
}

void update_precision() {
	if(precision_builder) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_precision_dialog_spinbutton_precision_value_changed, NULL);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision")), PRECISION);
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_precision_dialog_spinbutton_precision_value_changed, NULL);
	}
}

void open_precision(GtkWindow *parent) {
	GtkWidget *dialog = get_precision_dialog();
	update_precision();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(precision_builder, "precision_dialog_spinbutton_precision")));
	gtk_widget_show(dialog);
}
