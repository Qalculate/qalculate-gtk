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
#include "expressionedit.h"
#include "stackview.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

extern GtkBuilder *main_builder;

GtkWidget *stackview;
GtkListStore *stackstore;
GtkCellRenderer *register_renderer, *register_index_renderer;
GtkTreeViewColumn *register_column;
GtkCssProvider *box_rpnl_provider;

MathStructure lastx;

void on_stackstore_row_inserted(GtkTreeModel*, GtkTreePath *path, GtkTreeIter*, gpointer);
void on_stackstore_row_deleted(GtkTreeModel*, GtkTreePath *path, gpointer);

bool b_editing_stack = false;

void on_button_rpn_add_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_ADD);
}
void on_button_rpn_sub_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_SUBTRACT);
}
void on_button_rpn_times_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_MULTIPLY);
}
void on_button_rpn_divide_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_DIVIDE);
}
void on_button_rpn_xy_clicked(GtkButton*, gpointer) {
	calculateRPN(OPERATION_RAISE);
}
void on_button_rpn_sqrt_clicked(GtkButton*, gpointer) {
	insert_button_function(CALCULATOR->f_sqrt);
}
void on_button_rpn_reciprocal_clicked(GtkButton*, gpointer) {
	insert_button_function(CALCULATOR->getActiveFunction("inv"));
}
void on_button_rpn_negate_clicked(GtkButton*, gpointer) {
	insert_button_function(CALCULATOR->getActiveFunction("neg"));
}
void on_button_rpn_sum_clicked(GtkButton*, gpointer) {
	insert_button_function(CALCULATOR->f_total);
}
void on_button_registerup_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter, iter2;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(!gtk_tree_model_get_iter_first(model, &iter)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	if(index == 0) {
		CALCULATOR->moveRPNRegister(1, CALCULATOR->RPNStackSize());
		gtk_tree_model_iter_nth_child(model, &iter2, NULL, CALCULATOR->RPNStackSize() - 1);
		gtk_list_store_move_after(stackstore, &iter, &iter2);
	} else {
		CALCULATOR->moveRPNRegisterUp(index + 1);
		gtk_tree_model_iter_nth_child(model, &iter2, NULL, index - 1);
		gtk_list_store_swap(stackstore, &iter, &iter2);
	}
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	if(index <= 1) {
		replace_current_result(CALCULATOR->getRPNRegister(1));
		setResult(NULL, true, false, false, "", 0, true);
	}
	updateRPNIndexes();
}
void on_button_registerdown_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter, iter2;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(CALCULATOR->RPNStackSize() == 0) return;
		if(!gtk_tree_model_iter_nth_child(model, &iter, NULL, CALCULATOR->RPNStackSize() - 1)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	if(index + 1 == (int) CALCULATOR->RPNStackSize()) {
		CALCULATOR->moveRPNRegister(CALCULATOR->RPNStackSize(), 1);
		gtk_tree_model_get_iter_first(model, &iter2);
		gtk_list_store_move_before(stackstore, &iter, &iter2);
	} else {
		CALCULATOR->moveRPNRegisterDown(index + 1);
		gtk_tree_model_iter_nth_child(model, &iter2, NULL, index + 1);
		gtk_list_store_swap(stackstore, &iter, &iter2);
	}
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	if(index == 0 || index == (int) CALCULATOR->RPNStackSize() - 1) {
		replace_current_result(CALCULATOR->getRPNRegister(1));
		setResult(NULL, true, false, false, "", 0, true);
	}
	updateRPNIndexes();
}
void on_button_registerswap_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter, iter2;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(!gtk_tree_model_get_iter_first(model, &iter)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	if(index == 0) {
		if(!gtk_tree_model_iter_nth_child(model, &iter2, NULL, 1)) return;
		CALCULATOR->moveRPNRegister(1, 2);
		gtk_list_store_swap(stackstore, &iter, &iter2);
	} else {
		CALCULATOR->moveRPNRegister(index + 1, 1);
		gtk_tree_model_get_iter_first(model, &iter2);
		gtk_list_store_move_before(stackstore, &iter, &iter2);
	}
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	replace_current_result(CALCULATOR->getRPNRegister(1));
	setResult(NULL, true, false, false, "", 0, true);
	updateRPNIndexes();
}
void on_button_lastx_clicked(GtkButton*, gpointer) {
	if(expression_modified()) {
		if(get_expression_text().find_first_not_of(SPACES) != string::npos) {
			execute_expression(true);
		}
	}
	CALCULATOR->RPNStackEnter(new MathStructure(lastx));
	RPNRegisterAdded("", 0);
	replace_current_result(CALCULATOR->getRPNRegister(1));
	setResult(NULL, true, true, false, "", 0, false);
}
void on_button_copyregister_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *text_copy = NULL;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(!gtk_tree_model_get_iter_first(model, &iter)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	CALCULATOR->RPNStackEnter(new MathStructure(*CALCULATOR->getRPNRegister(index + 1)));
	gtk_tree_model_get(model, &iter, 1, &text_copy, -1);
	RPNRegisterAdded(text_copy, 0);
	g_free(text_copy);
	replace_current_result(CALCULATOR->getRPNRegister(1));
	setResult(NULL, true, false, false, "", 0, true);
}
void on_button_editregister_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_set_cursor_on_cell(GTK_TREE_VIEW(stackview), path, register_column, register_renderer, TRUE);
		gtk_tree_path_free(path);
	}
}
void on_button_deleteregister_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) {
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(stackview));
		if(!gtk_tree_model_get_iter_first(model, &iter)) return;
	}
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	CALCULATOR->deleteRPNRegister(index + 1);
	RPNRegisterRemoved(index);
	if(CALCULATOR->RPNStackSize() == 0) {
		clearresult();
		current_result()->clear();
	} else if(index == 0) {
		replace_current_result(CALCULATOR->getRPNRegister(1));
		setResult(NULL, true, false, false, "", 0, true);
	}
}
void on_button_clearstack_clicked(GtkButton*, gpointer) {
	CALCULATOR->clearRPNStack();
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	gtk_list_store_clear(stackstore);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	clearresult();
	current_result()->clear();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_clearstack")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_copyregister")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_deleteregister")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerdown")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerup")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerswap")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sqrt")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_reciprocal")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_negate")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sum")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_add")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sub")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_times")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_divide")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_xy")), FALSE);
}
void on_stackview_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_editregister")), TRUE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_editregister")), FALSE);
	}
}
void on_stackview_item_edited(GtkCellRendererText*, gchar *path, gchar *new_text, gpointer) {
	int index = s2i(path);
	GtkTreeIter iter;
	gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index);
	gtk_list_store_set(stackstore, &iter, 1, new_text, -1);
	execute_expression(true, false, OPERATION_ADD, NULL, true, index);
	b_editing_stack = false;
}
void on_stackview_item_editing_started(GtkCellRenderer*, GtkCellEditable*, gchar*, gpointer) {
	b_editing_stack = true;
}
void on_stackview_item_editing_canceled(GtkCellRenderer*, gpointer) {
	b_editing_stack = false;
}
int inserted_stack_index = -1;
void on_stackstore_row_inserted(GtkTreeModel*, GtkTreePath *path, GtkTreeIter*, gpointer) {
	inserted_stack_index = gtk_tree_path_get_indices(path)[0];
}
void on_stackstore_row_deleted(GtkTreeModel*, GtkTreePath *path, gpointer) {
	if(inserted_stack_index >= 0) {
		CALCULATOR->moveRPNRegister(gtk_tree_path_get_indices(path)[0] + 1, inserted_stack_index + 1);
		inserted_stack_index = -1;
		updateRPNIndexes();
	}
}

