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
#include "nameseditdialog.h"
#include "unknowneditdialog.h"
#include "variableeditdialog.h"
#include "openhelp.h"
#include "matrixeditdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *matrixedit_builder = NULL;

GtkWidget *tMatrixEdit;
GtkListStore *tMatrixEdit_store;
vector<GtkTreeViewColumn*> matrix_edit_columns;

KnownVariable *edited_matrix = NULL;

GtkTreeIter matrix_edit_prev_iter;
gint matrix_edit_prev_column = 0;
bool block_matrix_edit_update_cursor = false;

void on_matrix_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_button_ok")), strlen(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")))) > 0);
}
void on_matrix_edit_button_names_clicked(GtkWidget*, gpointer) {
	if(!edit_names(edited_matrix, TYPE_VARIABLE, gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name"))), GTK_WINDOW(gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog")))) return;
	string str = first_name();
	if(!str.empty()) {
		g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variable_edit_entry_name_changed, NULL);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")), str.c_str());
		g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variable_edit_entry_name_changed, NULL);
	}
	on_matrix_changed();
}
gboolean on_tMatrixEdit_cursor_changed(GtkTreeView*, gpointer) {
	if(block_matrix_edit_update_cursor) return FALSE;
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrixEdit), &path, &column);
	bool b = false;
	if(path) {
		if(column) {
			if(gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrixEdit_store), &iter, path)) {
				gint i_column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), "column"));
				matrix_edit_prev_iter = iter;
				matrix_edit_prev_column = i_column;
				gchar *pos_str;
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")))) {
					pos_str = g_strdup_printf("(%i, %i)", gtk_tree_path_get_indices(path)[0] + 1, i_column + 1);
				} else {
					pos_str = g_strdup_printf("%i", (int) (i_column + 1 + matrix_edit_columns.size() * gtk_tree_path_get_indices(path)[0]));
				}
				gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_position")), pos_str);
				g_free(pos_str);
				b = true;
			}
		}
		gtk_tree_path_free(path);
	}
	if(!b) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_position")), _("none"));
	return FALSE;
}
void on_tMatrixEdit_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer model) {
	GtkTreeIter iter;
	gint i_column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));
	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL(model), &iter, path_string);
	gtk_list_store_set(GTK_LIST_STORE (model), &iter, i_column, new_text, -1);
	on_matrix_changed();
}
gboolean on_tMatrixEdit_editable_key_press_event(GtkWidget *w, GdkEventKey *event, gpointer) {
	guint keyval = 0;
	gdk_event_get_keyval((GdkEvent*) event, &keyval);
	switch(keyval) {
		case GDK_KEY_Up: {}
		case GDK_KEY_Down: {}
		case GDK_KEY_Tab: {}
		case GDK_KEY_ISO_Enter: {}
		case GDK_KEY_KP_Enter: {}
		case GDK_KEY_Return: {
			gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(w));
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrixEdit), &path, &column);
			if(path) {
				if(column) {
					for(size_t i = 0; i < matrix_edit_columns.size(); i++) {
						if(matrix_edit_columns[i] == column) {
							if(keyval == GDK_KEY_Tab) {
								i++;
								if(i >= matrix_edit_columns.size()) {
									gtk_tree_path_next(path);
									GtkTreeIter iter;
									if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrixEdit_store), &iter, path)) {
										gtk_tree_path_free(path);
										path = gtk_tree_path_new_first();
									}
									i = 0;
								}
							} else {
								if(keyval == GDK_KEY_Up) {
									if(!gtk_tree_path_prev(path)) {
										gtk_tree_path_free(path);
										path = gtk_tree_path_new_from_indices(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tMatrixEdit_store), NULL) - 1, -1);
									}
								} else {
									gtk_tree_path_next(path);
									GtkTreeIter iter;
									if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrixEdit_store), &iter, path)) {
										gtk_tree_path_free(path);
										if(keyval != GDK_KEY_Up) return TRUE;
										path = gtk_tree_path_new_first();
									}
								}
							}
							gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[i], FALSE, 0.0, 0.0);
							while(gtk_events_pending()) gtk_main_iteration();
							gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[i], TRUE);
							on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
							break;
						}
					}
				}
				gtk_tree_path_free(path);
			}
			return TRUE;
		}
	}
	return FALSE;
}
void on_tMatrixEdit_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar*, gpointer) {
	g_signal_connect(G_OBJECT(editable), "key-press-event", G_CALLBACK(on_tMatrixEdit_editable_key_press_event), renderer);
}
gboolean on_tMatrixEdit_key_press_event(GtkWidget*, GdkEventKey *event, gpointer) {
	guint keyval = 0;
	gdk_event_get_keyval((GdkEvent*) event, &keyval);
	switch(keyval) {
		case GDK_KEY_Return: {break;}
		case GDK_KEY_Tab: {
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrixEdit), &path, &column);
			if(path) {
				if(column) {
					for(size_t i = 0; i < matrix_edit_columns.size(); i++) {
						if(matrix_edit_columns[i] == column) {
							i++;
							if(i < matrix_edit_columns.size()) {
								gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[i], FALSE);
								while(gtk_events_pending()) gtk_main_iteration();
								gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[i], FALSE, 0.0, 0.0);
							} else {
								gtk_tree_path_next(path);
								GtkTreeIter iter;
								if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrixEdit_store), &iter, path)) {
									gtk_tree_path_free(path);
									path = gtk_tree_path_new_first();
								}
								gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[0], FALSE);
								while(gtk_events_pending()) gtk_main_iteration();
								gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[0], FALSE, 0.0, 0.0);
							}
							gtk_tree_path_free(path);
							on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
							return TRUE;
						}
					}
				}
				gtk_tree_path_free(path);
			}
			break;
		}
		default: {
			if(gdk_keyval_to_unicode(keyval) < 32) return FALSE;
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrixEdit), &path, &column);
			if(path) {
				if(column) {
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, column, TRUE);
					while(gtk_events_pending()) gtk_main_iteration();
					gboolean return_val = FALSE;
					g_signal_emit_by_name((gpointer) gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog"), "key_press_event", event, &return_val);
					gtk_tree_path_free(path);
					return TRUE;
				}
				gtk_tree_path_free(path);
			}
		}
	}
	return FALSE;
}
gboolean on_tMatrixEdit_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	guint button = 0;
	gdouble x = 0, y = 0;
	gdk_event_get_button((GdkEvent*) event, &button);
	gdk_event_get_coords((GdkEvent*) event, &x, &y);
	if(button != 1) return FALSE;
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tMatrixEdit), (gint) x, (gint) y, &path, &column, NULL, NULL) && path && column) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, column, TRUE);
		gtk_tree_path_free(path);
		return TRUE;
	}
	if(path) gtk_tree_path_free(path);
	return FALSE;
}
void on_matrix_edit_checkbutton_temporary_toggled(GtkToggleButton *w, gpointer) {
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category")))), gtk_toggle_button_get_active(w) ? CALCULATOR->temporaryCategory().c_str() : "");
}
void on_matrix_edit_spinbutton_columns_value_changed(GtkSpinButton *w, gpointer) {
	gint c = matrix_edit_columns.size();
	gint new_c = gtk_spin_button_get_value_as_int(w);
	if(new_c < c) {
		for(gint index_c = new_c; index_c < c; index_c++) {
			gtk_tree_view_remove_column(GTK_TREE_VIEW(tMatrixEdit), matrix_edit_columns[index_c]);
		}
		matrix_edit_columns.resize(new_c);
	} else {
		GtkTreeIter iter;
		for(gint index_c = c; index_c < new_c; index_c++) {
			GtkCellRenderer *matrix_edit_renderer = gtk_cell_renderer_text_new();
			g_object_set(G_OBJECT(matrix_edit_renderer), "editable", TRUE, NULL);
			g_object_set(G_OBJECT(matrix_edit_renderer), "xalign", 1.0, NULL);
			g_object_set_data(G_OBJECT(matrix_edit_renderer), "column", GINT_TO_POINTER(index_c));
			g_signal_connect(G_OBJECT(matrix_edit_renderer), "edited", G_CALLBACK(on_tMatrixEdit_edited), GTK_TREE_MODEL(tMatrixEdit_store));
			g_signal_connect(G_OBJECT(matrix_edit_renderer), "editing-started", G_CALLBACK(on_tMatrixEdit_editing_started), GTK_TREE_MODEL(tMatrixEdit_store));
			GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(i2s(index_c).c_str(), matrix_edit_renderer, "text", index_c, NULL);
			g_object_set_data(G_OBJECT(column), "column", GINT_TO_POINTER(index_c));
			g_object_set_data(G_OBJECT(column), "renderer", (gpointer) matrix_edit_renderer);
			gtk_tree_view_column_set_min_width(column, 50);
			gtk_tree_view_column_set_alignment(column, 0.5);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tMatrixEdit), column);
			gtk_tree_view_column_set_expand(column, TRUE);
			matrix_edit_columns.push_back(column);
		}
		if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrixEdit_store), &iter)) return;
		bool b_matrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")));
		while(true) {
			for(gint index_c = c; index_c < new_c; index_c++) {
				if(b_matrix) gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, "0", -1);
				else gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, "", -1);
			}
			if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrixEdit_store), &iter)) break;
		}
	}
}
void on_matrix_edit_spinbutton_rows_value_changed(GtkSpinButton *w, gpointer) {
	gint new_r = gtk_spin_button_get_value_as_int(w);
	gint r = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tMatrixEdit_store), NULL);
	gint c = matrix_edit_columns.size();
	bool b_matrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")));
	GtkTreeIter iter;
	if(r < new_r) {
		while(r < new_r) {
			gtk_list_store_append(GTK_LIST_STORE(tMatrixEdit_store), &iter);
			for(gint i = 0; i < c; i++) {
				if(b_matrix) gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, i, "0", -1);
				else gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, i, "", -1);
			}
			r++;
		}
	} else if(new_r < r) {
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(tMatrixEdit_store), &iter, NULL, new_r);
		while(gtk_list_store_iter_is_valid(GTK_LIST_STORE(tMatrixEdit_store), &iter)) {
			gtk_list_store_remove(GTK_LIST_STORE(tMatrixEdit_store), &iter);
		}
	}
}
void on_matrix_edit_radiobutton_matrix_toggled(GtkToggleButton*, gpointer) {
	on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
}
void on_matrix_edit_radiobutton_vector_toggled(GtkToggleButton*, gpointer) {
	on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
}

