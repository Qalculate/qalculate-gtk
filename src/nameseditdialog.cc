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

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *namesedit_builder = NULL;

GtkWidget *tNames;
GtkListStore *tNames_store;
GtkCellRenderer *names_edit_name_renderer;
GtkTreeViewColumn *names_edit_name_column;

bool names_changed = false;
int names_edited = 0;
int names_type = 0;
ExpressionItem *name_object;

enum {
	NAMES_NAME_COLUMN,
	NAMES_ABBREVIATION_COLUMN,
	NAMES_REFERENCE_COLUMN,
	NAMES_PLURAL_COLUMN,
	NAMES_SUFFIX_COLUMN,
	NAMES_AVOID_INPUT_COLUMN,
	NAMES_COMPLETION_ONLY_COLUMN,
	NAMES_CASE_SENSITIVE_COLUMN,
	NAMES_N_COLUMNS
};

void on_tNames_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_modify")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_remove")), TRUE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_modify")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_remove")), FALSE);
	}
}

void on_name_changed() {
	names_changed = true;
}
void on_names_edit_property_toggled(GtkCellRendererToggle *renderer, gchar *path, gpointer user_data) {
	GtkTreeIter iter;
	int c = GPOINTER_TO_INT(user_data);
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(tNames_store), &iter, path)) {
		gboolean g_b;
		gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, c, &g_b, -1);
		gtk_list_store_set(tNames_store, &iter, c, !g_b, -1);
		on_name_changed();
	}
}
bool names_edit_name_taken(const gchar *str) {
	bool name_taken = false;
	if(names_type == TYPE_VARIABLE && CALCULATOR->variableNameTaken(str, (Variable*) name_object)) name_taken = true;
	else if(names_type == TYPE_UNIT && CALCULATOR->unitNameTaken(str, (Unit*) name_object)) name_taken = true;
	else if(names_type == TYPE_FUNCTION && CALCULATOR->functionNameTaken(str, (MathFunction*) name_object)) name_taken = true;
	return name_taken;
}
void on_names_edit_name_edited(GtkCellRendererText*, gchar *path, gchar *new_text, gpointer) {
	GtkTreeIter iter;
	if(names_type != -1 && gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(tNames_store), &iter, path)) {
		string str = new_text;
		remove_blank_ends(str);
		if((names_type == TYPE_FUNCTION && !CALCULATOR->functionNameIsValid(str)) || (names_type == TYPE_VARIABLE && !CALCULATOR->variableNameIsValid(str)) || (names_type == TYPE_UNIT && !CALCULATOR->unitNameIsValid(str))) {
			if(names_type == TYPE_FUNCTION) str = CALCULATOR->convertToValidFunctionName(str);
			else if(names_type == TYPE_VARIABLE) str = CALCULATOR->convertToValidVariableName(str);
			else if(names_type == TYPE_UNIT) str = CALCULATOR->convertToValidUnitName(str);
			show_message(_("Illegal name"), GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_dialog")));
		}
		if(names_edit_name_taken(str.c_str())) {
			show_message(_("A conflicting object with the same name exists. If you proceed and save changes, the conflicting object will be overwritten or deactivated."), GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_dialog")));
		}
		gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, str.c_str(), -1);
		on_name_changed();
	}
}
void on_names_edit_button_add_clicked(GtkButton*, gpointer) {
	GtkTreeIter iter;
	gtk_list_store_append(tNames_store, &iter);
	GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(tNames_store), &iter);
	gtk_tree_view_set_cursor_on_cell(GTK_TREE_VIEW(tNames), path, names_edit_name_column, names_edit_name_renderer, TRUE);
	gtk_tree_path_free(path);
	on_name_changed();
}
void on_names_edit_button_modify_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tNames)), &model, &iter)) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_set_cursor_on_cell(GTK_TREE_VIEW(tNames), path, names_edit_name_column, names_edit_name_renderer, TRUE);
		gtk_tree_path_free(path);
	}
}
void on_names_edit_button_remove_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tNames));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		gtk_list_store_remove(tNames_store, &iter);
		on_name_changed();
	}
}