void selected_register_function(MathFunction *f) {
	if(!f) return;
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) return;
	GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
	gint index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	execute_expression(true, true, OPERATION_ADD, f, true, index);
}
void on_popup_menu_item_stack_copytext_activate(GtkMenuItem*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) return;
	gchar *gstr;
	gtk_tree_model_get(model, &iter, 1, &gstr, -1);
	set_clipboard(gstr, -1, false, true);
	g_free(gstr);
}
void on_popup_menu_item_stack_inserttext_activate(GtkMenuItem*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) return;
	gchar *gstr;
	gtk_tree_model_get(model, &iter, 1, &gstr, -1);
	insert_text(gstr);
	g_free(gstr);
}
void on_popup_menu_item_stack_negate_activate(GtkMenuItem*, gpointer) {
	selected_register_function(CALCULATOR->getActiveFunction("neg"));
}
void on_popup_menu_item_stack_invert_activate(GtkMenuItem*, gpointer) {
	selected_register_function(CALCULATOR->getActiveFunction("inv"));
}
void on_popup_menu_item_stack_square_activate(GtkMenuItem*, gpointer) {
	selected_register_function(CALCULATOR->f_sq);
}
void on_popup_menu_item_stack_sqrt_activate(GtkMenuItem*, gpointer) {
	selected_register_function(CALCULATOR->f_sqrt);
}
void on_popup_menu_item_stack_copy_activate(GtkMenuItem*, gpointer) {
	on_button_copyregister_clicked(NULL, NULL);
}
void on_popup_menu_item_stack_movetotop_activate(GtkMenuItem*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter, iter2;
	GtkTreePath *path;
	gint index;
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter)) return;
	path = gtk_tree_model_get_path(model, &iter);
	index = gtk_tree_path_get_indices(path)[0];
	gtk_tree_path_free(path);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	CALCULATOR->moveRPNRegister(index + 1, 1);
	gtk_tree_model_get_iter_first(model, &iter2);
	gtk_list_store_move_before(stackstore, &iter, &iter2);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	replace_current_result(CALCULATOR->getRPNRegister(1));
	setResult(NULL, true, false, false, "", 0, true);
	updateRPNIndexes();
}
void on_popup_menu_item_stack_up_activate(GtkMenuItem*, gpointer) {
	on_button_registerup_clicked(NULL, NULL);
}
void on_popup_menu_item_stack_down_activate(GtkMenuItem*, gpointer) {
	on_button_registerdown_clicked(NULL, NULL);
}
void on_popup_menu_item_stack_swap_activate(GtkMenuItem*, gpointer) {
	on_button_registerswap_clicked(NULL, NULL);
}
void on_popup_menu_item_stack_edit_activate(GtkMenuItem*, gpointer) {
	on_button_editregister_clicked(NULL, NULL);
}
void on_popup_menu_item_stack_delete_activate(GtkMenuItem*, gpointer) {
	on_button_deleteregister_clicked(NULL, NULL);
}
void on_popup_menu_item_stack_clear_activate(GtkMenuItem*, gpointer) {
	on_button_clearstack_clicked(NULL, NULL);
}
void update_stackview_popup() {
	GtkTreeModel *model;
	GtkTreeIter iter;
	bool b_sel = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &model, &iter);
	gint index = -1;
	if(b_sel) {
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		index = gtk_tree_path_get_indices(path)[0];
		gtk_tree_path_free(path);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_inserttext")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_copytext")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_copy")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_movetotop")), b_sel && index != 0);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_moveup")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_movedown")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_swap")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_edit")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_negate")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_invert")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_square")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_sqrt")), b_sel);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_item_stack_delete")), b_sel);
}
gboolean on_stackview_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	GtkTreePath *path = NULL;
	GtkTreeSelection *select = NULL;
	if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS) {
		if(b_busy) return TRUE;
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(stackview), event->x, event->y, &path, NULL, NULL, NULL)) {
			select = gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview));
			if(!gtk_tree_selection_path_is_selected(select, path)) {
				gtk_tree_selection_unselect_all(select);
				gtk_tree_selection_select_path(select, path);
			}
			gtk_tree_path_free(path);
		}
		update_stackview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_stackview")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_stackview")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_stackview_popup_menu(GtkWidget*, gpointer) {
	if(b_busy) return TRUE;
	update_stackview_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_stackview")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_stackview")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}