GtkWidget* get_matrix_edit_dialog(void) {
	if(!matrixedit_builder) {

		matrixedit_builder = getBuilder("matrixedit.ui");
		g_assert(matrixedit_builder != NULL);

		g_assert(gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog") != NULL);

		GType types[200];
		for(gint i = 0; i < 200; i += 1) {
			types[i] = G_TYPE_STRING;
		}
		tMatrixEdit_store = gtk_list_store_newv(200, types);
		tMatrixEdit = GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_view"));
		gtk_tree_view_set_model (GTK_TREE_VIEW(tMatrixEdit), GTK_TREE_MODEL(tMatrixEdit_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tMatrixEdit));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

		g_signal_connect((gpointer) gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(matrixedit_builder, "matrix_edit_textview_description"))), "changed", G_CALLBACK(on_matrix_changed), NULL);

		gtk_builder_add_callback_symbols(matrixedit_builder, "on_tMatrixEdit_button_press_event", G_CALLBACK(on_tMatrixEdit_button_press_event), "on_tMatrixEdit_cursor_changed", G_CALLBACK(on_tMatrixEdit_cursor_changed), "on_tMatrixEdit_key_press_event", G_CALLBACK(on_tMatrixEdit_key_press_event), "on_matrix_changed", G_CALLBACK(on_matrix_changed), "on_variable_edit_entry_name_changed", G_CALLBACK(on_variable_edit_entry_name_changed), "on_matrix_edit_button_names_clicked", G_CALLBACK(on_matrix_edit_button_names_clicked), "on_matrix_edit_spinbutton_rows_value_changed", G_CALLBACK(on_matrix_edit_spinbutton_rows_value_changed), "on_matrix_edit_spinbutton_columns_value_changed", G_CALLBACK(on_matrix_edit_spinbutton_columns_value_changed), "on_matrix_edit_radiobutton_matrix_toggled", G_CALLBACK(on_matrix_edit_radiobutton_matrix_toggled), "on_matrix_edit_radiobutton_vector_toggled", G_CALLBACK(on_matrix_edit_radiobutton_vector_toggled), "on_matrix_edit_checkbutton_temporary_toggled", G_CALLBACK(on_matrix_edit_checkbutton_temporary_toggled), NULL);
		gtk_builder_connect_signals(matrixedit_builder, NULL);

	}

	/* populate combo menu */

	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);
	GList *items = NULL;
	for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
		if(!CALCULATOR->variables[i]->category().empty()) {
			//add category if not present
			if(g_hash_table_lookup(hash, (gconstpointer) CALCULATOR->variables[i]->category().c_str()) == NULL) {
				items = g_list_insert_sorted(items, (gpointer) CALCULATOR->variables[i]->category().c_str(), (GCompareFunc) compare_categories);
				//remember added categories
				g_hash_table_insert(hash, (gpointer) CALCULATOR->variables[i]->category().c_str(), (gpointer) hash);
			}
		}
	}
	for(GList *l = items; l != NULL; l = l->next) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category")), (const gchar*) l->data);
	}
	g_hash_table_destroy(hash);
	g_list_free(items);

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog"));
}

