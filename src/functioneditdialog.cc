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
#include "openhelp.h"
#include "nameseditdialog.h"
#include "dataseteditdialog.h"
#include "functioneditdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

enum {
	MENU_ARGUMENT_TYPE_FREE,
	MENU_ARGUMENT_TYPE_NUMBER,
	MENU_ARGUMENT_TYPE_INTEGER,
	MENU_ARGUMENT_TYPE_SYMBOLIC,
	MENU_ARGUMENT_TYPE_TEXT,
	MENU_ARGUMENT_TYPE_DATE,
	MENU_ARGUMENT_TYPE_VECTOR,
	MENU_ARGUMENT_TYPE_MATRIX,
	MENU_ARGUMENT_TYPE_POSITIVE,
	MENU_ARGUMENT_TYPE_NONZERO,
	MENU_ARGUMENT_TYPE_NONNEGATIVE,
	MENU_ARGUMENT_TYPE_POSITIVE_INTEGER,
	MENU_ARGUMENT_TYPE_NONZERO_INTEGER,
	MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER,
	MENU_ARGUMENT_TYPE_BOOLEAN,
	MENU_ARGUMENT_TYPE_EXPRESSION_ITEM,
	MENU_ARGUMENT_TYPE_FUNCTION,
	MENU_ARGUMENT_TYPE_UNIT,
	MENU_ARGUMENT_TYPE_VARIABLE,
	MENU_ARGUMENT_TYPE_FILE,
	MENU_ARGUMENT_TYPE_ANGLE,
	MENU_ARGUMENT_TYPE_DATA_OBJECT,
	MENU_ARGUMENT_TYPE_DATA_PROPERTY
};

GtkBuilder *functionedit_builder = NULL, *argumentrules_builder = NULL;

GtkWidget *tFunctionArguments;
GtkListStore *tFunctionArguments_store;
GtkWidget *tSubfunctions;
GtkListStore *tSubfunctions_store;

size_t last_subfunction_index;
Argument *selected_argument;
MathFunction *edited_function = NULL;

Argument *edit_argument(Argument *arg = NULL);

void on_argument_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_button_ok")), TRUE);
}
void on_function_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_ok")), strlen(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")))) > 0);
}
void on_subfunction_changed() {
	GtkTextIter istart;
	gtk_text_buffer_get_start_iter(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_subexpression"))), &istart);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_subok")), !gtk_text_iter_is_end(&istart));
}
void on_function_edit_entry_name_changed(GtkEditable *editable, gpointer) {
	correct_name_entry(editable, TYPE_FUNCTION, (gpointer) on_function_edit_entry_name_changed);
	name_entry_changed();
}
void on_tSubfunctions_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	if(gtk_tree_selection_get_selected(treeselection, &model, NULL)) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_subfunction")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_subfunction")), TRUE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_subfunction")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_subfunction")), FALSE);
	}
}

void on_tFunctionArguments_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	selected_argument = NULL;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		Argument *arg;
		gtk_tree_model_get(model, &iter, 2, &arg, -1);
		selected_argument = arg;
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_argument")), !(edited_function && edited_function->isBuiltin()));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_argument")), !(edited_function && edited_function->isBuiltin()));
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_argument")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_argument")), FALSE);
	}
}
void update_argument_refs() {
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionArguments_store), &iter)) return;
	int i = 0;
	do {
		string refstr = "\\";
		if(i < 3) refstr += 'x' + i;
		else refstr += 'a' + (i - 3);
		gtk_list_store_set(tFunctionArguments_store, &iter, 3, refstr.c_str(), -1);
		i++;
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tFunctionArguments_store), &iter));
}
void update_function_arguments_list(MathFunction *f) {
	if(!functionedit_builder) return;
	selected_argument = NULL;
	gtk_list_store_clear(tFunctionArguments_store);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_argument")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_argument")), FALSE);
	if(f) {
		GtkTreeIter iter;
		Argument *arg;
		int args = f->maxargs();
		if(args < 0) {
			args = f->minargs() + 1;
		}
		if((int) f->lastArgumentDefinitionIndex() > args) args = (int) f->lastArgumentDefinitionIndex();
		Argument defarg;
		string str, str2;
		for(int i = 1; i <= args; i++) {
			gtk_list_store_append(tFunctionArguments_store, &iter);
			arg = f->getArgumentDefinition(i);
			if(arg) {
				arg = arg->copy();
				str = arg->printlong();
				str2 = arg->name();
			} else {
				str = defarg.printlong();
				str2 = "";
			}
			gtk_list_store_set(tFunctionArguments_store, &iter, 0, str2.c_str(), 1, str.c_str(), 2, (gpointer) arg, -1);
		}
		update_argument_refs();
	}
}

