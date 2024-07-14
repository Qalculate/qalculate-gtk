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
#include "nameseditdialog.h"
#include "dataseteditdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *datasetedit_builder = NULL;

GtkWidget *tDataProperties;
GtkListStore *tDataProperties_store;

DataProperty *selected_dataproperty = NULL;
DataSet *edited_dataset = NULL;
DataProperty *edited_dataproperty = NULL;
bool auto_dataset_name = false, auto_dataset_file = false;
vector<DataProperty*> tmp_props;
vector<DataProperty*> tmp_props_orig;

bool edit_dataproperty(DataProperty *dp, bool new_property = false);

void on_dataset_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_ok")), strlen(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")))) > 0);
}
void on_dataproperty_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_button_ok")), strlen(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")))) > 0);
}
void on_tDataProperties_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	selected_dataproperty = NULL;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 3, &selected_dataproperty, -1);
	}
	if(selected_dataproperty) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_edit_property")), selected_dataproperty->isUserModified());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_del_property")), selected_dataproperty->isUserModified());
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_edit_property")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_del_property")), FALSE);
	}
}

void update_dataset_property_list(DataSet*) {
	if(!datasetedit_builder) return;
	selected_dataproperty = NULL;
	gtk_list_store_clear(tDataProperties_store);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_edit_property")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_del_property")), FALSE);
	GtkTreeIter iter;
	string str;
	for(size_t i = 0; i < tmp_props.size(); i++) {
		if(tmp_props[i]) {
			gtk_list_store_append(tDataProperties_store, &iter);
			str = "";
			switch(tmp_props[i]->propertyType()) {
				case PROPERTY_STRING: {
					str += _("text");
					break;
				}
				case PROPERTY_NUMBER: {
					if(tmp_props[i]->isApproximate()) {
						str += _("approximate");
						str += " ";
					}
					str += _("number");
					break;
				}
				case PROPERTY_EXPRESSION: {
					if(tmp_props[i]->isApproximate()) {
						str += _("approximate");
						str += " ";
					}
					str += _("expression");
					break;
				}
			}
			if(tmp_props[i]->isKey()) {
				str += " (";
				str += _("key");
				str += ")";
			}
			gtk_list_store_set(tDataProperties_store, &iter, 0, tmp_props[i]->title(false).c_str(), 1, tmp_props[i]->getName().c_str(), 2, str.c_str(), 3, (gpointer) tmp_props[i], -1);
		}
	}
}

void on_dataset_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	correct_name_entry(editable, TYPE_FUNCTION,  (gpointer) on_dataset_edit_entry_name_changed);
	auto_dataset_name = false;
	name_entry_changed();
}
void on_dataset_edit_entry_file_changed(GtkEditable*, gpointer) {
	auto_dataset_file = false;
}
void on_dataset_edit_entry_desc_changed(GtkEditable *w, gpointer) {
	if(auto_dataset_file) {
		string str = gtk_entry_get_text(GTK_ENTRY(w));
		remove_blank_ends(str);
		gsub(" ", "_", str);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file")), str.c_str());
		auto_dataset_file = true;
	}
	if(auto_dataset_name) {
		string str = gtk_entry_get_text(GTK_ENTRY(w));
		remove_blank_ends(str);
		gsub(" ", "_", str);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")), str.c_str());
		auto_dataset_name = true;
	}
}
void on_dataset_edit_button_new_property_clicked(GtkButton*, gpointer) {
	DataProperty *dp = new DataProperty(edited_dataset);
	dp->setUserModified(true);
	if(edit_dataproperty(dp, true)) {
		tmp_props.push_back(dp);
		tmp_props_orig.push_back(NULL);
		update_dataset_property_list(edited_dataset);
		on_dataset_changed();
	} else {
		delete dp;
	}
}
void on_dataset_edit_button_edit_property_clicked(GtkButton*, gpointer) {
	if(selected_dataproperty) {
		if(edit_dataproperty(selected_dataproperty, false)) {
			update_dataset_property_list(edited_dataset);
			on_dataset_changed();
		}
	}
}
void on_dataset_edit_button_del_property_clicked(GtkButton*, gpointer) {
	if(edited_dataset && selected_dataproperty && selected_dataproperty->isUserModified()) {
		for(size_t i = 0; i < tmp_props.size(); i++) {
			if(tmp_props[i] == selected_dataproperty) {
				delete tmp_props[i];
				if(tmp_props_orig[i]) {
					tmp_props[i] = NULL;
				} else {
					tmp_props.erase(tmp_props.begin() + i);
					tmp_props_orig.erase(tmp_props_orig.begin() + i);
				}
				break;
			}
		}
		update_dataset_property_list(edited_dataset);
		on_dataset_changed();
	}
}
void on_dataset_edit_button_names_clicked(GtkWidget*, gpointer) {
	if(!edit_names(edited_dataset, TYPE_FUNCTION, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name"))), GTK_WINDOW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog")))) return;
	string str = first_name();
	if(!str.empty()) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_dataset_edit_entry_name_changed, NULL);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_dataset_edit_entry_name_changed, NULL);
	}
	on_dataset_changed();
}