GtkWidget* get_names_edit_dialog(void) {
	if(!namesedit_builder) {

		namesedit_builder = getBuilder("namesedit.ui");
		g_assert(namesedit_builder != NULL);

		g_assert(gtk_builder_get_object(namesedit_builder, "names_edit_dialog") != NULL);

		tNames = GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_treeview"));

		tNames_store = gtk_list_store_new(NAMES_N_COLUMNS, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tNames), GTK_TREE_MODEL(tNames_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tNames));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		names_edit_name_renderer = renderer;
		g_signal_connect((gpointer) renderer, "edited", G_CALLBACK(on_names_edit_name_edited), NULL);
		g_object_set(G_OBJECT(renderer), "editable", true, NULL);
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", NAMES_NAME_COLUMN, NULL);
		names_edit_name_column = column;
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tNames_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_NAME_COLUMN);
		gtk_tree_view_column_set_expand(column, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_toggle_new();
		g_signal_connect((gpointer) renderer, "toggled", G_CALLBACK(on_names_edit_property_toggled), GINT_TO_POINTER(NAMES_ABBREVIATION_COLUMN));
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "activatable", TRUE, NULL);
		column = gtk_tree_view_column_new_with_attributes(_("Abbreviation"), renderer, "active", NAMES_ABBREVIATION_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_ABBREVIATION_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_toggle_new();
		g_signal_connect((gpointer) renderer, "toggled", G_CALLBACK(on_names_edit_property_toggled), GINT_TO_POINTER(NAMES_PLURAL_COLUMN));
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "activatable", TRUE, NULL);
		column = gtk_tree_view_column_new_with_attributes(_("Plural"), renderer, "active", NAMES_PLURAL_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_PLURAL_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes(_("Reference"), renderer, "active", NAMES_REFERENCE_COLUMN, NULL);
		g_signal_connect((gpointer) renderer, "toggled", G_CALLBACK(on_names_edit_property_toggled), GINT_TO_POINTER(NAMES_REFERENCE_COLUMN));
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "activatable", TRUE, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_REFERENCE_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes(_("Avoid input"), renderer, "active", NAMES_AVOID_INPUT_COLUMN, NULL);
		g_signal_connect((gpointer) renderer, "toggled", G_CALLBACK(on_names_edit_property_toggled), GINT_TO_POINTER(NAMES_AVOID_INPUT_COLUMN));
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "activatable", TRUE, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_AVOID_INPUT_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes(_("Suffix"), renderer, "active", NAMES_SUFFIX_COLUMN, NULL);
		g_signal_connect((gpointer) renderer, "toggled", G_CALLBACK(on_names_edit_property_toggled), GINT_TO_POINTER(NAMES_SUFFIX_COLUMN));
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "activatable", TRUE, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_SUFFIX_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes(_("Case sensitive"), renderer, "active", NAMES_CASE_SENSITIVE_COLUMN, NULL);
		g_signal_connect((gpointer) renderer, "toggled", G_CALLBACK(on_names_edit_property_toggled), GINT_TO_POINTER(NAMES_CASE_SENSITIVE_COLUMN));
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "activatable", TRUE, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_CASE_SENSITIVE_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes(_("Completion only"), renderer, "active", NAMES_COMPLETION_ONLY_COLUMN, NULL);
		g_signal_connect((gpointer) renderer, "toggled", G_CALLBACK(on_names_edit_property_toggled), GINT_TO_POINTER(NAMES_COMPLETION_ONLY_COLUMN));
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "activatable", TRUE, NULL);
		gtk_tree_view_column_set_sort_column_id(column, NAMES_COMPLETION_ONLY_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tNames), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tNames_selection_changed), NULL);

		gtk_builder_add_callback_symbols(namesedit_builder, "on_names_edit_button_add_clicked", G_CALLBACK(on_names_edit_button_add_clicked), "on_names_edit_button_modify_clicked", G_CALLBACK(on_names_edit_button_modify_clicked), "on_names_edit_button_remove_clicked", G_CALLBACK(on_names_edit_button_remove_clicked), NULL);
		gtk_builder_connect_signals(namesedit_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_dialog"));
}

