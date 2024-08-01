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
#include "modes.h"
#include "mainwindow.h"
#include "shortcutsdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *shortcuts_builder = NULL;

GtkWidget *tShortcuts, *tShortcutsType;
GtkListStore *tShortcuts_store, *tShortcutsType_store;

guint32 current_shortcut_key = 0;
guint32 current_shortcut_modifier = 0;

unordered_map<guint64, keyboard_shortcut> keyboard_shortcuts;

void on_tShortcuts_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);
void on_tShortcutsType_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);

void on_tShortcuts_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		guint64 val = 0;
		gtk_tree_model_get(model, &iter, 3, &val, -1);
		unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.find(val);
		if(it != keyboard_shortcuts.end()) {
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_remove")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_edit")), TRUE);
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_remove")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_edit")), FALSE);
	}
}

void on_tShortcutsType_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), "");
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		int type = 0;
		gtk_tree_model_get(model, &iter, 1, &type, -1);
		if(type == SHORTCUT_TYPE_COPY_RESULT) {
			if(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(shortcuts_builder, "shortcuts_combo_value"))) < 0) {
				gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(shortcuts_builder, "shortcuts_combo_value")));
				for(size_t i = 0; i <= 7; i++) {
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(shortcuts_builder, "shortcuts_combo_value")), shortcut_copy_value_text(i));
				}
			}
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(shortcuts_builder, "shortcuts_combo_value")), 0);
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(shortcuts_builder, "shortcuts_stack_value")), GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_combo_value")));
		} else {
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(shortcuts_builder, "shortcuts_stack_value")), GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
		}
		switch(type) {
			case SHORTCUT_TYPE_FUNCTION: {}
			case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {}
			case SHORTCUT_TYPE_VARIABLE: {}
			case SHORTCUT_TYPE_UNIT: {}
			case SHORTCUT_TYPE_TEXT: {}
			case SHORTCUT_TYPE_CONVERT: {}
			case SHORTCUT_TYPE_TO_NUMBER_BASE: {}
			case SHORTCUT_TYPE_META_MODE: {}
			case SHORTCUT_TYPE_PRECISION: {}
			case SHORTCUT_TYPE_MIN_DECIMALS: {}
			case SHORTCUT_TYPE_MAX_DECIMALS: {}
			case SHORTCUT_TYPE_MINMAX_DECIMALS: {}
			case SHORTCUT_TYPE_COPY_RESULT: {}
			case SHORTCUT_TYPE_INPUT_BASE: {}
			case SHORTCUT_TYPE_OUTPUT_BASE: {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_label_value")), TRUE);
				break;
			}
			default: {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), FALSE);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_label_value")), FALSE);
			}
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_ok")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_add")), TRUE);
	} else {
		gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(shortcuts_builder, "shortcuts_stack_value")), GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
		const gchar *gstr = gtk_button_get_label(GTK_BUTTON(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_add")));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_ok")), strlen(gstr) > 2 && gstr[strlen(gstr) - 2] != '1');
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_add")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_label_value")), FALSE);
	}
}

