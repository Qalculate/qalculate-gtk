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
#include "exportcsvdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *csvexport_builder = NULL;

void on_csv_export_combobox_delimiter_changed(GtkComboBox *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_delimiter_other")), gtk_combo_box_get_active(w) == DELIMITER_OTHER);
}
void on_csv_export_button_file_clicked(GtkEntry*, gpointer) {
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	GtkFileChooserNative *d = gtk_file_chooser_native_new(_("Select file to export to"), GTK_WINDOW(gtk_builder_get_object(csvexport_builder, "csv_export_dialog")), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Open"), _("_Cancel"));
#else
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to export to"), GTK_WINDOW(gtk_builder_get_object(csvexport_builder, "csv_export_dialog")), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
#endif
	string filestr = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));
	remove_blank_ends(filestr);
	if(!filestr.empty()) gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d), filestr.c_str());
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#else
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#endif
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")), gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d)));
	}
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	g_object_unref(d);
#else
	gtk_widget_destroy(d);
#endif
}
void on_csv_export_radiobutton_current_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), !gtk_toggle_button_get_active(w));
}
void on_csv_export_radiobutton_matrix_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), gtk_toggle_button_get_active(w));
}

GtkWidget* get_csv_export_dialog(void) {

	if(!csvexport_builder) {

		csvexport_builder = getBuilder("csvexport.ui");
		g_assert(csvexport_builder != NULL);

		g_assert(gtk_builder_get_object(csvexport_builder, "csv_export_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(csvexport_builder, "csv_export_combobox_delimiter")), 0);

		gtk_builder_add_callback_symbols(csvexport_builder, "on_csv_export_radiobutton_current_toggled", G_CALLBACK(on_csv_export_radiobutton_current_toggled), "on_csv_export_radiobutton_matrix_toggled", G_CALLBACK(on_csv_export_radiobutton_matrix_toggled), "on_csv_export_combobox_delimiter_changed", G_CALLBACK(on_csv_export_combobox_delimiter_changed), "on_csv_export_button_file_clicked", G_CALLBACK(on_csv_export_button_file_clicked), NULL);
		gtk_builder_connect_signals(csvexport_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_dialog"));

}

void export_csv_file(GtkWindow *win, KnownVariable *v) {

	GtkWidget *dialog = get_csv_export_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), win);

	if(v) {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")), v->preferredDisplayName(false, false, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")).name.c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), v->preferredDisplayName(false, false, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")).name.c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_matrix")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_current")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_matrix")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), FALSE);
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));
	} else {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")), "");
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), "");
		if(current_result() && current_result()->isVector()) {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_current")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_current")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), FALSE);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_current")), FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_matrix")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")), TRUE);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")));
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_matrix")), TRUE);
	}

run_csv_export_dialog:
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));
		remove_blank_ends(str);
		if(str.empty()) {
			//no filename -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_file")));
			show_message(_("No file name entered."), GTK_WINDOW(dialog));
			goto run_csv_export_dialog;
		}
		string delimiter = "";
		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(csvexport_builder, "csv_export_combobox_delimiter")))) {
			case DELIMITER_COMMA: {
				delimiter = ",";
				break;
			}
			case DELIMITER_TABULATOR: {
				delimiter = "\t";
				break;
			}
			case DELIMITER_SEMICOLON: {
				delimiter = ";";
				break;
			}
			case DELIMITER_SPACE: {
				delimiter = " ";
				break;
			}
			case DELIMITER_OTHER: {
				delimiter = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_delimiter_other")));
				break;
			}
		}
		if(delimiter.empty()) {
			//no delimiter -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_delimiter_other")));
			show_message(_("No delimiter selected."), GTK_WINDOW(dialog));
			goto run_csv_export_dialog;
		}
		MathStructure *matrix_struct;
		if(v) {
			matrix_struct = (MathStructure*) &v->get();
		} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvexport_builder, "csv_export_radiobutton_current")))) {
			matrix_struct = current_result();
		} else {
			string str2 = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")));
			remove_blank_ends(str2);
			if(str2.empty()) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")));
				show_message(_("No variable name entered."), GTK_WINDOW(dialog));
				goto run_csv_export_dialog;
			}
			Variable *var = CALCULATOR->getActiveVariable(str2, true);
			if(!var || !var->isKnown()) {
				var = CALCULATOR->getVariable(str2);
				while(var && !var->isKnown()) {
					var = CALCULATOR->getActiveVariable(str2, true);
				}
			}
			if(!var || !var->isKnown()) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvexport_builder, "csv_export_entry_matrix")));
				show_message(_("No known variable with entered name found."), GTK_WINDOW(dialog));
				goto run_csv_export_dialog;
			}
			matrix_struct = (MathStructure*) &((KnownVariable*) var)->get();
		}
		CALCULATOR->startControl(600000);
		if(!CALCULATOR->exportCSV(*matrix_struct, str.c_str(), delimiter) && CALCULATOR->aborted()) {
			GtkWidget *edialog = gtk_message_dialog_new(main_window(), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not export to file \n%s"), str.c_str());
			if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
			gtk_dialog_run(GTK_DIALOG(edialog));
			gtk_widget_destroy(edialog);
		}
		CALCULATOR->stopControl();
	}
	gtk_widget_hide(dialog);

}
