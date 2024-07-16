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
#include "matrixdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *matrix_builder = NULL;

GtkWidget *tMatrix;
GtkListStore *tMatrix_store;
vector<GtkTreeViewColumn*> matrix_columns;

GtkTreeIter matrix_prev_iter;
gint matrix_prev_column = 0;
bool block_matrix_update_cursor = false;

gboolean on_tMatrix_cursor_changed(GtkTreeView*, gpointer) {
	if(block_matrix_update_cursor) return FALSE;
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrix), &path, &column);
	bool b = false;
	if(path) {
		if(column) {
			if(gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrix_store), &iter, path)) {
				gint i_column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), "column"));
				matrix_prev_iter = iter;
				matrix_prev_column = i_column;
				gchar *pos_str;
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_matrix")))) {
					pos_str = g_strdup_printf("(%i, %i)", gtk_tree_path_get_indices(path)[0] + 1, i_column + 1);
				} else {
					pos_str = g_strdup_printf("%i", (int) (i_column + 1 + matrix_columns.size() * gtk_tree_path_get_indices(path)[0]));
				}
				gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_position")), pos_str);
				g_free(pos_str);
				b = true;
			}
		}
		gtk_tree_path_free(path);
	}
	if(!b) gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_position")), _("none"));
	return FALSE;
}
void on_tMatrix_edited(GtkCellRendererText *cell, gchar *path_string, gchar *new_text, gpointer model) {
	GtkTreeIter iter;
	gint i_column = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "column"));
	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &iter, path_string);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, i_column, new_text, -1);
}
gboolean on_tMatrix_editable_key_press_event(GtkWidget *w, GdkEventKey *event, gpointer renderer) {
	switch(event->keyval) {
		case GDK_KEY_Up: {}
		case GDK_KEY_Down: {}
		case GDK_KEY_Tab: {}
		case GDK_KEY_ISO_Enter: {}
		case GDK_KEY_KP_Enter: {}
		case GDK_KEY_Return: {
			gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(w));
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrix), &path, &column);
			if(path) {
				if(column) {
					for(size_t i = 0; i < matrix_columns.size(); i++) {
						if(matrix_columns[i] == column) {
							if(event->keyval == GDK_KEY_Tab) {
								i++;
								if(i >= matrix_columns.size()) {
									gtk_tree_path_next(path);
									GtkTreeIter iter;
									if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrix_store), &iter, path)) {
										gtk_tree_path_free(path);
										path = gtk_tree_path_new_first();
									}
									i = 0;
								}
							} else {
								if(event->keyval == GDK_KEY_Up) {
									if(!gtk_tree_path_prev(path)) {
										gtk_tree_path_free(path);
										path = gtk_tree_path_new_from_indices(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tMatrix_store), NULL) - 1, -1);
									}
								} else {
									gtk_tree_path_next(path);
									GtkTreeIter iter;
									if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrix_store), &iter, path)) {
										gtk_tree_path_free(path);
										if(event->keyval != GDK_KEY_Up) return TRUE;
										path = gtk_tree_path_new_first();
									}
								}
							}
							gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrix), path, matrix_columns[i], FALSE, 0.0, 0.0);
							while(gtk_events_pending()) gtk_main_iteration();
							gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, matrix_columns[i], TRUE);
							on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
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
gboolean on_tMatrix_key_press_event(GtkWidget*, GdkEventKey *event, gpointer) {
	switch(event->keyval) {
		case GDK_KEY_Return: {break;}
		case GDK_KEY_Tab: {
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrix), &path, &column);
			if(path) {
				if(column) {
					for(size_t i = 0; i < matrix_columns.size(); i++) {
						if(matrix_columns[i] == column) {
							i++;
							if(i < matrix_columns.size()) {
								gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, matrix_columns[i], FALSE);
								while(gtk_events_pending()) gtk_main_iteration();
								gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrix), path, matrix_columns[i], FALSE, 0.0, 0.0);
							} else {
								gtk_tree_path_next(path);
								GtkTreeIter iter;
								if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tMatrix_store), &iter, path)) {
									gtk_tree_path_free(path);
									path = gtk_tree_path_new_first();
								}
								gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, matrix_columns[0], FALSE);
								while(gtk_events_pending()) gtk_main_iteration();
								gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrix), path, matrix_columns[0], FALSE, 0.0, 0.0);
							}
							gtk_tree_path_free(path);
							on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
							return TRUE;
						}
					}
				}
				gtk_tree_path_free(path);
			}
			break;
		}
		default: {
			if(event->length == 0) return FALSE;
			GtkTreeViewColumn *column = NULL;
			GtkTreePath *path = NULL;
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(tMatrix), &path, &column);
			if(path) {
				if(column) {
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, column, TRUE);
					while(gtk_events_pending()) gtk_main_iteration();
					gboolean return_val = FALSE;
					g_signal_emit_by_name((gpointer) gtk_builder_get_object(matrix_builder, "matrix_dialog"), "key_press_event", event, &return_val);
					gtk_tree_path_free(path);
					return TRUE;
				}
				gtk_tree_path_free(path);
			}
		}
	}
	return FALSE;
}
void on_tMatrix_editing_started(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data) {
	g_signal_connect(G_OBJECT(editable), "key-press-event", G_CALLBACK(on_tMatrix_editable_key_press_event), renderer);
}
gboolean on_tMatrix_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	if(event->button != 1) return FALSE;
	GtkTreeViewColumn *column = NULL;
	GtkTreePath *path = NULL;
	if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tMatrix), (gint) event->x, (gint) event->y, &path, &column, NULL, NULL) && path && column) {
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, column, TRUE);
		gtk_tree_path_free(path);
		return TRUE;
	}
	if(path) gtk_tree_path_free(path);
	return FALSE;
}
void on_matrix_spinbutton_columns_value_changed(GtkSpinButton *w, gpointer) {
	gint c = matrix_columns.size();
	gint new_c = gtk_spin_button_get_value_as_int(w);
	if(new_c < c) {
		for(gint index_c = new_c; index_c < c; index_c++) {
			gtk_tree_view_remove_column(GTK_TREE_VIEW(tMatrix), matrix_columns[index_c]);
		}
		matrix_columns.resize(new_c);
	} else {
		GtkTreeIter iter;
		for(gint index_c = c; index_c < new_c; index_c++) {
			GtkCellRenderer *matrix_renderer = gtk_cell_renderer_text_new();
			g_object_set(G_OBJECT(matrix_renderer), "editable", TRUE, NULL);
			g_object_set(G_OBJECT(matrix_renderer), "xalign", 1.0, NULL);
			g_object_set_data(G_OBJECT(matrix_renderer), "column", GINT_TO_POINTER(index_c));
			g_signal_connect(G_OBJECT(matrix_renderer), "edited", G_CALLBACK(on_tMatrix_edited), GTK_TREE_MODEL(tMatrix_store));
			g_signal_connect(G_OBJECT(matrix_renderer), "editing-started", G_CALLBACK(on_tMatrix_editing_started), GTK_TREE_MODEL(tMatrix_store));
			GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(i2s(index_c).c_str(), matrix_renderer, "text", index_c, NULL);
			g_object_set_data (G_OBJECT(column), "column", GINT_TO_POINTER(index_c));
			gtk_tree_view_column_set_min_width(column, 50);
			gtk_tree_view_column_set_alignment(column, 0.5);
			gtk_tree_view_append_column(GTK_TREE_VIEW(tMatrix), column);
			gtk_tree_view_column_set_expand(column, TRUE);
			matrix_columns.push_back(column);
		}
		if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrix_store), &iter)) return;
		bool b_matrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_matrix")));
		while(true) {
			for(gint index_c = c; index_c < new_c; index_c++) {
				if(b_matrix) gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, "0", -1);
				else gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, "", -1);
			}
			if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrix_store), &iter)) break;
		}
	}
}
void on_matrix_spinbutton_rows_value_changed(GtkSpinButton *w, gpointer) {
	gint new_r = gtk_spin_button_get_value_as_int(w);
	gint r = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(tMatrix_store), NULL);
	gint c = matrix_columns.size();
	bool b_matrix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_matrix")));
	GtkTreeIter iter;
	if(r < new_r) {
		while(r < new_r) {
			gtk_list_store_append(GTK_LIST_STORE(tMatrix_store), &iter);
			for(gint i = 0; i < c; i++) {
				if(b_matrix) gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, i, "0", -1);
				else gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, i, "", -1);
			}
			r++;
		}
	} else if(new_r < r) {
		gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(tMatrix_store), &iter, NULL, new_r);
		while(gtk_list_store_iter_is_valid(GTK_LIST_STORE(tMatrix_store), &iter)) {
			gtk_list_store_remove(GTK_LIST_STORE(tMatrix_store), &iter);
		}
	}
}
void on_matrix_radiobutton_matrix_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_elements")), _("Elements"));
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_elements")), _("Elements (in horizontal order)"));
	}
	on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
}
void on_matrix_radiobutton_vector_toggled(GtkToggleButton *w, gpointer) {
	if(!gtk_toggle_button_get_active(w)) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_elements")), _("Elements"));
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_elements")), _("Elements (in horizontal order)"));
	}
	on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
}

