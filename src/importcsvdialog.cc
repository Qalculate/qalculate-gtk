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
#include "importcsvdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *csvimport_builder = NULL;

void on_csv_import_radiobutton_matrix_toggled(GtkToggleButton*, gpointer) {
}
void on_csv_import_radiobutton_vectors_toggled(GtkToggleButton*, gpointer) {
}
void on_csv_import_combobox_delimiter_changed(GtkComboBox *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_delimiter_other")), gtk_combo_box_get_active(w) == DELIMITER_OTHER);
}
void on_csv_import_button_file_clicked(GtkEntry*, gpointer) {
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	GtkFileChooserNative *d = gtk_file_chooser_native_new(_("Select file to import"), GTK_WINDOW(gtk_builder_get_object(csvimport_builder, "csv_import_dialog")), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Open"), _("_Cancel"));
#else
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to import"), GTK_WINDOW(gtk_builder_get_object(csvimport_builder, "csv_import_dialog")), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
#endif
	string filestr = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")));
	remove_blank_ends(filestr);
	if(!filestr.empty()) gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d), filestr.c_str());
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#else
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#endif
		const gchar *file_str = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")), file_str);
		string name_str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")));
		remove_blank_ends(name_str);
		if(name_str.empty()) {
			name_str = file_str;
			size_t i = name_str.find_last_of("/");
			if(i != string::npos) name_str = name_str.substr(i + 1, name_str.length() - i);
			i = name_str.find_last_of(".");
			if(i != string::npos) name_str = name_str.substr(0, i);
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")), name_str.c_str());
		}
	}
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	g_object_unref(d);
#else
	gtk_widget_destroy(d);
#endif
}

GtkWidget* get_csv_import_dialog(void) {

	if(!csvimport_builder) {

		csvimport_builder = getBuilder("csvimport.ui");
		g_assert(csvimport_builder != NULL);

		g_assert(gtk_builder_get_object(csvimport_builder, "csv_import_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(csvimport_builder, "csv_import_combobox_delimiter")), 0);

		gtk_builder_add_callback_symbols(csvimport_builder, "on_csv_import_button_file_clicked", G_CALLBACK(on_csv_import_button_file_clicked), "on_csv_import_radiobutton_matrix_toggled", G_CALLBACK(on_csv_import_radiobutton_matrix_toggled), "on_csv_import_radiobutton_vectors_toggled", G_CALLBACK(on_csv_import_radiobutton_vectors_toggled), "on_variable_edit_entry_name_changed", G_CALLBACK(on_variable_edit_entry_name_changed), "on_csv_import_combobox_delimiter_changed", G_CALLBACK(on_csv_import_combobox_delimiter_changed), NULL);
		gtk_builder_connect_signals(csvimport_builder, NULL);

	}

	/* populate combo menu */
	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->variables[i]->category().c_str()) == NULL) {
				items = g_list_append(items, (gpointer) CALCULATOR->variables[i]->category().c_str());
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->variables[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(csvimport_builder, "csv_import_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_dialog"));
}

void import_csv_file(GtkWindow *parent) {

	GtkWidget *dialog = get_csv_import_dialog();
	if(parent) gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);

	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_desc")), "");

	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")));

run_csv_import_dialog:
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")));
		remove_blank_ends(str);
		if(str.empty()) {
			//no filename -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_file")));
			show_message(_("No file name entered."), dialog);
			goto run_csv_import_dialog;
		}
		string name_str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")));
		remove_blank_ends(name_str);
		if(name_str.empty()) {
			//no name -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_csv_import_dialog;
		}
		//variable with the same name exists -- overwrite or open dialog again
		if(CALCULATOR->variableNameTaken(name_str)) {
			Variable *var = CALCULATOR->getActiveVariable(str, true);
			if((!var || var->category() != CALCULATOR->temporaryCategory()) && !ask_question(_("A unit or variable with the same name already exists.\nDo you want to overwrite it?"), dialog)) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_name")));
				goto run_csv_import_dialog;
			}
		}
		string delimiter = "";
		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(csvimport_builder, "csv_import_combobox_delimiter")))) {
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
				delimiter = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_delimiter_other")));
				break;
			}
		}
		if(delimiter.empty()) {
			//no filename -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(csvimport_builder, "csv_import_entry_delimiter_other")));
			show_message(_("No delimiter selected."), dialog);
			goto run_csv_import_dialog;
		}
		block_error();
		if(!CALCULATOR->importCSV(str.c_str(), gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(csvimport_builder, "csv_import_spinbutton_first_row"))), gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvimport_builder, "csv_import_checkbutton_headers"))), delimiter, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(csvimport_builder, "csv_import_radiobutton_matrix"))), name_str, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(csvimport_builder, "csv_import_entry_desc"))), gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(csvimport_builder, "csv_import_combo_category"))))) {
			GtkWidget *edialog = gtk_message_dialog_new(GTK_WINDOW(mainwindow), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Could not import from file \n%s"), str.c_str());
			if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
			gtk_dialog_run(GTK_DIALOG(edialog));
			gtk_widget_destroy(edialog);
		}
		display_errors(NULL, dialog);
		unblock_error();
		update_vmenu();
	}
	gtk_widget_hide(dialog);
}