gboolean on_function_edit_textview_expression_key_press_event(GtkWidget *w, GdkEventKey *event, gpointer renderer) {
	if(textview_in_quotes(GTK_TEXT_VIEW(w))) return FALSE;
	const gchar *key = key_press_get_symbol(event);
	if(!key) return FALSE;
	if(strlen(key) > 0) {
		GtkTextBuffer *expression_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
		gtk_text_buffer_delete_selection(expression_buffer, FALSE, TRUE);
		gtk_text_buffer_insert_at_cursor(expression_buffer, key, -1);
		return TRUE;
	}
	return FALSE;
}
void on_function_edit_treeview_subfunctions_expression_edited(GtkCellRendererText*, gchar *path, gchar *new_text, gpointer) {
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(tSubfunctions_store), &iter, path)) {
		gtk_list_store_set(tSubfunctions_store, &iter, 1, new_text, -1);
		on_function_changed();
	}
}
void on_function_edit_treeview_subfunctions_precalculate_toggled(GtkCellRendererToggle *renderer, gchar *path, gpointer) {
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(tSubfunctions_store), &iter, path)) {
		gboolean g_b;
		gtk_tree_model_get(GTK_TREE_MODEL(tSubfunctions_store), &iter, 2, &g_b, -1);
		gtk_list_store_set(tSubfunctions_store, &iter, 2, !g_b, -1);
		on_function_changed();
	}
}
void on_function_edit_button_modify_subfunction_clicked(GtkButton*, gpointer) {
	gtk_window_set_transient_for(GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "subfunction_edit_dialog")), GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")));
	update_window_properties(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "subfunction_edit_dialog")));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tSubfunctions)), &model, &iter)) {
		gchar *gstr;
		gboolean g_b = FALSE;
		gtk_tree_model_get(GTK_TREE_MODEL(tSubfunctions_store), &iter, 1, &gstr, 2, &g_b, -1);
		GtkTextBuffer *expression_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_subexpression")));
		gtk_text_buffer_set_text(expression_buffer, gstr, -1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_precalculate")), g_b);
		g_free(gstr);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_subok")), FALSE);
		if(gtk_dialog_run(GTK_DIALOG(gtk_builder_get_object(functionedit_builder, "subfunction_edit_dialog"))) == GTK_RESPONSE_OK) {
			GtkTextIter istart, iend;
			gtk_text_buffer_get_start_iter(expression_buffer, &istart);
			gtk_text_buffer_get_end_iter(expression_buffer, &iend);
			gstr = gtk_text_buffer_get_text(expression_buffer, &istart, &iend, FALSE);
			gtk_list_store_set(tSubfunctions_store, &iter, 1, gstr, 2, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_precalculate"))), -1);
			g_free(gstr);
			on_function_changed();
		}
	}
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "subfunction_edit_dialog")));
}
void on_function_edit_button_add_subfunction_clicked(GtkButton*, gpointer) {
	gtk_window_set_transient_for(GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "subfunction_edit_dialog")), GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")));
	update_window_properties(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "subfunction_edit_dialog")));
	GtkTextBuffer *expression_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_subexpression")));
	gtk_text_buffer_set_text(expression_buffer, "", -1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_precalculate")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_subok")), FALSE);
	if(gtk_dialog_run(GTK_DIALOG(gtk_builder_get_object(functionedit_builder, "subfunction_edit_dialog"))) == GTK_RESPONSE_OK) {
		GtkTreeIter iter;
		gtk_list_store_append(tSubfunctions_store, &iter);
		string str = "\\";
		last_subfunction_index++;
		str += i2s(last_subfunction_index);
		GtkTextIter istart, iend;
		gtk_text_buffer_get_start_iter(expression_buffer, &istart);
		gtk_text_buffer_get_end_iter(expression_buffer, &iend);
		gchar *gstr = gtk_text_buffer_get_text(expression_buffer, &istart, &iend, FALSE);
		gtk_list_store_set(tSubfunctions_store, &iter, 0, str.c_str(), 1, gstr, 3, last_subfunction_index, 2, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_precalculate"))), -1);
		g_free(gstr);
		on_function_changed();
	}
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "subfunction_edit_dialog")));
}
void on_function_edit_button_remove_subfunction_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tSubfunctions)), &model, &iter)) {
		GtkTreeIter iter2 = iter;
		while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tSubfunctions_store), &iter2)) {
			guint index;
			gtk_tree_model_get(GTK_TREE_MODEL(tSubfunctions_store), &iter2, 3, &index, -1);
			index--;
			string str = "\\";
			str += i2s(index);
			gtk_list_store_set(tSubfunctions_store, &iter2, 0, str.c_str(), 3, index, -1);
		}
		gtk_list_store_remove(tSubfunctions_store, &iter);
		last_subfunction_index--;
		on_function_changed();
	}
}
void on_function_edit_button_add_argument_clicked(GtkButton*, gpointer) {
	Argument *arg = edit_argument(NULL);
	if(arg) {
		GtkTreeIter iter;
		gtk_list_store_append(tFunctionArguments_store, &iter);
		gtk_list_store_set(tFunctionArguments_store, &iter, 0, arg->name().c_str(), 1, arg->printlong().c_str(), 2, (gpointer) arg, -1);
		update_argument_refs();
		on_function_changed();
	}
}
void on_function_edit_button_remove_argument_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionArguments));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		if(selected_argument) {
			delete selected_argument;
			selected_argument = NULL;
		}
		gtk_list_store_remove(tFunctionArguments_store, &iter);
		update_argument_refs();
		on_function_changed();
	}
}
void on_function_edit_button_modify_argument_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionArguments));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		Argument *arg = edit_argument(selected_argument);
		if(arg) {
			if(selected_argument) delete selected_argument;
			selected_argument = arg;
			gtk_list_store_set(tFunctionArguments_store, &iter, 0, selected_argument->name().c_str(), 1, selected_argument->printlong().c_str(), 2, (gpointer) selected_argument, -1);
			on_function_changed();
		}
	}
}
void on_function_edit_treeview_arguments_name_edited(GtkCellRendererText*, gchar *path, gchar *new_text, gpointer) {
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(tFunctionArguments_store), &iter, path)) {
		Argument *arg;
		gtk_tree_model_get(GTK_TREE_MODEL(tFunctionArguments_store), &iter, 2, &arg, -1);
		if(!arg) arg = new Argument();
		arg->setName(new_text);
		gtk_list_store_set(tFunctionArguments_store, &iter, 0, new_text, 2, (gpointer) arg, -1);
		on_function_changed();
	}
}
void on_function_edit_treeview_arguments_row_activated(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn *column, gpointer) {
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tFunctionArguments_store), &iter, path)) return;
	Argument *edited_arg = NULL;
	gtk_tree_model_get(GTK_TREE_MODEL(tFunctionArguments_store), &iter, 2, &edited_arg, -1);
	Argument *arg = edit_argument(edited_arg);
	if(arg) {
		if(edited_arg) delete edited_arg;
		selected_argument = arg;
		gtk_list_store_set(tFunctionArguments_store, &iter, 0, selected_argument->name().c_str(), 1, selected_argument->printlong().c_str(), 2, (gpointer) selected_argument, -1);
		on_function_changed();
	}
}
void on_argument_rules_combobox_type_changed(GtkComboBox *om, gpointer) {
	int argtype = ARGUMENT_TYPE_FREE;
	int menu_index = gtk_combo_box_get_active(om);
	switch(menu_index) {
		case MENU_ARGUMENT_TYPE_TEXT: {argtype = ARGUMENT_TYPE_TEXT; break;}
		case MENU_ARGUMENT_TYPE_SYMBOLIC: {argtype = ARGUMENT_TYPE_SYMBOLIC; break;}
		case MENU_ARGUMENT_TYPE_DATE: {argtype = ARGUMENT_TYPE_DATE; break;}
		case MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER: {}
		case MENU_ARGUMENT_TYPE_POSITIVE_INTEGER: {}
		case MENU_ARGUMENT_TYPE_NONZERO_INTEGER: {}
		case MENU_ARGUMENT_TYPE_INTEGER: {argtype = ARGUMENT_TYPE_INTEGER; break;}
		case MENU_ARGUMENT_TYPE_NONNEGATIVE: {}
		case MENU_ARGUMENT_TYPE_POSITIVE: {}
		case MENU_ARGUMENT_TYPE_NONZERO: {}
		case MENU_ARGUMENT_TYPE_NUMBER: {argtype = ARGUMENT_TYPE_NUMBER; break;}
		case MENU_ARGUMENT_TYPE_VECTOR: {argtype = ARGUMENT_TYPE_VECTOR; break;}
		case MENU_ARGUMENT_TYPE_MATRIX: {argtype = ARGUMENT_TYPE_MATRIX; break;}
		case MENU_ARGUMENT_TYPE_EXPRESSION_ITEM: {argtype = ARGUMENT_TYPE_EXPRESSION_ITEM; break;}
		case MENU_ARGUMENT_TYPE_FUNCTION: {argtype = ARGUMENT_TYPE_FUNCTION; break;}
		case MENU_ARGUMENT_TYPE_UNIT: {argtype = ARGUMENT_TYPE_UNIT; break;}
		case MENU_ARGUMENT_TYPE_VARIABLE: {argtype = ARGUMENT_TYPE_VARIABLE; break;}
		case MENU_ARGUMENT_TYPE_FILE: {argtype = ARGUMENT_TYPE_FILE; break;}
		case MENU_ARGUMENT_TYPE_BOOLEAN: {argtype = ARGUMENT_TYPE_BOOLEAN; break;}
		case MENU_ARGUMENT_TYPE_ANGLE: {argtype = ARGUMENT_TYPE_ANGLE; break;}
		case MENU_ARGUMENT_TYPE_DATA_OBJECT: {argtype = ARGUMENT_TYPE_DATA_OBJECT; break;}
		case MENU_ARGUMENT_TYPE_DATA_PROPERTY: {argtype = ARGUMENT_TYPE_DATA_PROPERTY; break;}
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix")), argtype != ARGUMENT_TYPE_FREE && argtype != ARGUMENT_TYPE_MATRIX);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix")), argtype == ARGUMENT_TYPE_FREE || argtype == ARGUMENT_TYPE_MATRIX);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_handle_vector")), argtype == ARGUMENT_TYPE_NUMBER || argtype == ARGUMENT_TYPE_INTEGER || argtype == ARGUMENT_TYPE_TEXT || argtype == ARGUMENT_TYPE_DATE || argtype == ARGUMENT_TYPE_BOOLEAN);
	if(argtype == ARGUMENT_TYPE_INTEGER || argtype == ARGUMENT_TYPE_NUMBER) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_forbid_zero")), menu_index == MENU_ARGUMENT_TYPE_NONZERO_INTEGER && menu_index == MENU_ARGUMENT_TYPE_NONZERO);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), argtype == ARGUMENT_TYPE_NUMBER || argtype == ARGUMENT_TYPE_INTEGER);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), argtype == ARGUMENT_TYPE_NUMBER || argtype == ARGUMENT_TYPE_INTEGER);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), menu_index == MENU_ARGUMENT_TYPE_POSITIVE || menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE || menu_index == MENU_ARGUMENT_TYPE_POSITIVE_INTEGER || menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER);
	if(menu_index == MENU_ARGUMENT_TYPE_POSITIVE_INTEGER) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 1.0);
	} else {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0.0);
	}
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0.0);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), argtype == ARGUMENT_TYPE_NUMBER);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), argtype == ARGUMENT_TYPE_NUMBER);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), menu_index == MENU_ARGUMENT_TYPE_POSITIVE || menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE || menu_index == MENU_ARGUMENT_TYPE_POSITIVE_INTEGER || menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), FALSE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE || argtype == ARGUMENT_TYPE_INTEGER);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), argtype == ARGUMENT_TYPE_INTEGER);
	if(argtype == ARGUMENT_TYPE_NUMBER) {
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 8);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 8);
		gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MIN);
		gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MAX);
		gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MIN);
		gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MAX);
	} else if(argtype == ARGUMENT_TYPE_INTEGER) {
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
		gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MIN);
		gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MAX);
		gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MIN);
		gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MAX);
	}
	if(menu_index == MENU_ARGUMENT_TYPE_POSITIVE || menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE || menu_index == MENU_ARGUMENT_TYPE_POSITIVE_INTEGER || menu_index == MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER) {
		g_signal_handlers_block_matched((gpointer) om, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_argument_rules_combobox_type_changed, NULL);
		if(argtype == ARGUMENT_TYPE_INTEGER) gtk_combo_box_set_active(om, MENU_ARGUMENT_TYPE_INTEGER);
		else gtk_combo_box_set_active(om, MENU_ARGUMENT_TYPE_NUMBER);
		g_signal_handlers_unblock_matched((gpointer) om, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_argument_rules_combobox_type_changed, NULL);
	}
}
void on_argument_rules_checkbutton_enable_min_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), gtk_toggle_button_get_active(w));
}
void on_argument_rules_checkbutton_enable_max_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), gtk_toggle_button_get_active(w));
}
void on_argument_rules_checkbutton_enable_condition_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_condition")), gtk_toggle_button_get_active(w));
}