string get_register_text(int index) {
	GtkTreeIter iter;
	if(!gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index - 1)) return "";
	gchar *gstr;
	gtk_tree_model_get(GTK_TREE_MODEL(stackstore), &iter, 1, &gstr, -1);
	string str = gstr;
	g_free(gstr);
	return str;
}

void stack_view_swap(int index) {
	if(index > 0) {
		GtkTreeIter iter;
		if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index - 1)) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
			on_button_registerswap_clicked(NULL, NULL);
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
		}
	} else {
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
		on_button_registerswap_clicked(NULL, NULL);
	}
}
void stack_view_copy(int index) {
	if(index > 0) {
		GtkTreeIter iter;
		if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index - 1)) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
			on_button_copyregister_clicked(NULL, NULL);
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
		}
	} else {
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
		on_button_copyregister_clicked(NULL, NULL);
	}
}
void stack_view_pop(int index) {
	if(index > 0) {
		GtkTreeIter iter;
		if(gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index - 1)) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), &iter);
			on_button_deleteregister_clicked(NULL, NULL);
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
		}
	} else {
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
		on_button_deleteregister_clicked(NULL, NULL);
	}
}
void stack_view_rotate(bool up) {
	gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)));
	if(up) on_button_registerup_clicked(NULL, NULL);
	else on_button_registerdown_clicked(NULL, NULL);
}
void stack_view_clear() {
	on_button_clearstack_clicked(NULL, NULL);
}
void stack_view_lastx() {
	on_button_lastx_clicked(NULL, NULL);
}
void RPNStackCleared() {
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	gtk_list_store_clear(stackstore);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
}
void updateRPNIndexes() {
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(stackstore), &iter)) return;
	for(int i = 1; ; i++) {
		gtk_list_store_set(stackstore, &iter, 0, i2s(i).c_str(), -1);
		if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(stackstore), &iter)) break;
	}
}
void RPNRegisterAdded(string text, gint index) {
	GtkTreeIter iter;
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	gtk_list_store_insert(stackstore, &iter, index);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_inserted, NULL);
	gtk_list_store_set(stackstore, &iter, 0, i2s(index + 1).c_str(), 1, text.c_str(), -1);
	updateRPNIndexes();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_clearstack")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_copyregister")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_deleteregister")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sqrt")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_reciprocal")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_negate")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_add")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sub")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_times")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_divide")), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_xy")), TRUE);
	if(CALCULATOR->RPNStackSize() >= 2) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerdown")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerup")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerswap")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sum")), TRUE);
	}
	on_stackview_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), NULL);
}
void RPNRegisterRemoved(gint index) {
	GtkTreeIter iter;
	gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index);
	g_signal_handlers_block_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	gtk_list_store_remove(stackstore, &iter);
	g_signal_handlers_unblock_matched((gpointer) stackstore, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_stackstore_row_deleted, NULL);
	updateRPNIndexes();
	if(CALCULATOR->RPNStackSize() == 0) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_clearstack")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_copyregister")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_deleteregister")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sqrt")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_reciprocal")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_negate")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_add")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sub")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_times")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_divide")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_xy")), FALSE);
	}
	if(CALCULATOR->RPNStackSize() < 2) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerdown")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerup")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerswap")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sum")), FALSE);
	}
	on_stackview_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview)), NULL);
}
void RPNRegisterChanged(string text, gint index) {
	GtkTreeIter iter;
	gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(stackstore), &iter, NULL, index);
	gtk_list_store_set(stackstore, &iter, 1, text.c_str(), -1);
}
void update_stack_font(bool initial) {
	if(use_custom_history_font) {
		g_object_set(G_OBJECT(register_renderer), "font", custom_history_font.c_str(), NULL);
		g_object_set(G_OBJECT(register_index_renderer), "font", custom_history_font.c_str(), NULL);
	} else if(!initial) {
		g_object_set(G_OBJECT(register_renderer), "font", "", NULL);
		g_object_set(G_OBJECT(register_index_renderer), "font", "", NULL);
	}
	if(!initial) updateRPNIndexes();
}
void update_stack_button_font() {
	if(use_custom_keypad_font) {
		gchar *gstr = font_name_to_css(custom_keypad_font.c_str());
		gtk_css_provider_load_from_data(box_rpnl_provider, gstr, -1, NULL);
		g_free(gstr);
	} else {
		gtk_css_provider_load_from_data(box_rpnl_provider, "", -1, NULL);
	}
}
bool editing_stack() {
	return b_editing_stack;
}
void update_lastx() {
	if(current_result()) {
		lastx = *current_result();
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_lastx")), TRUE);
	}
}