bool edit_names(ExpressionItem *item, int type, const gchar *namestr, GtkWindow *win, DataProperty *dp) {

	names_type = type;
	name_object = item;
	if(dp) names_type = -1;
	bool is_dp = (names_type == -1);
	bool is_unit = (names_type == TYPE_UNIT);
	names_changed = false;

	GtkWidget *dialog = get_names_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), win);

	GtkTreeIter iter;

	gtk_widget_set_sensitive(tNames, !(item && item->isBuiltin() && !(item->type() == TYPE_FUNCTION && item->subtype() == SUBTYPE_DATA_SET)));

	if(!names_edited) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_modify")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "names_edit_button_remove")), FALSE);

		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(tNames), 1), !is_dp);
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(tNames), 2), is_unit);
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(tNames), 3), TRUE);
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(tNames), 4), !is_dp);
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(tNames), 5), !is_dp);
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(tNames), 6), !is_dp);
		gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(tNames), 7), !is_dp);

		gtk_list_store_clear(tNames_store);

		if(!is_dp && item && item->countNames() > 0) {
			for(size_t i = 1; i <= item->countNames(); i++) {
				const ExpressionName *ename = &item->getName(i);
				gtk_list_store_append(tNames_store, &iter);
				gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, ename->name.c_str(), NAMES_ABBREVIATION_COLUMN, ename->abbreviation, NAMES_PLURAL_COLUMN, ename->plural, NAMES_REFERENCE_COLUMN, ename->reference, NAMES_SUFFIX_COLUMN, ename->suffix, NAMES_AVOID_INPUT_COLUMN, ename->avoid_input, NAMES_CASE_SENSITIVE_COLUMN, ename->case_sensitive, NAMES_COMPLETION_ONLY_COLUMN, ename->completion_only, -1);
				if(i == 1 && namestr && strlen(namestr) > 0 && item->getName(1).name != namestr) {
					if(names_edit_name_taken(namestr)) {
						show_message(_("A conflicting object with the same name exists. If you proceed and save changes, the conflicting object will be overwritten or deactivated."), GTK_WIDGET(win));
					}
					gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, namestr, -1);
				}
			}
		} else if(is_dp && dp && dp->countNames() > 0) {
			for(size_t i = 1; i <= dp->countNames(); i++) {
				gtk_list_store_append(tNames_store, &iter);
				gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, dp->getName(i).c_str(), NAMES_ABBREVIATION_COLUMN, FALSE, NAMES_PLURAL_COLUMN, FALSE, NAMES_REFERENCE_COLUMN, dp->nameIsReference(i), NAMES_SUFFIX_COLUMN, FALSE, NAMES_AVOID_INPUT_COLUMN, FALSE, NAMES_CASE_SENSITIVE_COLUMN, FALSE, NAMES_COMPLETION_ONLY_COLUMN, FALSE, -1);
				if(i == 1 && namestr && strlen(namestr) > 0) {
					gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, namestr, -1);
				}
			}
		} else if(namestr && strlen(namestr) > 0) {
			gtk_list_store_append(tNames_store, &iter);
			if(is_dp) {
				gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, namestr, NAMES_ABBREVIATION_COLUMN, FALSE, NAMES_PLURAL_COLUMN, FALSE, NAMES_REFERENCE_COLUMN, TRUE, NAMES_SUFFIX_COLUMN, FALSE, NAMES_AVOID_INPUT_COLUMN, FALSE, NAMES_CASE_SENSITIVE_COLUMN, FALSE, NAMES_COMPLETION_ONLY_COLUMN, FALSE, -1);
			} else {
				if(names_edit_name_taken(namestr)) {
					show_message(_("A conflicting object with the same name exists. If you proceed and save changes, the conflicting object will be overwritten or deactivated."), GTK_WIDGET(win));
				}
				ExpressionName ename(namestr);
				ename.reference = true;
				gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, ename.name.c_str(), NAMES_ABBREVIATION_COLUMN, ename.abbreviation, NAMES_PLURAL_COLUMN, ename.plural, NAMES_REFERENCE_COLUMN, ename.reference, NAMES_SUFFIX_COLUMN, ename.suffix, NAMES_AVOID_INPUT_COLUMN, ename.avoid_input, NAMES_CASE_SENSITIVE_COLUMN, ename.case_sensitive, NAMES_COMPLETION_ONLY_COLUMN, ename.completion_only, -1);
			}
		}
	} else if(namestr && strlen(namestr) > 0) {
		if(!is_dp && names_edited == 2 && names_edit_name_taken(namestr)) {
			show_message(_("A conflicting object with the same name exists. If you proceed and save changes, the conflicting object will be overwritten or deactivated."), GTK_WIDGET(win));
		}
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) {
			gtk_list_store_set(tNames_store, &iter, NAMES_NAME_COLUMN, namestr, -1);
		}
		on_tNames_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(tNames)), NULL);
	}

	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(namesedit_builder, "button_close")));

	gtk_dialog_run(GTK_DIALOG(dialog));
	names_edited = 1;

	gtk_widget_hide(dialog);

	return names_changed;
}
bool has_name() {
	GtkTreeIter iter;
	return gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter);
}
string first_name() {
	gchar *gstr = NULL;
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, NAMES_NAME_COLUMN, &gstr, -1);
	string str;
	if(gstr) {
		str = gstr;
		g_free(gstr);
	}
	return str;
}
void set_name_label_and_entry(ExpressionItem *item, GtkWidget *entry, GtkWidget *label) {
	const ExpressionName *ename = &item->getName(1);
	gtk_entry_set_text(GTK_ENTRY(entry), ename->name.c_str());
	if(label && item->countNames() > 1) {
		string str = "+ ";
		for(size_t i = 2; i <= item->countNames(); i++) {
			if(i > 2) str += ", ";
			str += item->getName(i).name;
		}
		gtk_label_set_text(GTK_LABEL(label), str.c_str());
	}
}
void set_edited_names(ExpressionItem *item, string str) {
	if(item->isBuiltin() && !(item->type() == TYPE_FUNCTION && item->subtype() == SUBTYPE_DATA_SET)) return;
	if(item->type() == TYPE_UNIT && item->subtype() == SUBTYPE_COMPOSITE_UNIT) {
		names_edited = 0;
		item->clearNames();
	}
	if(names_edited) {
		item->clearNames();
		GtkTreeIter iter;
		size_t i = 0;
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) {
			ExpressionName ename;
			gchar *gstr;
			while(true) {
				gboolean abbreviation = FALSE, suffix = FALSE, plural = FALSE;
				gboolean reference = FALSE, avoid_input = FALSE, case_sensitive = FALSE, completion_only = FALSE;
				gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, NAMES_NAME_COLUMN, &gstr, NAMES_ABBREVIATION_COLUMN, &abbreviation, NAMES_SUFFIX_COLUMN, &suffix, NAMES_PLURAL_COLUMN, &plural, NAMES_REFERENCE_COLUMN, &reference, NAMES_AVOID_INPUT_COLUMN, &avoid_input, NAMES_CASE_SENSITIVE_COLUMN, &case_sensitive, NAMES_COMPLETION_ONLY_COLUMN, &completion_only, -1);
				if(i == 0 && names_edited == 2 && !str.empty()) ename.name = str;
				else ename.name = gstr;
				ename.abbreviation = abbreviation; ename.suffix = suffix;
				ename.plural = plural; ename.reference = reference;
				ename.avoid_input = avoid_input; ename.case_sensitive = case_sensitive;
				ename.completion_only = completion_only;
				ename.unicode = false;
				for(size_t i2 = 0; i2 < str.length(); i2++) {
					if((unsigned char) str[i2] >= 0xC0) {
						ename.unicode = TRUE;
						break;
					}
				}
				item->addName(ename);
				g_free(gstr);
				if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tNames_store), &iter)) break;
				i++;
			}
		} else {
			ExpressionName ename(str);
			ename.reference = true;
			item->addName(ename);
		}
	} else {
		if(item->countNames() == 0) {
			ExpressionName ename(str);
			ename.reference = true;
			item->addName(ename);
		} else {
			item->setName(str, 1);
		}
	}
}