void on_function_edit_button_names_clicked(GtkWidget*, gpointer) {
	if(!edit_names(edited_function, TYPE_FUNCTION, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name"))), GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")))) return;
	string str = first_name();
	if(!str.empty()) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(functionedit_builder, "function_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_function_edit_entry_name_changed, NULL);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(functionedit_builder, "function_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_function_edit_entry_name_changed, NULL);
	}
	on_function_changed();
}

GtkWidget* get_function_edit_dialog(void) {

	if(!functionedit_builder) {

		functionedit_builder = getBuilder("functionedit.ui");
		g_assert(functionedit_builder != NULL);

		g_assert(gtk_builder_get_object(functionedit_builder, "function_edit_dialog") != NULL);

		tFunctionArguments = GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_treeview_arguments"));
		tFunctionArguments_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tFunctionArguments), GTK_TREE_MODEL(tFunctionArguments_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionArguments));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
		g_object_set(G_OBJECT(renderer), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
		g_signal_connect((gpointer) renderer, "edited", G_CALLBACK(on_function_edit_treeview_arguments_name_edited), NULL);
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_expand(column, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctionArguments), column);
		renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
		column = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, "text", 1, NULL);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_expand(column, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctionArguments), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("Reference", renderer, "text", 3, NULL);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_expand(column, FALSE);
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "style", PANGO_STYLE_ITALIC, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctionArguments), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tFunctionArguments_selection_changed), NULL);

		tSubfunctions = GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_treeview_subfunctions"));
		tSubfunctions_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_UINT);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tSubfunctions), GTK_TREE_MODEL(tSubfunctions_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tSubfunctions));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
		g_signal_connect((gpointer) renderer, "edited", G_CALLBACK(on_function_edit_treeview_subfunctions_expression_edited), NULL);
		column = gtk_tree_view_column_new_with_attributes(_("Expression"), renderer, "text", 1, NULL);
		gtk_tree_view_column_set_expand(column, TRUE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tSubfunctions), column);
		renderer = gtk_cell_renderer_toggle_new();
		g_signal_connect((gpointer) renderer, "toggled", G_CALLBACK(on_function_edit_treeview_subfunctions_precalculate_toggled), NULL);
		column = gtk_tree_view_column_new_with_attributes(_("Precalculate"), renderer, "active", 2, NULL);
		gtk_tree_view_column_set_expand(column, FALSE);
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, "activatable", TRUE, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tSubfunctions), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Reference"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_expand(column, FALSE);
		g_object_set(G_OBJECT(renderer), "xalign", 0.5, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tSubfunctions), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tSubfunctions_selection_changed), NULL);

		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_description"))), "changed", G_CALLBACK(on_function_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_expression"))), "changed", G_CALLBACK(on_function_changed), NULL);
		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_subexpression"))), "changed", G_CALLBACK(on_subfunction_changed), NULL);

		gtk_builder_add_callback_symbols(functionedit_builder, "on_function_changed", G_CALLBACK(on_function_changed), "on_function_edit_entry_name_changed", G_CALLBACK(on_function_edit_entry_name_changed), "on_function_edit_button_names_clicked", G_CALLBACK(on_function_edit_button_names_clicked), "on_function_edit_textview_expression_key_press_event", G_CALLBACK(on_function_edit_textview_expression_key_press_event), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_function_edit_button_add_subfunction_clicked", G_CALLBACK(on_function_edit_button_add_subfunction_clicked), "on_function_edit_button_modify_subfunction_clicked", G_CALLBACK(on_function_edit_button_modify_subfunction_clicked), "on_function_edit_button_remove_subfunction_clicked", G_CALLBACK(on_function_edit_button_remove_subfunction_clicked), "on_function_edit_treeview_arguments_row_activated", G_CALLBACK(on_function_edit_treeview_arguments_row_activated), "on_function_edit_button_add_argument_clicked", G_CALLBACK(on_function_edit_button_add_argument_clicked), "on_function_edit_button_modify_argument_clicked", G_CALLBACK(on_function_edit_button_modify_argument_clicked), "on_function_edit_button_remove_argument_clicked", G_CALLBACK(on_function_edit_button_remove_argument_clicked), "on_subfunction_changed", G_CALLBACK(on_subfunction_changed), NULL);
		gtk_builder_connect_signals(functionedit_builder, NULL);

	}

	/* populate combo menu */

	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
		if(!CALCULATOR->functions[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->functions[i]->category().c_str()) == NULL) {
				items = g_list_insert_sorted(items, (gpointer) CALCULATOR->functions[i]->category().c_str(), (GCompareFunc) compare_categories);
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->functions[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(functionedit_builder, "function_edit_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_dialog"));
}