void create_stack_view() {

	stackview = GTK_WIDGET(gtk_builder_get_object(main_builder, "stackview"));

	box_rpnl_provider = gtk_css_provider_new();
	gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rpnl"))), GTK_STYLE_PROVIDER(box_rpnl_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	if(use_custom_keypad_font) update_stack_button_font();
	if(use_custom_history_font) update_stack_font(true);

	GList *l;
	GList *list;
	CHILDREN_SET_FOCUS_ON_CLICK("box_rm")
	CHILDREN_SET_FOCUS_ON_CLICK("box_re")
	SET_FOCUS_ON_CLICK(gtk_builder_get_object(main_builder, "button_clearstack"));
	SET_FOCUS_ON_CLICK(gtk_builder_get_object(main_builder, "button_editregister"));
	CHILDREN_SET_FOCUS_ON_CLICK("box_ro1")
	CHILDREN_SET_FOCUS_ON_CLICK("box_ro2")

	SET_FOCUS_ON_CLICK(gtk_builder_get_object(main_builder, "button_rpn_sum"));
	stackstore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(stackview), GTK_TREE_MODEL(stackstore));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(stackview));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	register_index_renderer = gtk_cell_renderer_text_new();
	g_object_set (G_OBJECT(register_index_renderer), "xalign", 0.5, NULL);
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Index"), register_index_renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(stackview), column);
	register_renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(register_renderer), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, "xalign", 1.0, "mode", GTK_CELL_RENDERER_MODE_EDITABLE, NULL);
	g_signal_connect((gpointer) register_renderer, "edited", G_CALLBACK(on_stackview_item_edited), NULL);
	g_signal_connect((gpointer) register_renderer, "editing-started", G_CALLBACK(on_stackview_item_editing_started), NULL);
	g_signal_connect((gpointer) register_renderer, "editing-canceled", G_CALLBACK(on_stackview_item_editing_canceled), NULL);
	register_column = gtk_tree_view_column_new_with_attributes(_("Value"), register_renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(stackview), register_column);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_stackview_selection_changed), NULL);
	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(stackview), TRUE);
	g_signal_connect((gpointer) stackstore, "row-deleted", G_CALLBACK(on_stackstore_row_deleted), NULL);
	g_signal_connect((gpointer) stackstore, "row-inserted", G_CALLBACK(on_stackstore_row_inserted), NULL);

	if(themestr == "Breeze" || themestr == "Breeze-Dark") {

		GtkCssProvider *link_style_top = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_top, "* {border-bottom-left-radius: 0; border-bottom-right-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_bot = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_bot, "* {border-top-left-radius: 0; border-top-right-radius: 0;}", -1, NULL);
		GtkCssProvider *link_style_mid = gtk_css_provider_new(); gtk_css_provider_load_from_data(link_style_mid, "* {border-radius: 0;}", -1, NULL);

		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ro1"))), "linked");
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ro2"))), "linked");
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rm"))), "linked");
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_re"))), "linked");

		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_add"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sub"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_times"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_divide"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_xy"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_negate"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_reciprocal"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_rpn_sqrt"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerup"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerdown"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_registerswap"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_copyregister"))), GTK_STYLE_PROVIDER(link_style_top), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_lastx"))), GTK_STYLE_PROVIDER(link_style_mid), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "button_deleteregister"))), GTK_STYLE_PROVIDER(link_style_bot), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}