GtkWidget *shortcut_label = NULL;
gboolean on_shortcut_key_released(GtkWidget *w, GdkEventKey *event, gpointer) {
	guint state = CLEAN_MODIFIERS(event->state);
	FIX_ALT_GR
	if(event->keyval == 0 || (event->keyval >= GDK_KEY_Shift_L && event->keyval <= GDK_KEY_Hyper_R)) return FALSE;
	if(state == 0 && event->keyval == GDK_KEY_Escape) {
		gtk_dialog_response(GTK_DIALOG(w), GTK_RESPONSE_CANCEL);
		return TRUE;
	}
	if(state == 0 && event->keyval >= GDK_KEY_ampersand && event->keyval <= GDK_KEY_z) return FALSE;
	current_shortcut_key = event->keyval;
	current_shortcut_modifier = state;
	gtk_dialog_response(GTK_DIALOG(w), GTK_RESPONSE_OK);
	return TRUE;
}
gboolean on_shortcut_key_pressed(GtkWidget *w, GdkEventKey *event, gpointer) {
	guint state = CLEAN_MODIFIERS(event->state);
	FIX_ALT_GR
	string str = "<span size=\"large\">";
	str += shortcut_to_text(event->keyval, state);
	str += "</span>";
	gtk_label_set_markup(GTK_LABEL(shortcut_label), str.c_str());
	return FALSE;
}
bool get_keyboard_shortcut(GtkWindow *parent) {
	GtkWidget *dialog = gtk_dialog_new();
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Set key combination"));
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	// Make the line reasonably long, but not to short (at least around 40 characters)
	string str = "<i>"; str += _("Press the key combination you wish to use for the action\n(press Escape to cancel)."); str += "</i>";
	GtkWidget *label = gtk_label_new(str.c_str());
	gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 16
	gtk_label_set_xalign(GTK_LABEL(label), 0.0);
#else
	gtk_widget_set_halign(label, GTK_ALIGN_START);
#endif
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label, FALSE, TRUE, 6);
	gtk_widget_show(label);
	str = "<span size=\"large\">"; str += _("No keys"); str += "</span>";
	shortcut_label = gtk_label_new(str.c_str());
	gtk_label_set_use_markup(GTK_LABEL(shortcut_label), TRUE);
	g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_shortcut_key_pressed), dialog);
	g_signal_connect(dialog, "key-release-event", G_CALLBACK(on_shortcut_key_released), dialog);
	gtk_box_pack_end(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), shortcut_label, TRUE, TRUE, 18);
	gtk_widget_show(shortcut_label);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		gtk_widget_destroy(dialog);
		return current_shortcut_key != 0;
	}
	gtk_widget_destroy(dialog);
	return false;
}
void on_shortcuts_entry_value_activate(GtkEntry*, gpointer d) {
	gtk_dialog_response(GTK_DIALOG(gtk_builder_get_object(shortcuts_builder, "shortcuts_type_dialog")), GTK_RESPONSE_ACCEPT);
}
void on_shortcuts_type_treeview_row_activated(GtkTreeView*, GtkTreePath*, GtkTreeViewColumn*, gpointer) {
	if(gtk_widget_get_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")))) gtk_widget_grab_focus(gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(shortcuts_builder, "shortcuts_stack_value"))));
	else gtk_dialog_response(GTK_DIALOG(gtk_builder_get_object(shortcuts_builder, "shortcuts_type_dialog")), GTK_RESPONSE_ACCEPT);
}
void on_shortcuts_button_new_clicked(GtkButton*, gpointer) {
	GtkWidget *d = GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_type_dialog"));
	update_window_properties(d);
	gtk_widget_grab_focus(tShortcutsType);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), "");
	gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(shortcuts_builder, "shortcuts_stack_value")), GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
	string label = _("Add Action");	label += " (1)";
	gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_add")), label.c_str());
	int type = -1;
	string value;
	keyboard_shortcut ks;
	run_shortcuts_dialog:
	int ret = gtk_dialog_run(GTK_DIALOG(d));
	if(ret == GTK_RESPONSE_ACCEPT || ret == GTK_RESPONSE_APPLY) {
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcutsType));
		type = -1;
		if(gtk_tree_selection_get_selected(select, &model, &iter)) gtk_tree_model_get(GTK_TREE_MODEL(tShortcutsType_store), &iter, 1, &type, -1);
		else if(ks.type.empty()) goto run_shortcuts_dialog;
		if(type == SHORTCUT_TYPE_COPY_RESULT) {
			value = i2s(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(shortcuts_builder, "shortcuts_combo_value"))));
		} else if(gtk_widget_is_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")))) {
			value = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
			if(type != SHORTCUT_TYPE_TEXT) remove_blank_ends(value);
			if(value.empty()) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
				show_message(_("Empty value."), GTK_WINDOW(d));
				goto run_shortcuts_dialog;
			}
			switch(type) {
				case SHORTCUT_TYPE_FUNCTION: {}
				case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
					if(value.length() > 2 && value.substr(value.length() - 2, 2) == "()") value = value.substr(0, value.length() - 2);
					if(!CALCULATOR->getActiveFunction(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Function not found."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_VARIABLE: {
					if(!CALCULATOR->getActiveVariable(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Variable not found."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_UNIT: {
					if(!CALCULATOR->getActiveUnit(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Unit not found."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_META_MODE: {
					if(mode_index(value, false) == (size_t) -1) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Mode not found."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_TO_NUMBER_BASE: {}
				case SHORTCUT_TYPE_INPUT_BASE: {}
				case SHORTCUT_TYPE_OUTPUT_BASE: {
					Number nbase; int base;
					base_from_string(value, base, nbase, type == SHORTCUT_TYPE_INPUT_BASE);
					if(base == BASE_CUSTOM && nbase.isZero()) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Unsupported base."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_PRECISION: {}
				case SHORTCUT_TYPE_MIN_DECIMALS: {}
				case SHORTCUT_TYPE_MAX_DECIMALS: {}
				case SHORTCUT_TYPE_MINMAX_DECIMALS: {
					int v = s2i(value);
					if(value.find_first_not_of(SPACE NUMBERS) != string::npos || v < -1 || (type == SHORTCUT_TYPE_PRECISION && v < 2)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Unsupported value."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
			}
		} else {
			value = "";
		}
		if(ks.type.empty() || type != -1) {
			ks.type.push_back(type);
			ks.value.push_back(value);
		}
		if(ret == GTK_RESPONSE_APPLY) {
			string label = _("Add Action"); label += " ("; label += i2s(ks.type.size() + 1); label += ")";
			gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_add")), label.c_str());
			gtk_widget_grab_focus(tShortcutsType);
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), "");
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcutsType)));
			goto run_shortcuts_dialog;
		}
		ask_keyboard_shortcut:
		if(get_keyboard_shortcut(GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_type_dialog")))) {
			ks.key = current_shortcut_key;
			ks.modifier = current_shortcut_modifier;
			guint64 id = (guint64) ks.key + (guint64) G_MAXUINT32 * (guint64) ks.modifier;
			unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.find(id);
			if(it != keyboard_shortcuts.end()) {
				if(!ask_question(_("The key combination is already in use.\nDo you wish to replace the current action?"), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_type_dialog")))) {
					goto ask_keyboard_shortcut;
				}
				GtkTreeIter iter;
				if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tShortcuts_store), &iter)) {
					do {
						guint64 id2 = 0;
						gtk_tree_model_get(GTK_TREE_MODEL(tShortcuts_store), &iter, 3, &id2, -1);
						if(id2 == id) {
							gtk_list_store_remove(tShortcuts_store, &iter);
							break;
						}
					} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tShortcuts_store), &iter));
				}
				int type = -1;
				if(it->second.type.size() == 1) type = it->second.type[0];
				keyboard_shortcuts.erase(id);
				if(type >= 0) update_accels(type);
			}
			default_shortcuts = false;
			GtkTreeIter iter;
			gtk_list_store_append(tShortcuts_store, &iter);
			gtk_list_store_set(tShortcuts_store, &iter, 0, shortcut_types_text(ks.type).c_str(), 1, shortcut_values_text(ks.value, ks.type).c_str(), 2, shortcut_to_text(ks.key, ks.modifier).c_str(), 3, id, -1);
			keyboard_shortcuts[id] = ks;
			if(ks.type.size() == 1) update_accels(ks.type[0]);
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), "");
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_remove")), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_edit")), FALSE);
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcuts)));
		}
	}
	gtk_widget_hide(d);
}
void on_shortcuts_button_remove_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcuts));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		guint64 id = 0;
		gtk_tree_model_get(GTK_TREE_MODEL(tShortcuts_store), &iter, 3, &id, -1);
		unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.find(id);
		int type = -1;
		if(it != keyboard_shortcuts.end() && it->second.type.size() == 1) type = it->second.type[0];
		keyboard_shortcuts.erase(id);
		if(type >= 0) update_accels(type);
		gtk_list_store_remove(tShortcuts_store, &iter);
		default_shortcuts = false;
	}
}
void on_shortcuts_button_edit_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcuts));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		guint64 id;
		gtk_tree_model_get(GTK_TREE_MODEL(tShortcuts_store), &iter, 3, &id, -1);
		unordered_map<guint64, keyboard_shortcut>::iterator it_old = keyboard_shortcuts.find(id);
		if(it_old != keyboard_shortcuts.end() && get_keyboard_shortcut(GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")))) {
			keyboard_shortcut ks;
			ks.type = it_old->second.type;
			ks.value = it_old->second.value;
			ks.key = current_shortcut_key;
			ks.modifier = current_shortcut_modifier;
			id = (guint64) ks.key + (guint64) G_MAXUINT32 * (guint64) ks.modifier;
			unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.find(id);
			bool b_replace = false;
			int old_type = -1;
			if(it != keyboard_shortcuts.end()) {
				if(it == it_old || !ask_question(_("The key combination is already in use.\nDo you wish to replace the current action?"), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")))) {
					return;
				}
				if(it->second.type.size() == 1) old_type = it->second.type[0];
				b_replace = true;
			}
			keyboard_shortcuts.erase(it_old);
			g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tShortcuts_selection_changed, NULL);
			gtk_list_store_remove(tShortcuts_store, &iter);
			g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tShortcuts_selection_changed, NULL);
			default_shortcuts = false;
			if(b_replace) {
				if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tShortcuts_store), &iter)) {
					do {
						guint64 id2 = 0;
						gtk_tree_model_get(GTK_TREE_MODEL(tShortcuts_store), &iter, 3, &id2, -1);
						if(id2 == id) {
							gtk_list_store_remove(tShortcuts_store, &iter);
							break;
						}
					} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tShortcuts_store), &iter));
				}
				update_accels(old_type);
				keyboard_shortcuts.erase(id);
			}
			keyboard_shortcuts[id] = ks;
			if(ks.type.size() == 1) update_accels(ks.type[0]);
			gtk_list_store_append(tShortcuts_store, &iter);
			gtk_list_store_set(tShortcuts_store, &iter, 0, shortcut_types_text(ks.type).c_str(), 1, shortcut_values_text(ks.value, ks.type).c_str(), 2, shortcut_to_text(ks.key, ks.modifier).c_str(), 3, id, -1);
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcuts)), &iter);
		}
		default_shortcuts = false;
	}
}
void on_shortcuts_treeview_row_activated(GtkTreeView *w, GtkTreePath *path, GtkTreeViewColumn *column, gpointer) {
	if(column == gtk_tree_view_get_column(w, 2)) {
		on_shortcuts_button_edit_clicked(NULL, NULL);
		return;
	}
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tShortcuts_store), &iter, path)) return;
	guint64 id;
	gtk_tree_model_get(GTK_TREE_MODEL(tShortcuts_store), &iter, 3, &id, -1);
	unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.find(id);
	if(it == keyboard_shortcuts.end() && !it->second.type.empty()) return;
	GtkWidget *d = GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_type_dialog"));
	update_window_properties(d);
	GtkTreeIter iter2;
	GtkTreeModel *model;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcutsType));
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tShortcutsType_store), &iter2)) {
		do {
			int type = 0;
			gtk_tree_model_get(GTK_TREE_MODEL(tShortcutsType_store), &iter2, 1, &type, -1);
			if(type == it->second.type[0]) {
				gtk_tree_selection_select_iter(select, &iter2);
				GtkTreePath *path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(tShortcutsType)), &iter2);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tShortcutsType), path, NULL, TRUE, 0.5, 0);
				gtk_tree_path_free(path);
				break;
			}
		} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tShortcutsType_store), &iter2));
	}
	if(it->second.type[0] == SHORTCUT_TYPE_COPY_RESULT) {
		int v = s2i(it->second.value[0]);
		if(v < 0 || v > 7) v = 0;
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(shortcuts_builder, "shortcuts_combo_value")), v);
	} else {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), it->second.value[0].c_str());
	}
	if(column == gtk_tree_view_get_column(w, 1)) gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
	else gtk_widget_grab_focus(GTK_WIDGET(w));
	string label = _("Add Action");	label += " (1)";
	gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_add")), label.c_str());
	int type = -1;
	string value;
	keyboard_shortcut ks;
	run_shortcuts_dialog:
	int ret = gtk_dialog_run(GTK_DIALOG(d));
	if(ret == GTK_RESPONSE_ACCEPT || ret == GTK_RESPONSE_APPLY) {
		type = -1;
		if(gtk_tree_selection_get_selected(select, &model, &iter2)) gtk_tree_model_get(GTK_TREE_MODEL(tShortcutsType_store), &iter2, 1, &type, -1);
		else if(ks.type.empty()) goto run_shortcuts_dialog;
		if(type == SHORTCUT_TYPE_COPY_RESULT) {
			value = i2s(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(shortcuts_builder, "shortcuts_combo_value"))));
		} else if(gtk_widget_is_sensitive(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")))) {
			value = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
			if(type != SHORTCUT_TYPE_TEXT) remove_blank_ends(value);
			if(value.empty()) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
				show_message(_("Empty value."), GTK_WINDOW(d));
				goto run_shortcuts_dialog;
			}
			switch(type) {
				case SHORTCUT_TYPE_FUNCTION: {}
				case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
					if(value.length() > 2 && value.substr(value.length() - 2, 2) == "()") value = value.substr(0, value.length() - 2);
					if(!CALCULATOR->getActiveFunction(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Function not found."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_VARIABLE: {
					if(!CALCULATOR->getActiveVariable(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Variable not found."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_UNIT: {
					if(!CALCULATOR->getActiveUnit(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Unit not found."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_META_MODE: {
					if(mode_index(value, false) == (size_t) -1) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Mode not found."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_TO_NUMBER_BASE: {}
				case SHORTCUT_TYPE_INPUT_BASE: {}
				case SHORTCUT_TYPE_OUTPUT_BASE: {
					Number nbase; int base;
					base_from_string(value, base, nbase, type == SHORTCUT_TYPE_INPUT_BASE);
					if(base == BASE_CUSTOM && nbase.isZero()) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Unsupported base."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_PRECISION: {}
				case SHORTCUT_TYPE_MIN_DECIMALS: {}
				case SHORTCUT_TYPE_MAX_DECIMALS: {}
				case SHORTCUT_TYPE_MINMAX_DECIMALS: {
					int v = s2i(value);
					if(value.find_first_not_of(SPACE NUMBERS) != string::npos || v < -1 || (type == SHORTCUT_TYPE_PRECISION && v < 2)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")));
						show_message(_("Unsupported value."), GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
			}
		} else {
			value = "";
		}
		if(ks.type.empty() || type != -1) {
			ks.type.push_back(type);
			ks.value.push_back(value);
		}
		if(ret == GTK_RESPONSE_APPLY) {
			string label = _("Add Action"); label += " ("; label += i2s(ks.type.size() + 1); label += ")";
			gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(shortcuts_builder, "shortcuts_button_add")), label.c_str());
			gtk_widget_grab_focus(tShortcutsType);
			gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(shortcuts_builder, "shortcuts_entry_value")), "");
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcutsType)));
			goto run_shortcuts_dialog;
		}
		int old_type = -1;
		if(it->second.type.size() == 1) old_type = it->second.type[0];
		it->second.type = ks.type;
		it->second.value = ks.value;
		if(old_type >= 0) update_accels(old_type);
		if(ks.type.size() == 1) update_accels(ks.type[0]);
		gtk_list_store_set(tShortcuts_store, &iter, 0, shortcut_types_text(ks.type).c_str(), 1, shortcut_values_text(ks.value, ks.type).c_str(), -1);
	}
	gtk_widget_hide(d);
}

GtkWidget* get_shortcuts_dialog(void) {
	if(!shortcuts_builder) {

		shortcuts_builder = getBuilder("shortcuts.ui");
		g_assert(shortcuts_builder != NULL);

		g_assert(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog") != NULL);

		tShortcuts = GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_treeview"));

		tShortcuts_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tShortcuts), GTK_TREE_MODEL(tShortcuts_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcuts));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Action"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tShortcuts), column);
		renderer = gtk_cell_renderer_text_new();
		g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, "width-chars", 20, NULL);
		column = gtk_tree_view_column_new_with_attributes(_("Value"), renderer, "text", 1, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 1);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tShortcuts), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Key combination"), renderer, "text", 2, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 2);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tShortcuts), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tShortcuts_selection_changed), NULL);

		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tShortcuts_store), 0, GTK_SORT_ASCENDING);

		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tShortcuts_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tShortcuts_store), 1, string_sort_func, GINT_TO_POINTER(1), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tShortcuts_store), 2, string_sort_func, GINT_TO_POINTER(2), NULL);

		tShortcutsType = GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_type_treeview"));

		tShortcutsType_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tShortcutsType), GTK_TREE_MODEL(tShortcutsType_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tShortcutsType));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Action"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tShortcutsType), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tShortcutsType_selection_changed), NULL);

		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tShortcutsType_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);

		for(int i = 0; i <= SHORTCUT_TYPE_QUIT; i++) {
			GtkTreeIter iter;
			gtk_list_store_append(tShortcutsType_store, &iter);
			gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(i), 1, i, -1);
			if(i == SHORTCUT_TYPE_RPN_MODE) {
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_CHAIN_MODE), 1, SHORTCUT_TYPE_CHAIN_MODE, -1);
			} else if(i == SHORTCUT_TYPE_COPY_RESULT) {
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_INSERT_RESULT), 1, SHORTCUT_TYPE_INSERT_RESULT, -1);
			} else if(i == SHORTCUT_TYPE_HISTORY_SEARCH) {
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_HISTORY_CLEAR), 1, SHORTCUT_TYPE_HISTORY_CLEAR, -1);
			} else if(i == SHORTCUT_TYPE_MINIMAL) {
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_ALWAYS_ON_TOP), 1, SHORTCUT_TYPE_ALWAYS_ON_TOP, -1);
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_DO_COMPLETION), 1, SHORTCUT_TYPE_DO_COMPLETION, -1);
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION), 1, SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION, -1);
			} else if(i == SHORTCUT_TYPE_SIMPLE_NOTATION) {
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_PRECISION), 1, SHORTCUT_TYPE_PRECISION, -1);
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_MIN_DECIMALS), 1, SHORTCUT_TYPE_MIN_DECIMALS, -1);
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_MAX_DECIMALS), 1, SHORTCUT_TYPE_MAX_DECIMALS, -1);
				gtk_list_store_append(tShortcutsType_store, &iter);
				gtk_list_store_set(tShortcutsType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_MINMAX_DECIMALS), 1, SHORTCUT_TYPE_MINMAX_DECIMALS, -1);
			}
			if(i == 0) gtk_tree_selection_select_iter(selection, &iter);
		}

		for(unordered_map<guint64, keyboard_shortcut>::iterator it = keyboard_shortcuts.begin(); it != keyboard_shortcuts.end(); ++it) {
			GtkTreeIter iter;
			gtk_list_store_insert(tShortcuts_store, &iter, 0);
			gtk_list_store_set(tShortcuts_store, &iter, 0, shortcut_types_text(it->second.type).c_str(), 1, shortcut_values_text(it->second.value, it->second.type).c_str(), 2, shortcut_to_text(it->second.key, it->second.modifier).c_str(), 3, (guint64) it->second.key + (guint64) G_MAXUINT32 * (guint64) it->second.modifier, -1);
		}

		gtk_builder_add_callback_symbols(shortcuts_builder, "on_shortcuts_treeview_row_activated", G_CALLBACK(on_shortcuts_treeview_row_activated), "on_shortcuts_button_new_clicked", G_CALLBACK(on_shortcuts_button_new_clicked), "on_shortcuts_button_edit_clicked", G_CALLBACK(on_shortcuts_button_edit_clicked), "on_shortcuts_button_remove_clicked", G_CALLBACK(on_shortcuts_button_remove_clicked), "on_shortcuts_type_treeview_row_activated", G_CALLBACK(on_shortcuts_type_treeview_row_activated), "on_shortcuts_entry_value_activate", G_CALLBACK(on_shortcuts_entry_value_activate), NULL);
		gtk_builder_connect_signals(shortcuts_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_dialog"));
}

void edit_shortcuts(GtkWindow *parent) {
	GtkWidget *dialog = get_shortcuts_dialog();
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(shortcuts_builder, "shortcuts_treeview")));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_widget_show(dialog);
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}