GtkWidget* get_argument_rules_dialog(void) {

	if(!argumentrules_builder) {

		argumentrules_builder = getBuilder("argumentrules.ui");
		g_assert(argumentrules_builder != NULL);

		g_assert(gtk_builder_get_object(argumentrules_builder, "argument_rules_dialog") != NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(argumentrules_builder, "argument_rules_combobox_type")), 0);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 8);
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 8);
		gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MIN);
		gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MAX);
		gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MIN);
		gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MAX);

		gtk_builder_add_callback_symbols(argumentrules_builder, "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_rules_combobox_type_changed", G_CALLBACK(on_argument_rules_combobox_type_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_rules_checkbutton_enable_condition_toggled", G_CALLBACK(on_argument_rules_checkbutton_enable_condition_toggled), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_rules_checkbutton_enable_min_toggled", G_CALLBACK(on_argument_rules_checkbutton_enable_min_toggled), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_rules_checkbutton_enable_max_toggled", G_CALLBACK(on_argument_rules_checkbutton_enable_max_toggled), "on_argument_changed", G_CALLBACK(on_argument_changed), "on_argument_changed", G_CALLBACK(on_argument_changed), NULL);
		gtk_builder_connect_signals(argumentrules_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_dialog"));
}

Argument *edit_argument(Argument *arg) {
	GtkWidget *dialog = get_argument_rules_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")));
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_name")), arg ? arg->name().c_str() : "");
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_condition")), arg && !arg->getCustomCondition().empty());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_test")), !arg || arg->tests());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix")), arg && arg->type() != ARGUMENT_TYPE_FREE && arg->type() != ARGUMENT_TYPE_MATRIX);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix")), !arg || arg->matrixAllowed() || arg->type() == ARGUMENT_TYPE_FREE || arg->type() == ARGUMENT_TYPE_MATRIX);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_condition")), arg ? localize_expression(arg->getCustomCondition()).c_str() : "");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_condition")), arg && !arg->getCustomCondition().empty());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_forbid_zero")), arg && arg->zeroForbidden());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_handle_vector")), arg && arg->handlesVector());
	int menu_index = MENU_ARGUMENT_TYPE_FREE;
	if(arg) {
		switch(arg->type()) {
			case ARGUMENT_TYPE_TEXT: {
				menu_index = MENU_ARGUMENT_TYPE_TEXT;
				break;
			}
			case ARGUMENT_TYPE_SYMBOLIC: {
				menu_index = MENU_ARGUMENT_TYPE_SYMBOLIC;
				break;
			}
			case ARGUMENT_TYPE_DATE: {
				menu_index = MENU_ARGUMENT_TYPE_DATE;
				break;
			}
			case ARGUMENT_TYPE_INTEGER: {
				menu_index = MENU_ARGUMENT_TYPE_INTEGER;
				break;
			}
			case ARGUMENT_TYPE_NUMBER: {
				menu_index = MENU_ARGUMENT_TYPE_NUMBER;
				break;
			}
			case ARGUMENT_TYPE_VECTOR: {
				menu_index = MENU_ARGUMENT_TYPE_VECTOR;
				break;
			}
			case ARGUMENT_TYPE_MATRIX: {
				menu_index = MENU_ARGUMENT_TYPE_MATRIX;
				break;
			}
			case ARGUMENT_TYPE_EXPRESSION_ITEM: {
				menu_index = MENU_ARGUMENT_TYPE_EXPRESSION_ITEM;
				break;
			}
			case ARGUMENT_TYPE_FUNCTION: {
				menu_index = MENU_ARGUMENT_TYPE_FUNCTION;
				break;
			}
			case ARGUMENT_TYPE_UNIT: {
				menu_index = MENU_ARGUMENT_TYPE_UNIT;
				break;
			}
			case ARGUMENT_TYPE_VARIABLE: {
				menu_index = MENU_ARGUMENT_TYPE_VARIABLE;
				break;
			}
			case ARGUMENT_TYPE_FILE: {
				menu_index = MENU_ARGUMENT_TYPE_FILE;
				break;
			}
			case ARGUMENT_TYPE_BOOLEAN: {
				menu_index = MENU_ARGUMENT_TYPE_BOOLEAN;
				break;
			}
			case ARGUMENT_TYPE_ANGLE: {
				menu_index = MENU_ARGUMENT_TYPE_ANGLE;
				break;
			}
			case ARGUMENT_TYPE_DATA_OBJECT: {
				menu_index = MENU_ARGUMENT_TYPE_DATA_OBJECT;
				break;
			}
			case ARGUMENT_TYPE_DATA_PROPERTY: {
				menu_index = MENU_ARGUMENT_TYPE_DATA_PROPERTY;
				break;
			}
		}
	}
	g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(argumentrules_builder, "argument_rules_combobox_type"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_argument_rules_combobox_type_changed, NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(argumentrules_builder, "argument_rules_combobox_type")), menu_index);
	g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(argumentrules_builder, "argument_rules_combobox_type"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_argument_rules_combobox_type_changed, NULL);
	switch(arg ? arg->type() : ARGUMENT_TYPE_FREE) {
		case ARGUMENT_TYPE_NUMBER: {
			NumberArgument *farg = (NumberArgument*) arg;
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), farg->min() != NULL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), farg->includeEqualsMin());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), farg->min() != NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), TRUE);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 8);
			gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MIN);
			gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MAX);
			if(farg->min()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), farg->min()->floatValue());
			} else {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
			}
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), farg->max() != NULL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), farg->includeEqualsMax());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), farg->max() != NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), TRUE);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 8);
			gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MIN);
			gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MAX);
			if(farg->max()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), farg->max()->floatValue());
			} else {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
			}
			break;
		}
		case ARGUMENT_TYPE_INTEGER: {
			IntegerArgument *iarg = (IntegerArgument*) arg;
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), iarg->min() != NULL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), iarg->min() != NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), FALSE);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
			gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MIN);
			gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_min")), INT_MAX);
			if(iarg->min()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), iarg->min()->intValue());
			} else {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
			}
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), TRUE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), iarg->max() != NULL);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), iarg->max() != NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), FALSE);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
			gtk_adjustment_set_lower(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MIN);
			gtk_adjustment_set_upper(GTK_ADJUSTMENT(gtk_builder_get_object(argumentrules_builder, "adjustment_max")), INT_MAX);
			if(iarg->max()) {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), iarg->max()->intValue());
			} else {
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
			}
			break;
		}
		default: {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), FALSE);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), 0);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), 0);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max")), FALSE);
		}
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_button_ok")), FALSE);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_name")));
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		int menu_index = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(argumentrules_builder, "argument_rules_combobox_type")));
		switch(menu_index) {
			case MENU_ARGUMENT_TYPE_TEXT: {arg = new TextArgument(); break;}
			case MENU_ARGUMENT_TYPE_SYMBOLIC: {arg = new SymbolicArgument(); break;}
			case MENU_ARGUMENT_TYPE_DATE: {arg = new DateArgument(); break;}
			case MENU_ARGUMENT_TYPE_NONNEGATIVE_INTEGER: {}
			case MENU_ARGUMENT_TYPE_POSITIVE_INTEGER: {}
			case MENU_ARGUMENT_TYPE_NONZERO_INTEGER: {}
			case MENU_ARGUMENT_TYPE_INTEGER: {arg = new IntegerArgument(); break;}
			case MENU_ARGUMENT_TYPE_NONNEGATIVE: {}
			case MENU_ARGUMENT_TYPE_POSITIVE: {}
			case MENU_ARGUMENT_TYPE_NONZERO: {}
			case MENU_ARGUMENT_TYPE_NUMBER: {arg = new NumberArgument(); break;}
			case MENU_ARGUMENT_TYPE_VECTOR: {arg = new VectorArgument(); break;}
			case MENU_ARGUMENT_TYPE_MATRIX: {arg = new MatrixArgument(); break;}
			case MENU_ARGUMENT_TYPE_EXPRESSION_ITEM: {arg = new ExpressionItemArgument(); break;}
			case MENU_ARGUMENT_TYPE_FUNCTION: {arg = new FunctionArgument(); break;}
			case MENU_ARGUMENT_TYPE_UNIT: {arg = new UnitArgument(); break;}
			case MENU_ARGUMENT_TYPE_VARIABLE: {arg = new VariableArgument(); break;}
			case MENU_ARGUMENT_TYPE_FILE: {arg = new FileArgument(); break;}
			case MENU_ARGUMENT_TYPE_BOOLEAN: {arg = new BooleanArgument(); break;}
			case MENU_ARGUMENT_TYPE_ANGLE: {arg = new AngleArgument(); break;}
			case MENU_ARGUMENT_TYPE_DATA_OBJECT: {arg = new DataObjectArgument(NULL, ""); break;}
			case MENU_ARGUMENT_TYPE_DATA_PROPERTY: {arg = new DataPropertyArgument(NULL, ""); break;}
			default: {arg = new Argument();}
		}
		arg->setName(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_name"))));
		arg->setTests(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_test"))));
		arg->setAlerts(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_test"))));
		arg->setMatrixAllowed(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_allow_matrix"))));
		arg->setZeroForbidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_forbid_zero"))));
		arg->setHandleVector(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_handle_vector"))));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_condition")))) {
			arg->setCustomCondition(unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(argumentrules_builder, "argument_rules_entry_condition")))));
		} else {
			arg->setCustomCondition("");
		}
		if(arg->type() == ARGUMENT_TYPE_NUMBER) {
			NumberArgument *farg = (NumberArgument*) arg;
			farg->setIncludeEqualsMin(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_min_include_equals"))));
			farg->setIncludeEqualsMax(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_max_include_equals"))));
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")))) {
				Number nr;
				nr.setFloat(gtk_spin_button_get_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min"))));
				farg->setMin(&nr);
			} else {
				farg->setMin(NULL);
			}
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")))) {
				Number nr;
				nr.setFloat(gtk_spin_button_get_value(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max"))));
				farg->setMax(&nr);
			} else {
				farg->setMax(NULL);
			}
		} else if(arg->type() == ARGUMENT_TYPE_INTEGER) {
			IntegerArgument *iarg = (IntegerArgument*) arg;
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_min")))) {
				Number integ(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_min"))), 1);
				iarg->setMin(&integ);
			} else {
				iarg->setMin(NULL);
			}
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_checkbutton_enable_max")))) {
				Number integ(gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(argumentrules_builder, "argument_rules_spinbutton_max"))), 1);
				iarg->setMax(&integ);
			} else {
				iarg->setMax(NULL);
			}
		}
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionArguments));
		if(gtk_tree_selection_get_selected(select, &model, &iter)) {
			gtk_list_store_set(tFunctionArguments_store, &iter, 0, arg->name().c_str(), 1, arg->printlong().c_str(), 2, (gpointer) arg, -1);
		}
		gtk_widget_hide(dialog);
		return arg;
	}
	gtk_widget_hide(dialog);
	return NULL;
}