/*
	display edit/new matrix dialog
	creates new matrix if v == NULL, mstruct_ is forced value, win is parent window
*/
void edit_matrix(const char *category, Variable *var, MathStructure *mstruct_, GtkWindow *win, gboolean create_vector) {

	if(var != NULL && !var->isKnown()) {
		edit_unknown(category, var, win);
		return;
	}

	KnownVariable *v = (KnownVariable*) var;

	if((v && !v->get().isVector()) || (mstruct_ && !mstruct_->isVector())) {
		edit_variable(category, v, mstruct_, win);
		return;
	}

	edited_matrix = v;
	reset_names_status();

	GtkWidget *dialog = get_matrix_edit_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), win);
	if(mstruct_) {
		create_vector = !mstruct_->isMatrix();
	} else if(v) {
		create_vector = !v->get().isMatrix();
	}
	if(create_vector) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")), TRUE);
	else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")), TRUE);

	if(create_vector) {
		if(v) {
			if(v->isLocal())
				gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Vector"));
			else
				gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Vector (global)"));
		} else {
			gtk_window_set_title(GTK_WINDOW(dialog), _("New Vector"));
		}
	} else {
		if(v) {
			if(v->isLocal())
				gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Matrix"));
			else
				gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Matrix (global)"));
		} else {
			gtk_window_set_title(GTK_WINDOW(dialog), _("New Matrix"));
		}
	}

	GtkTextBuffer *description_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(matrixedit_builder, "matrix_edit_textview_description")));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_checkbutton_hidden")), false);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(matrixedit_builder, "matrix_edit_tabs")), 0);
	gtk_text_buffer_set_text(description_buffer, "", -1);

	int r = 4, c = 4;
	const MathStructure *old_vctr = NULL;
	if(v) {
		if(create_vector) {
			old_vctr = &v->get();
		} else {
			c = v->get().columns();
			r = v->get().rows();
		}
		//fill in original parameters
		set_name_label_and_entry(v, GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));
		//can only change name and value of user variable
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), !v->isBuiltin());
		//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_table_elements")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")), !v->isBuiltin());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")), !v->isBuiltin());
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category")))), v->category().c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_checkbutton_hidden")), v->isHidden());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_checkbutton_temporary")), v->category() == CALCULATOR->temporaryCategory());
		gtk_text_buffer_set_text(description_buffer, v->description().c_str(), -1);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_desc")), v->title(false).c_str());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_button_ok")), FALSE);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), TRUE);
		//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_table_elements")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")), TRUE);

		//fill in default values
		string v_name;
		int i = 1;
		do {
			v_name = "v"; v_name += i2s(i);
			i++;
		} while(CALCULATOR->nameTaken(v_name));
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")), v_name.c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_checkbutton_temporary")), !category || CALCULATOR->temporaryCategory() == category);
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category")))), !category ? CALCULATOR->temporaryCategory().c_str() : category);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_desc")), "");
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_button_ok")), TRUE);
	}
	if(mstruct_) {
		//forced value
		if(create_vector) {
			old_vctr = mstruct_;
		} else {
			c = mstruct_->columns();
			r = mstruct_->rows();
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), FALSE);
		//gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_table_elements")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_matrix")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")), FALSE);

		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_button_ok")), TRUE);
	}

	if(create_vector) {
		if(old_vctr) {
			r = old_vctr->countChildren();
			c = (int) ::sqrt(::sqrt((double) r)) + 8;
			if(c < 10) c = 10;
			if(r % c > 0) {
				r = r / c + 1;
			} else {
				r = r / c;
			}
			if(r < 100) r = 100;
		} else {
			c = 10;
			r = 100;
		}
	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), r);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), c);
	on_matrix_edit_spinbutton_columns_value_changed(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")), NULL);
	on_matrix_edit_spinbutton_rows_value_changed(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")), NULL);

	CALCULATOR->startControl(2000);
	PrintOptions po;
	po.number_fraction_format = FRACTION_DECIMAL_EXACT;
	po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
	while(gtk_events_pending()) gtk_main_iteration();
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
	for(size_t index_r = 0; b && index_r < (size_t) r; index_r++) {
		for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
			if(create_vector) {
				if(old_vctr && index_r * c + index_c < old_vctr->countChildren()) {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, old_vctr->getChild(index_r * c + index_c + 1)->print(po).c_str(), -1);
				} else {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, "", -1);
				}
			} else {
				if(v) {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, v->get().getElement(index_r + 1, index_c + 1)->print(po).c_str(), -1);
				} else if(mstruct_) {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, mstruct_->getElement(index_r + 1, index_c + 1)->print(po).c_str(), -1);
				} else {
					gtk_list_store_set(GTK_LIST_STORE(tMatrixEdit_store), &iter, index_c, "0", -1);
				}
			}
		}
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
	}
	CALCULATOR->stopControl();
	if(r > 0 && c > 0) {
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[0], TRUE);
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrixEdit), path, matrix_edit_columns[0], FALSE, 0.0, 0.0);
		on_tMatrixEdit_cursor_changed(GTK_TREE_VIEW(tMatrixEdit), NULL);
		gtk_tree_path_free(path);
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrixedit_builder, "matrix_edit_label_position")), "");
	}
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));