void set_edited_names(DataProperty *dp, string str) {
	if(names_edited) {
		dp->clearNames();
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tNames_store), &iter)) {
			gchar *gstr;
			while(true) {
				gboolean reference = FALSE;
				gtk_tree_model_get(GTK_TREE_MODEL(tNames_store), &iter, NAMES_NAME_COLUMN, &gstr, NAMES_REFERENCE_COLUMN, &reference, -1);
				dp->addName(gstr, reference);
				g_free(gstr);
				if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tNames_store), &iter)) break;
			}
		} else {
			dp->addName(str);
		}
	} else if(dp->countNames() == 0) {
			dp->setName(str, true);
	} else {
		vector<string> names;
		vector<bool> name_refs;
		for(size_t i = 1; i <= dp->countNames(); i++) {
			if(i == 1) names.push_back(str);
			else names.push_back(dp->getName(i));
			name_refs.push_back(dp->nameIsReference(i));
		}
		dp->clearNames();
		for(size_t i = 0; i < names.size(); i++) {
			dp->addName(names[i], name_refs[i]);
		}
	}
}
void correct_name_entry(GtkEditable *editable, ExpressionItemType etype, gpointer function) {
	string str = gtk_entry_get_text(GTK_ENTRY(editable));
	if(str.empty()) return;
	remove_blank_ends(str);
	bool b = false;
	if(!str.empty()) {
		switch(etype) {
			case TYPE_FUNCTION: {
				b = CALCULATOR->functionNameIsValid(str);
				if(!b) str = CALCULATOR->convertToValidFunctionName(str);
				break;
			}
			case TYPE_UNIT: {
				b = CALCULATOR->unitNameIsValid(str);
				if(!b) str = CALCULATOR->convertToValidUnitName(str);
				break;
			}
			case TYPE_VARIABLE: {
				b = CALCULATOR->variableNameIsValid(str);
				if(!b) str = CALCULATOR->convertToValidVariableName(str);
				break;
			}
		}
	}
	if(!b) {
		g_signal_handlers_block_matched((gpointer) editable, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, function, NULL);
		gtk_entry_set_text(GTK_ENTRY(editable), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) editable, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, function, NULL);
	}
}
void reset_names_status() {names_edited = 0;}
void name_entry_changed() {
	if(names_edited == 1) names_edited = false;
}
int names_status() {return names_edited;}
void set_names_status(int i) {names_edited = i;}