void fix_expression(string &str) {
	if(str.empty()) return;
	size_t i = 0;
	bool b = false;
	while(true) {
		i = str.find("\\", i);
		if(i == string::npos || i == str.length() - 1) break;
		if((str[i + 1] >= 'a' && str[i + 1] <= 'z') || (str[i + 1] >= 'A' && str[i + 1] <= 'Z') || (str[i + 1] >= '1' && str[i + 1] <= '9')) {
			b = true;
			break;
		}
		i++;
	}
	if(!b) {
		gsub("x", "\\x", str);
		gsub("y", "\\y", str);
		gsub("z", "\\z", str);
	}
	CALCULATOR->parseSigns(str);
}

/*
	display edit/new function dialog
	creates new function if f == NULL, win is parent window
*/
void edit_function(const char *category, MathFunction *f, GtkWindow *win, const char *name, const char *expression, bool enable_ok) {

	if(f && f->subtype() == SUBTYPE_DATA_SET) {
		edit_dataset((DataSet*) f, win);
		return;
	}

	GtkWidget *dialog = get_function_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), win);

	edited_function = f;
	reset_names_status();

	if(f) {
		if(f->isLocal())
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Function"));
		else
			gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Function (global)"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Function"));
	}

	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_description")));
	GtkTextBuffer *expression_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functionedit_builder, "function_edit_textview_expression")));

	//clear entries
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_condition")), "");
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_condition")), !f || !f->isBuiltin());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")), !f || !f->isBuiltin());
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_textview_expression")), !f || !f->isBuiltin());
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(functionedit_builder, "function_edit_combo_category")))), category ? category : "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_desc")), "");
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_example")), "");
	gtk_text_buffer_set_text(description_buffer, "", -1);
	gtk_text_buffer_set_text(expression_buffer, "", -1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_hidden")), false);

	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_add_argument")), !f || !f->isBuiltin());

	gtk_list_store_clear(tSubfunctions_store);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_modify_subfunction")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_remove_subfunction")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_add_subfunction")), !f || !f->isBuiltin());

	last_subfunction_index = 0;
	if(f) {
		//fill in original paramaters
		set_name_label_and_entry(f, GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
		if(!f->isBuiltin()) {
			gtk_text_buffer_set_text(expression_buffer, localize_expression(((UserFunction*) f)->formula()).c_str(), -1);
		}
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(functionedit_builder, "function_edit_combo_category")))), f->category().c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_desc")), f->title(false).c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_condition")), localize_expression(f->condition()).c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_example")), localize_expression(f->example()).c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_hidden")), f->isHidden());
		gtk_text_buffer_set_text(description_buffer, f->description().c_str(), -1);

		if(!f->isBuiltin()) {
			GtkTreeIter iter;
			string str;
			for(size_t i = 1; i <= ((UserFunction*) f)->countSubfunctions(); i++) {
				gtk_list_store_append(tSubfunctions_store, &iter);
				str = "\\";
				str += i2s(i);
				gtk_list_store_set(tSubfunctions_store, &iter, 0, str.c_str(), 1, localize_expression(((UserFunction*) f)->getSubfunction(i)).c_str(), 3, i, 2, ((UserFunction*) f)->subfunctionPrecalculated(i), -1);
				last_subfunction_index = i;
			}
		}
	}
	if(name) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")), name);
	if(expression) gtk_text_buffer_set_text(expression_buffer, expression, -1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_button_ok")), enable_ok && (name || expression));
	update_function_arguments_list(f);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(functionedit_builder, "function_edit_tabs")), 0);