#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION <= 18
	if(RUNTIME_CHECK_GTK_VERSION_LESS(3, 18) && (themestr == "Ambiance" || themestr == "Radiance")) {
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_re"))), "linked");
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_rm"))), "linked");
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ro1"))), "linked");
		gtk_style_context_remove_class(gtk_widget_get_style_context(GTK_WIDGET(gtk_builder_get_object(main_builder, "box_ro2"))), "linked");
	}
#endif
	if(!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "document-edit-symbolic")) gtk_image_set_from_icon_name(GTK_IMAGE(gtk_builder_get_object(main_builder, "image_edit")), "gtk-edit", GTK_ICON_SIZE_BUTTON);

	lastx.setUndefined();

	gtk_builder_add_callback_symbols(main_builder, "on_button_rpn_add_clicked", G_CALLBACK(on_button_rpn_add_clicked), "on_button_rpn_sub_clicked", G_CALLBACK(on_button_rpn_sub_clicked), "on_button_rpn_times_clicked", G_CALLBACK(on_button_rpn_times_clicked), "on_button_rpn_divide_clicked", G_CALLBACK(on_button_rpn_divide_clicked), "on_button_rpn_xy_clicked", G_CALLBACK(on_button_rpn_xy_clicked), "on_button_rpn_negate_clicked", G_CALLBACK(on_button_rpn_negate_clicked), "on_button_rpn_reciprocal_clicked", G_CALLBACK(on_button_rpn_reciprocal_clicked), "on_button_rpn_sqrt_clicked", G_CALLBACK(on_button_rpn_sqrt_clicked), "on_button_rpn_sum_clicked", G_CALLBACK(on_button_rpn_sum_clicked), "on_stackview_button_press_event", G_CALLBACK(on_stackview_button_press_event), "on_stackview_popup_menu", G_CALLBACK(on_stackview_popup_menu), "on_button_registerup_clicked", G_CALLBACK(on_button_registerup_clicked), "on_button_registerdown_clicked", G_CALLBACK(on_button_registerdown_clicked), "on_button_registerswap_clicked", G_CALLBACK(on_button_registerswap_clicked), "on_button_copyregister_clicked", G_CALLBACK(on_button_copyregister_clicked), "on_button_lastx_clicked", G_CALLBACK(on_button_lastx_clicked), "on_button_deleteregister_clicked", G_CALLBACK(on_button_deleteregister_clicked), "on_button_editregister_clicked", G_CALLBACK(on_button_editregister_clicked), "on_button_clearstack_clicked", G_CALLBACK(on_button_clearstack_clicked), "on_popup_menu_item_stack_inserttext_activate", G_CALLBACK(on_popup_menu_item_stack_inserttext_activate), "on_popup_menu_item_stack_copytext_activate", G_CALLBACK(on_popup_menu_item_stack_copytext_activate), "on_popup_menu_item_stack_copy_activate", G_CALLBACK(on_popup_menu_item_stack_copy_activate), "on_popup_menu_item_stack_movetotop_activate", G_CALLBACK(on_popup_menu_item_stack_movetotop_activate), "on_popup_menu_item_stack_swap_activate", G_CALLBACK(on_popup_menu_item_stack_swap_activate), "on_popup_menu_item_stack_edit_activate", G_CALLBACK(on_popup_menu_item_stack_edit_activate), "on_popup_menu_item_stack_negate_activate", G_CALLBACK(on_popup_menu_item_stack_negate_activate), "on_popup_menu_item_stack_invert_activate", G_CALLBACK(on_popup_menu_item_stack_invert_activate), "on_popup_menu_item_stack_square_activate", G_CALLBACK(on_popup_menu_item_stack_square_activate), "on_popup_menu_item_stack_sqrt_activate", G_CALLBACK(on_popup_menu_item_stack_sqrt_activate), "on_popup_menu_item_stack_delete_activate", G_CALLBACK(on_popup_menu_item_stack_delete_activate), "on_popup_menu_item_stack_clear_activate", G_CALLBACK(on_popup_menu_item_stack_clear_activate), NULL);
}