void on_dataproperty_edit_button_names_clicked(GtkWidget*, gpointer) {
	if(!edit_names(NULL, -1, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name"))), GTK_WINDOW(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_dialog")), edited_dataproperty)) return;
	string str = first_name();
	if(!str.empty()) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_dataproperty_changed, NULL);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_dataproperty_changed, NULL);
	}
	on_dataproperty_changed();
}
void on_dataproperty_edit_combobox_type_changed(GtkComboBox *om, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_key")), gtk_combo_box_get_active(om) != 2);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_case")), gtk_combo_box_get_active(om) == 0);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_approximate")), gtk_combo_box_get_active(om) != 0);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_brackets")), gtk_combo_box_get_active(om) != 0);
}

GtkWidget* get_dataset_edit_dialog(void) {

	if(!datasetedit_builder) {

		datasetedit_builder = getBuilder("datasetedit.ui");
		g_assert(datasetedit_builder != NULL);

		g_assert(gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog") != NULL);

		tDataProperties = GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_treeview_properties"));
		tDataProperties_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tDataProperties), GTK_TREE_MODEL(tDataProperties_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataProperties));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataProperties), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataProperties), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, "text", 2, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataProperties), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tDataProperties_selection_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_description"))), "changed", G_CALLBACK(on_dataset_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_copyright"))), "changed", G_CALLBACK(on_dataset_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_textview_description"))), "changed", G_CALLBACK(on_dataproperty_changed), NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), 0);

		gtk_builder_add_callback_symbols(datasetedit_builder, "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_dataproperty_edit_combobox_type_changed", G_CALLBACK(on_dataproperty_edit_combobox_type_changed), "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_unit_entry_key_press_event", G_CALLBACK(on_unit_entry_key_press_event), "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_dataproperty_edit_button_names_clicked", G_CALLBACK(on_dataproperty_edit_button_names_clicked), "on_dataproperty_changed", G_CALLBACK(on_dataproperty_changed), "on_dataset_changed", G_CALLBACK(on_dataset_changed), "on_dataset_edit_entry_desc_changed", G_CALLBACK(on_dataset_edit_entry_desc_changed), "on_dataset_changed", G_CALLBACK(on_dataset_changed), "on_dataset_edit_entry_file_changed", G_CALLBACK(on_dataset_edit_entry_file_changed), "on_dataset_edit_button_new_property_clicked", G_CALLBACK(on_dataset_edit_button_new_property_clicked), "on_dataset_edit_button_edit_property_clicked", G_CALLBACK(on_dataset_edit_button_edit_property_clicked), "on_dataset_edit_button_del_property_clicked", G_CALLBACK(on_dataset_edit_button_del_property_clicked), "on_dataset_changed", G_CALLBACK(on_dataset_changed), "on_dataset_edit_entry_name_changed", G_CALLBACK(on_dataset_edit_entry_name_changed), "on_dataset_edit_button_names_clicked", G_CALLBACK(on_dataset_edit_button_names_clicked), "on_dataset_changed", G_CALLBACK(on_dataset_changed), "on_dataset_changed", G_CALLBACK(on_dataset_changed), "on_dataset_changed", G_CALLBACK(on_dataset_changed), NULL);
		gtk_builder_connect_signals(datasetedit_builder, NULL);

	}
	update_window_properties(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog"));
}