run_function_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
		remove_blank_ends(str);
		if(str.empty() && (!names_status() || !has_name())) {
			//no name -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(functionedit_builder, "function_edit_tabs")), 0);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
			show_message(_("Empty name field."), GTK_WINDOW(dialog));
			goto run_function_edit_dialog;
		}
		GtkTextIter e_iter_s, e_iter_e;
		gtk_text_buffer_get_start_iter(expression_buffer, &e_iter_s);
		gtk_text_buffer_get_end_iter(expression_buffer, &e_iter_e);
		gchar *gstr = gtk_text_buffer_get_text(expression_buffer, &e_iter_s, &e_iter_e, FALSE);
		string str2 = unlocalize_expression(gstr);
		g_free(gstr);
		remove_blank_ends(str2);
		fix_expression(str2);
		if(!(f && f->isBuiltin()) && str2.empty()) {
			//no expression/relation -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(functionedit_builder, "function_edit_tabs")), 0);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_textview_expression")));
			show_message(_("Empty expression field."), GTK_WINDOW(dialog));
			goto run_function_edit_dialog;
		}
		GtkTextIter d_iter_s, d_iter_e;
		gtk_text_buffer_get_start_iter(description_buffer, &d_iter_s);
		gtk_text_buffer_get_end_iter(description_buffer, &d_iter_e);
		gchar *gstr_descr = gtk_text_buffer_get_text(description_buffer, &d_iter_s, &d_iter_e, FALSE);
		//function with the same name exists -- overwrite or open the dialog again
		if((!f || !f->hasName(str)) && ((names_status() != 1 && !str.empty()) || !has_name()) && CALCULATOR->functionNameTaken(str, f)) {
			MathFunction *func = CALCULATOR->getActiveFunction(str, true);
			if((!f || f != func) && (!func || func->category() != CALCULATOR->temporaryCategory()) && !ask_question(_("A function with the same name already exists.\nDo you want to overwrite the function?"), GTK_WINDOW(dialog))) {
				gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(functionedit_builder, "function_edit_tabs")), 0);
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functionedit_builder, "function_edit_entry_name")));
				goto run_function_edit_dialog;
			}
		}
		bool add_func = false;
		if(f) {
			f->setLocal(true);
			//edited an existing function
			f->setCategory(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(functionedit_builder, "function_edit_combo_category"))));
			f->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_desc"))));
			f->setDescription(gstr_descr);
			if(!f->isBuiltin()) {
				f->clearArgumentDefinitions();
			}
		} else {
			//new function
			f = new UserFunction(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT((gtk_builder_get_object(functionedit_builder, "function_edit_combo_category")))), "", "", true, -1, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_desc"))), gstr_descr);
			add_func = true;
		}
		g_free(gstr_descr);
		if(f) {
			string scond = unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_condition"))));
			fix_expression(scond);
			f->setCondition(scond);
			f->setExample(unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(functionedit_builder, "function_edit_entry_example")))));
			GtkTreeIter iter;
			bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionArguments_store), &iter);
			int i = 1;
			Argument *arg;
			while(b) {
				gtk_tree_model_get(GTK_TREE_MODEL(tFunctionArguments_store), &iter, 2, &arg, -1);
				if(arg && f->isBuiltin() && f->getArgumentDefinition(i)) {
					f->getArgumentDefinition(i)->setName(arg->name());
					delete arg;
				} else if(arg) {
					f->setArgumentDefinition(i, arg);
				}
				b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tFunctionArguments_store), &iter);
				i++;
			}
			if(!f->isBuiltin()) {
				((UserFunction*) f)->clearSubfunctions();
				b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tSubfunctions_store), &iter);
				while(b) {
					gchar *gstr;
					gboolean g_b = FALSE;
					gtk_tree_model_get(GTK_TREE_MODEL(tSubfunctions_store), &iter, 1, &gstr, 2, &g_b, -1);
					string ssub = unlocalize_expression(gstr);
					fix_expression(ssub);
					((UserFunction*) f)->addSubfunction(ssub, g_b);
					b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tSubfunctions_store), &iter);
					g_free(gstr);
				}
				((UserFunction*) f)->setFormula(str2);
			}
			f->setHidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(functionedit_builder, "function_edit_checkbutton_hidden"))));
			set_edited_names(f, str);
			if(add_func) {
				CALCULATOR->addFunction(f);
			}
			function_edited(f);
		}
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-functions.html#qalculate-function-creation", GTK_WINDOW(gtk_builder_get_object(functionedit_builder, "function_edit_dialog")));
		goto run_function_edit_dialog;
	} else {
		GtkTreeIter iter;
		bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionArguments_store), &iter);
		Argument *arg;
		while(b) {
			gtk_tree_model_get(GTK_TREE_MODEL(tFunctionArguments_store), &iter, 2, &arg, -1);
			if(arg) delete arg;
			b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tFunctionArguments_store), &iter);
		}
	}
	edited_function = NULL;
	gtk_widget_hide(dialog);
}