GtkWidget* get_matrix_dialog(void) {
	if(!matrix_builder) {

		matrix_builder = getBuilder("matrix.ui");
		g_assert(matrix_builder != NULL);

		g_assert(gtk_builder_get_object(matrix_builder, "matrix_dialog") != NULL);

		GType types[10000];
		for(gint i = 0; i < 10000; i++) {
			types[i] = G_TYPE_STRING;
		}
		tMatrix_store = gtk_list_store_newv(10000, types);
		tMatrix = GTK_WIDGET(gtk_builder_get_object(matrix_builder, "matrix_view"));
		gtk_tree_view_set_model (GTK_TREE_VIEW(tMatrix), GTK_TREE_MODEL(tMatrix_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tMatrix));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_NONE);

		gtk_builder_add_callback_symbols(matrix_builder, "on_matrix_spinbutton_rows_value_changed", G_CALLBACK(on_matrix_spinbutton_rows_value_changed), "on_matrix_spinbutton_columns_value_changed", G_CALLBACK(on_matrix_spinbutton_columns_value_changed), "on_matrix_radiobutton_matrix_toggled", G_CALLBACK(on_matrix_radiobutton_matrix_toggled), "on_matrix_radiobutton_vector_toggled", G_CALLBACK(on_matrix_radiobutton_vector_toggled), "on_tMatrix_button_press_event", G_CALLBACK(on_tMatrix_button_press_event), "on_tMatrix_cursor_changed", G_CALLBACK(on_tMatrix_cursor_changed), "on_tMatrix_key_press_event", G_CALLBACK(on_tMatrix_key_press_event), NULL);
		gtk_builder_connect_signals(matrix_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(matrix_builder, "matrix_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(matrix_builder, "matrix_dialog"));

}

bool element_needs_parenthesis(const string &str_e) {
	bool in_cit1 = false, in_cit2 = false;
	int pars = 0, brackets = 0;
	for(size_t i = 0; i < str_e.size(); i++) {
		switch(str_e[i]) {
			case LEFT_VECTOR_WRAP_CH: {
				if(!in_cit1 && !in_cit2) brackets++;
				break;
			}
			case RIGHT_VECTOR_WRAP_CH: {
				if(!in_cit1 && !in_cit2 && brackets > 0) brackets--;
				break;
			}
			case LEFT_PARENTHESIS_CH: {
				if(brackets == 0 && !in_cit1 && !in_cit2) pars++;
				break;
			}
			case RIGHT_PARENTHESIS_CH: {
				if(brackets == 0 && !in_cit1 && !in_cit2 && pars > 0) pars--;
				break;
			}
			case '\"': {
				if(in_cit1) in_cit1 = false;
				else if(!in_cit2) in_cit1 = true;
				break;
			}
			case '\'': {
				if(in_cit2) in_cit2 = false;
				else if(!in_cit1) in_cit1 = true;
				break;
			}
			default: {
				if(!in_cit1 && !in_cit2 && brackets == 0 && pars == 0 && (str_e[i] == ' ' || str_e[i] == '\n' || str_e[i] == '\t' || (str_e[i] == ',' && printops.decimalpoint() != ",") || str_e[i] == ';' || ((unsigned char) str_e[i] == 0xE2 && i + 2 < str_e.size() && (unsigned char) str_e[i + 1] == 0x80 && ((unsigned char) str_e[i + 2] == 0x89 || (unsigned char) str_e[i + 2] == 0xAF)))) {
					return true;
				}
			}
		}
	}
	return false;
}

string get_matrix(const MathStructure *initial_value, GtkWindow *win, gboolean create_vector, bool is_text_struct, bool is_result) {

	GtkWidget *dialog = get_matrix_dialog();
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));

	if(initial_value && !initial_value->isVector()) {
		return "";
	}

	if(initial_value) {
		create_vector = !initial_value->isMatrix();
	}
	if(create_vector) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_vector")), TRUE);
	else gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_matrix")), TRUE);

	if(is_result) {
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_button_cancel")), _("_Close"));
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(matrix_builder, "matrix_button_cancel")));
		if(create_vector) {
			gtk_window_set_title(GTK_WINDOW(dialog), _("Vector Result"));
		} else {
			gtk_window_set_title(GTK_WINDOW(dialog), _("Matrix Result"));
		}
	} else {
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_button_cancel")), _("_Cancel"));
		gtk_widget_grab_focus(tMatrix);
		if(create_vector) {
			gtk_window_set_title(GTK_WINDOW(dialog), _("Vector"));
		} else {
			gtk_window_set_title(GTK_WINDOW(dialog), _("Matrix"));
		}
	}

	int r = 4, c = 4;
	if(create_vector) {
		if(initial_value) {
			r = initial_value->countChildren();
			c = (int) sqrt(::sqrt(r)) + 8;
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
	} else if(initial_value) {
		c = initial_value->columns();
		r = initial_value->rows();
	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_rows")), r);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_columns")), c);
	on_matrix_spinbutton_columns_value_changed(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_columns")), NULL);
	on_matrix_spinbutton_rows_value_changed(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_rows")), NULL);


	printops.can_display_unicode_string_arg = (void*) tMatrix;
	while(gtk_events_pending()) gtk_main_iteration();
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrix_store), &iter);
	CALCULATOR->startControl(5000);
	for(size_t index_r = 0; b && index_r < (size_t) r; index_r++) {
		for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
			if(create_vector) {
				if(initial_value && index_r * c + index_c < initial_value->countChildren()) {
					if(is_text_struct) gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, initial_value->getChild(index_r * c + index_c + 1)->symbol().c_str(), -1);
					else gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, initial_value->getChild(index_r * c + index_c + 1)->print(printops).c_str(), -1);
				} else {
					gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, "", -1);
				}
			} else {
				if(initial_value) {
					if(is_text_struct) gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, initial_value->getElement(index_r + 1, index_c + 1)->symbol().c_str(), -1);
					else gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, initial_value->getElement(index_r + 1, index_c + 1)->print(printops).c_str(), -1);
				} else {
					gtk_list_store_set(GTK_LIST_STORE(tMatrix_store), &iter, index_c, "0", -1);
				}
			}
		}
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrix_store), &iter);
	}
	CALCULATOR->stopControl();

	if(r > 0 && c > 0) {
		GtkTreePath *path = gtk_tree_path_new_from_indices(0, -1);
		if(!is_result) gtk_tree_view_set_cursor(GTK_TREE_VIEW(tMatrix), path, matrix_columns[0], TRUE);
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tMatrix), path, matrix_columns[0], FALSE, 0.0, 0.0);
		on_tMatrix_cursor_changed(GTK_TREE_VIEW(tMatrix), NULL);
		gtk_tree_path_free(path);
	} else {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(matrix_builder, "matrix_label_position")), "");
	}
	printops.can_display_unicode_string_arg = NULL;

	string matrixstr;

	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_OK) {
		//clicked "OK"
		string str;
		bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tMatrix_store), &iter);
		c = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_columns")));
		r = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_spinbutton_rows")));
		gchar *gstr = NULL;
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(matrix_builder, "matrix_radiobutton_vector")))) {
			bool b1 = false;
			matrixstr = "[";
			for(size_t index_r = 0; index_r < (size_t) r && b; index_r++) {
				for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
					gtk_tree_model_get(GTK_TREE_MODEL(tMatrix_store), &iter, index_c, &gstr, -1);
					str = gstr;
					g_free(gstr);
					remove_blank_ends(str);
					if(!str.empty()) {
						if(b1) {
							matrixstr += " ";
						} else {
							b1 = true;
						}
						if(element_needs_parenthesis(str)) {
							matrixstr += "(";
							matrixstr += str;
							matrixstr += ")";
						} else {
							matrixstr += str;
						}
					}
				}
				b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrix_store), &iter);
			}
			matrixstr += "]";
		} else {
			matrixstr = "[";
			bool b1 = false;
			for(size_t index_r = 0; index_r < (size_t) r && b; index_r++) {
				if(b1) {
					matrixstr += "; ";
				} else {
					b1 = true;
				}
				bool b2 = false;
				for(size_t index_c = 0; index_c < (size_t) c; index_c++) {
					if(b2) {
						matrixstr += " ";
					} else {
						b2 = true;
					}
					gtk_tree_model_get(GTK_TREE_MODEL(tMatrix_store), &iter, index_c, &gstr, -1);
					str = gstr;
					remove_blank_ends(str);
					g_free(gstr);
					if(element_needs_parenthesis(str)) {
						matrixstr += "(";
						matrixstr += str;
						matrixstr += ")";
					} else {
						matrixstr += str;
					}
				}
				b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tMatrix_store), &iter);
			}
			matrixstr += "]";
		}
	}
	gtk_widget_hide(dialog);
	return matrixstr;
}