GtkWidget* get_dataproperty_edit_dialog(void) {
	update_window_properties(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_dialog")));
	return GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_dialog"));
}

bool edit_dataproperty(DataProperty *dp, bool new_property) {

	GtkWidget *dialog = get_dataproperty_edit_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_dialog")));

	edited_dataproperty = dp;
	int names_status_bak = names_status();
	reset_names_status();

	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")), dp->getName().c_str());

	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_title")), dp->title(false).c_str());
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_unit")), localize_expression(dp->getUnitString(), true).c_str());

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_hide")), dp->isHidden());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_key")), dp->isKey());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_approximate")), dp->isApproximate());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_case")), dp->isCaseSensitive());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_brackets")), dp->usesBrackets());

	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_textview_description")));
	gtk_text_buffer_set_text(description_buffer, dp->description().c_str(), -1);

	switch(dp->propertyType()) {
		case PROPERTY_STRING: {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), 0);
			break;
		}
		case PROPERTY_NUMBER: {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), 1);
			break;
		}
		case PROPERTY_EXPRESSION: {
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), 2);
			break;
		}
	}

	on_dataproperty_edit_combobox_type_changed(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")), NULL);

	bool return_val = false;

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_button_ok")), new_property);

	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")));

	run_dataproperty_edit_dialog:
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {

		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")));
		remove_blank_ends(str);
		if(str.empty() && (!names_status() || !has_name())) {
			//no name -- open dialog again
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_dataproperty_edit_dialog;
		}

		dp->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_title"))));
		dp->setUnit(unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_entry_unit")))));

		dp->setHidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_hide"))));
		dp->setKey(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_key"))));
		dp->setApproximate(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_approximate"))));
		dp->setCaseSensitive(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_case"))));
		dp->setUsesBrackets(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_checkbutton_brackets"))));

		GtkTextIter e_iter_s, e_iter_e;
		gtk_text_buffer_get_start_iter(description_buffer, &e_iter_s);
		gtk_text_buffer_get_end_iter(description_buffer, &e_iter_e);
		gchar *gstr = gtk_text_buffer_get_text(description_buffer, &e_iter_s, &e_iter_e, FALSE);
		dp->setDescription(gstr);
		g_free(gstr);

		switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(datasetedit_builder, "dataproperty_edit_combobox_type")))) {
			case 0: {
				dp->setPropertyType(PROPERTY_STRING);
				break;
			}
			case 1: {
				dp->setPropertyType(PROPERTY_NUMBER);
				break;
			}
			case 2: {
				dp->setPropertyType(PROPERTY_EXPRESSION);
				break;
			}
		}
		set_edited_names(dp, str);

		return_val = true;

	}

	set_names_status(names_status_bak);
	edited_dataproperty = NULL;

	gtk_widget_hide(dialog);

	return return_val;

}

void edit_dataset(DataSet *ds, GtkWindow *win) {
	GtkWidget *dialog = get_dataset_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), win);

	edited_dataset = ds;
	reset_names_status();

	if(ds) {
		if(ds->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Data Set"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Data Set (global)"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Data Set"));
	}

	auto_dataset_name = false;
	auto_dataset_file = false;

	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_description")));
	GtkTextBuffer *copyright_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_copyright")));

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_textview_copyright")), !ds || ds->isLocal());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file")), !ds || ds->isLocal());

	//clear entries
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")), "");
	//gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(datasetedit_builder, "dataset_edit_label_names")), "");
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")), TRUE);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_object_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_property_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_default_property")), "info");
	gtk_text_buffer_set_text(description_buffer, "", -1);
	gtk_text_buffer_set_text(copyright_buffer, "", -1);

	gtk_list_store_clear(tDataProperties_store);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_edit_property")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_del_property")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_new_property")), TRUE);

	if(ds) {
		//fill in original paramaters
		set_name_label_and_entry(ds, GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")));
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc")), ds->title(false).c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file")), ds->defaultDataFile().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_default_property")), ds->defaultProperty().c_str());
		Argument *arg = ds->getArgumentDefinition(1);
		if(arg) {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_object_name")), arg->name().c_str());
		}
		arg = ds->getArgumentDefinition(2);
		if(arg) {
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_property_name")), arg->name().c_str());
		}
		gtk_text_buffer_set_text(description_buffer, ds->description().c_str(), -1);
		gtk_text_buffer_set_text(copyright_buffer, ds->copyright().c_str(), -1);
		DataPropertyIter it;
		DataProperty *dp = ds->getFirstProperty(&it);
		while(dp) {
			tmp_props.push_back(new DataProperty(*dp));
			tmp_props_orig.push_back(dp);
			dp = ds->getNextProperty(&it);
		}
	} else {
		auto_dataset_name = true;
		auto_dataset_file = true;
	}

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_button_ok")), FALSE);

	update_dataset_property_list(ds);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(datasetedit_builder, "dataset_edit_tabs")), 0);

	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc")));

	run_dataset_edit_dialog:
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")));
		remove_blank_ends(str);
		if(str.empty() && (!names_status() || !has_name())) {
			//no name -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(datasetedit_builder, "dataset_edit_tabs")), 2);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")));
			show_message(_("Empty name field."), dialog);
			goto run_dataset_edit_dialog;
		}
		GtkTextIter d_iter_s, d_iter_e;
		gtk_text_buffer_get_start_iter(description_buffer, &d_iter_s);
		gtk_text_buffer_get_end_iter(description_buffer, &d_iter_e);
		GtkTextIter c_iter_s, c_iter_e;
		gtk_text_buffer_get_start_iter(copyright_buffer, &c_iter_s);
		gtk_text_buffer_get_end_iter(copyright_buffer, &c_iter_e);
		//dataset with the same name exists -- overwrite or open the dialog again
		if((!ds || !ds->hasName(str)) && ((names_status() != 1 && !str.empty()) || !has_name()) && CALCULATOR->functionNameTaken(str, ds)) {
			MathFunction *func = CALCULATOR->getActiveFunction(str, true);
			if((!ds || ds != func) && (!func || func->category() != CALCULATOR->temporaryCategory()) && !ask_question(_("A function with the same name already exists.\nDo you want to overwrite the function?"), dialog)) {
				gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(datasetedit_builder, "dataset_edit_tabs")), 2);
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_name")));
				goto run_dataset_edit_dialog;
			}
		}
		bool add_func = false;
		gchar *gstr_descr = gtk_text_buffer_get_text(description_buffer, &d_iter_s, &d_iter_e, FALSE);
		if(ds) {
			//edited an existing dataset
			ds->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc"))));
			if(ds->isLocal()) ds->setDefaultDataFile(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file"))));
			ds->setDescription(gstr_descr);
		} else {
			//new dataset
			DataSet *ds_atom = CALCULATOR->getDataSet("atom");
			ds = new DataSet(ds_atom ? ds_atom->category() : _("Data Sets"), "", gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_file"))), gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_desc"))), gstr_descr, true);
			add_func = true;
		}
		g_free(gstr_descr);
		string str2;
		if(ds) {
			str2 = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_object_name")));
			remove_blank_ends(str2);
			if(str2.empty()) str2 = _("Object");
			Argument *arg = ds->getArgumentDefinition(1);
			if(arg) {
				arg->setName(str2);
			}
			str2 = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_property_name")));
			remove_blank_ends(str2);
			if(str2.empty()) str2 = _("Property");
			arg = ds->getArgumentDefinition(2);
			if(arg) {
				arg->setName(str2);
			}
			ds->setDefaultProperty(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(datasetedit_builder, "dataset_edit_entry_default_property"))));
			gchar *gstr = gtk_text_buffer_get_text(copyright_buffer, &c_iter_s, &c_iter_e, FALSE);
			ds->setCopyright(gstr);
			g_free(gstr);
			for(size_t i = 0; i < tmp_props.size();) {
				if(!tmp_props[i]) {
					if(tmp_props_orig[i]) ds->delProperty(tmp_props_orig[i]);
					i++;
				} else if(tmp_props[i]->isUserModified()) {
					if(tmp_props_orig[i]) {
						tmp_props_orig[i]->set(*tmp_props[i]);
						i++;
					} else {
						ds->addProperty(tmp_props[i]);
						tmp_props.erase(tmp_props.begin() + i);
					}
				} else {
					i++;
				}
			}
			set_edited_names(ds, str);
			if(add_func) {
				CALCULATOR->addDataSet(ds);
				ds->loadObjects();
				ds->setObjectsLoaded(true);
			}
			dataset_edited(ds);
		}
	}
	for(size_t i = 0; i < tmp_props.size(); i++) {
		if(tmp_props[i]) delete tmp_props[i];
	}
	tmp_props.clear();
	tmp_props_orig.clear();
	edited_dataset = NULL;
	gtk_widget_hide(dialog);
}