run_matrix_edit_dialog:
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));
		remove_blank_ends(str);
		if(str.empty() && (!names_status() || !has_name())) {
			//no name -- open dialog again
			gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(matrixedit_builder, "matrix_edit_tabs")), 0);
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));
			show_message(_("Empty name field."), GTK_WINDOW(dialog));
			goto run_matrix_edit_dialog;
		}

		//variable with the same name exists -- overwrite or open dialog again
		if((!v || !v->hasName(str)) && ((names_status() != 1 && !str.empty()) || !has_name()) && CALCULATOR->variableNameTaken(str)) {
			Variable *var = CALCULATOR->getActiveVariable(str, true);
			if((!v || v != var) && (!var || var->category() != CALCULATOR->temporaryCategory()) && !ask_question(_("A unit or variable with the same name already exists.\nDo you want to overwrite it?"), GTK_WINDOW(dialog))) {
				gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(matrixedit_builder, "matrix_edit_tabs")), 0);
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_name")));
				goto run_matrix_edit_dialog;
			}
		}
		if(!v) {
			//no need to create a new variable when a variable with the same name exists
			var = CALCULATOR->getActiveVariable(str, true);
			if(var && var->isLocal() && var->isKnown()) v = (KnownVariable*) var;
		}
		MathStructure mstruct_new;
		if(!mstruct_) {
			b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
			c = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_columns")));
			r = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_spinbutton_rows")));
			gchar *gstr = NULL;
			string mstr;
			block_error();
			ParseOptions pa = evalops.parse_options; pa.base = 10; pa.rpn = false;
			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_radiobutton_vector")))) {
				mstruct_new.clearVector();
				for(size_t index_r = 0; index_r < (size_t) r && b; index_r++) {
					for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
						gtk_tree_model_get(GTK_TREE_MODEL(tMatrixEdit_store), &iter, index_c, &gstr, -1);
						mstr = gstr;
						g_free(gstr);
						remove_blank_ends(mstr);
						if(!mstr.empty()) {
							mstruct_new.addChild(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(mstr, pa), pa));
						}
					}
					b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
				}
			} else {
				mstruct_new.clearMatrix();
				mstruct_new.resizeMatrix((size_t) r, (size_t) c, m_undefined);
				for(size_t index_r = 0; index_r < (size_t) r && b; index_r++) {
					for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
						gtk_tree_model_get(GTK_TREE_MODEL(tMatrixEdit_store), &iter, index_c, &gstr, -1);
						mstr = gstr;
						g_free(gstr);
						remove_blank_ends(mstr);
						mstruct_new.setElement(CALCULATOR->parse(CALCULATOR->unlocalizeExpression(mstr, pa), pa), index_r + 1, index_c + 1);
					}
					b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrixEdit_store), &iter);
				}
			}
			display_errors(GTK_WINDOW(dialog));
			unblock_error();
		}
		bool add_var = false;
		if(v) {
			v->setLocal(true);
			//update existing variable
			if(!v->isBuiltin()) {
				if(mstruct_) {
					v->set(*mstruct_);
				} else {
					v->set(mstruct_new);
				}
			}
		} else {
			//new variable
			if(mstruct_) {
				v = new KnownVariable("", "", *mstruct_, "", true);
			} else {
				v = new KnownVariable("", "", mstruct_new, "", true);
			}
			add_var = true;
		}
		if(v) {
			v->setHidden(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrixedit_builder, "matrix_edit_checkbutton_hidden"))));
			v->setCategory(gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(matrixedit_builder, "matrix_edit_combo_category"))));
			v->setTitle(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(matrixedit_builder, "matrix_edit_entry_desc"))));
			GtkTextIter d_iter_s, d_iter_e;
			gtk_text_buffer_get_start_iter(description_buffer, &d_iter_s);
			gtk_text_buffer_get_end_iter(description_buffer, &d_iter_e);
			gchar *gstr_descr = gtk_text_buffer_get_text(description_buffer, &d_iter_s, &d_iter_e, FALSE);
			v->setDescription(gstr_descr);
			g_free(gstr_descr);
			set_edited_names(v, str);
			if(add_var) {
				CALCULATOR->addVariable(v);
			}
			variable_edited(v);
		}
	} else if(response == GTK_RESPONSE_HELP) {
		show_help("qalculate-variables.html#qalculate-vectors-matrices", GTK_WINDOW(gtk_builder_get_object(matrixedit_builder, "matrix_edit_dialog")));
		goto run_matrix_edit_dialog;
	}
	edited_matrix = NULL;
	gtk_widget_hide(dialog);
}
