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
#include "keypad.h"
#include "buttonseditdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *buttonsedit_builder = NULL;
extern GtkBuilder *main_builder;

GtkWidget *tButtonsEditType, *tButtonsEdit;
GtkListStore *tButtonsEditType_store, *tButtonsEdit_store;

extern std::vector<custom_button> custom_buttons;

#define SET_BUTTONS_EDIT_ITEM_3(l, t1, t2, t3) \
	{gtk_list_store_set(tButtonsEdit_store, &iter, 1, custom_buttons[i].text.empty() ? l : custom_buttons[i].text.c_str(), 2, custom_buttons[i].type[0] == -1 ? t1 : button_valuetype_text(custom_buttons[i].type[0], custom_buttons[i].value[0]).c_str(), 3, custom_buttons[i].type[1] == -1 ? t2 : button_valuetype_text(custom_buttons[i].type[1], custom_buttons[i].value[1]).c_str(), 4, custom_buttons[i].type[2] == -1 ? t3 : button_valuetype_text(custom_buttons[i].type[2], custom_buttons[i].value[2]).c_str(), -1);\
	if(index >= 0) break;}

#define SET_BUTTONS_EDIT_ITEM_3B(l, t1, t2, t3) SET_BUTTONS_EDIT_ITEM_3(gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(main_builder, l))), t1, t2, t3)
#define SET_BUTTONS_EDIT_ITEM_2(l, t1, t2) SET_BUTTONS_EDIT_ITEM_3(l, t1, t2, t2)
#define SET_BUTTONS_EDIT_ITEM_2B(l, t1, t2) SET_BUTTONS_EDIT_ITEM_3(gtk_label_get_text(GTK_LABEL(gtk_builder_get_object(main_builder, l))), t1, t2, t2)
#define SET_BUTTONS_EDIT_ITEM_C(l) SET_BUTTONS_EDIT_ITEM_3(l, "-", "-", "-")

void on_tButtonsEdit_selection_changed(GtkTreeSelection *treeselection, gpointer);
void on_tButtonsEdit_update_selection(GtkTreeSelection *treeselection, bool update_label_entry);
void on_tButtonsEditType_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);
void on_buttonsedit_button_clicked(GtkButton *w, gpointer user_data);
void on_buttonsedit_defaults_clicked(GtkButton *w, gpointer user_data);
void on_buttonsedit_label_changed(GtkEditable *w, gpointer user_data);
void update_custom_buttons_edit(int index = -1, bool update_label_entry = true);

void on_tButtonsEditType_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")), "");
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		int type = 0;
		gtk_tree_model_get(model, &iter, 1, &type, -1);
		if(type == SHORTCUT_TYPE_COPY_RESULT) {
			if(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(buttonsedit_builder, "shortcuts_combo_value"))) < 0) {
				gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(buttonsedit_builder, "shortcuts_combo_value")));
				for(size_t i = 0; i <= 7; i++) {
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gtk_builder_get_object(buttonsedit_builder, "shortcuts_combo_value")), shortcut_copy_value_text(i));
				}
			}
			gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(buttonsedit_builder, "shortcuts_combo_value")), 0);
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(buttonsedit_builder, "shortcuts_stack_value")), GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_combo_value")));
		} else {
			gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(buttonsedit_builder, "shortcuts_stack_value")), GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
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
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")), TRUE);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_label_value")), TRUE);
				break;
			}
			default: {
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")), FALSE);
				gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_label_value")), FALSE);
			}
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_button_ok")), TRUE);
	} else {
		gtk_stack_set_visible_child(GTK_STACK(gtk_builder_get_object(buttonsedit_builder, "shortcuts_stack_value")), GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_button_ok")), FALSE);
	}
}

void on_buttonsedit_type_treeview_row_activated(GtkTreeView*, GtkTreePath*, GtkTreeViewColumn*, gpointer) {
	if(gtk_widget_get_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")))) gtk_widget_grab_focus(gtk_stack_get_visible_child(GTK_STACK(gtk_builder_get_object(buttonsedit_builder, "shortcuts_stack_value"))));
	else gtk_dialog_response(GTK_DIALOG(gtk_builder_get_object(buttonsedit_builder, "shortcuts_type_dialog")), GTK_RESPONSE_ACCEPT);
}

void on_buttonsedit_entry_value_activate(GtkEntry*, gpointer d) {
	gtk_dialog_response(GTK_DIALOG(gtk_builder_get_object(buttonsedit_builder, "shortcuts_type_dialog")), GTK_RESPONSE_ACCEPT);
}

void on_buttons_edit_entry_label_changed(GtkEditable *w, gpointer user_data) {
	int i = 0;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEdit));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(!gtk_tree_selection_get_selected(select, &model, &iter)) return;
	gtk_tree_model_get(model, &iter, 0, &i, -1);
	gtk_entry_get_text(GTK_ENTRY(w));
	custom_buttons[i].text = gtk_entry_get_text(GTK_ENTRY(w));
	update_custom_buttons(i);
	update_custom_buttons_edit(i, false);
}
void on_tButtonsEdit_update_selection(GtkTreeSelection *treeselection, bool update_label_entry) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(update_label_entry) g_signal_handlers_block_matched((gpointer) gtk_builder_get_object(buttonsedit_builder, "buttons_edit_entry_label"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_buttons_edit_entry_label_changed, NULL);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		int i = 0;
		gchar *gstr, *gstr2, *gstr3, *gstr4;
		gtk_tree_model_get(model, &iter, 0, &i, 1, &gstr, 2, &gstr2, 3, &gstr3, 4, &gstr4, -1);
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_button_1")), gstr2);
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_button_2")), gstr3);
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_button_3")), gstr4);
		if(update_label_entry) {
			if(i <= 1) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_entry_label")), custom_buttons[i].text.c_str());
			else gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_entry_label")), gstr);
		}
		g_free(gstr); g_free(gstr2); g_free(gstr3); g_free(gstr4);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_box_edit")), TRUE);
	} else {
		if(update_label_entry) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_entry_label")), "");
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_box_edit")), FALSE);
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_button_1")), "");
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_button_2")), "");
		gtk_widget_set_tooltip_text(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_button_3")), "");
	}
	if(update_label_entry) g_signal_handlers_unblock_matched((gpointer) gtk_builder_get_object(buttonsedit_builder, "buttons_edit_entry_label"), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_buttons_edit_entry_label_changed, NULL);
}
void on_tButtonsEdit_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	on_tButtonsEdit_update_selection(treeselection, true);
}
void on_buttonsedit_button_x_clicked(int b_i) {
	int i = 0;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEdit));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(!gtk_tree_selection_get_selected(select, &model, &iter)) return;
	gtk_tree_model_get(model, &iter, 0, &i, -1);
	GtkWidget *d = GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_type_dialog"));
	update_window_properties(d);
	GtkTreeIter iter2;
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEditType));
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tButtonsEditType_store), &iter2)) {
		int type = 0;
		gtk_tree_model_get(GTK_TREE_MODEL(tButtonsEditType_store), &iter2, 1, &type, -1);
		if(type == -1 && i >= 29) {
			gtk_list_store_remove(tButtonsEditType_store, &iter2);
		} else if(type == -2 && i < 29) {
			gtk_list_store_insert(tButtonsEditType_store, &iter2, 0);
			gtk_list_store_set(tButtonsEditType_store, &iter2, 0, _("Default"), 1, -1, -1);
		}
		do {
			int type = 0;
			gtk_tree_model_get(GTK_TREE_MODEL(tButtonsEditType_store), &iter2, 1, &type, -1);
			if(type == -2 && i >= 29) type = -1;
			if(type == custom_buttons[i].type[b_i]) {
				gtk_tree_selection_select_iter(select, &iter2);
				GtkTreePath *path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(tButtonsEditType)), &iter2);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tButtonsEditType), path, NULL, TRUE, 0.5, 0);
				gtk_tree_path_free(path);
				break;
			}
		} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tButtonsEditType_store), &iter2));
	}
	if(custom_buttons[i].type[b_i] == SHORTCUT_TYPE_COPY_RESULT) {
		int v = s2i(custom_buttons[i].value[b_i]);
		if(v < 0 || v > 7) v = 0;
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(buttonsedit_builder, "shortcuts_combo_value")), v);
	} else {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")), custom_buttons[i].value[b_i].c_str());
	}
	gtk_widget_grab_focus(tButtonsEditType);
	run_shortcuts_dialog:
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
		int type = 0;
		string value;
		if(gtk_tree_selection_get_selected(select, &model, &iter2)) gtk_tree_model_get(GTK_TREE_MODEL(tButtonsEditType_store), &iter2, 1, &type, -1);
		else goto run_shortcuts_dialog;
		if(i >= 29 && type == -2) type = -1;
		if(type == SHORTCUT_TYPE_COPY_RESULT) {
			value = i2s(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(buttonsedit_builder, "shortcuts_combo_value"))));
		} else if(gtk_widget_is_sensitive(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")))) {
			value = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
			if(type != SHORTCUT_TYPE_TEXT) remove_blank_ends(value);
			if(value.empty()) {
				gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
				show_message(_("Empty value."), GTK_WINDOW(d));
				goto run_shortcuts_dialog;
			}
			switch(type) {
				case SHORTCUT_TYPE_FUNCTION: {}
				case SHORTCUT_TYPE_FUNCTION_WITH_DIALOG: {
					remove_blanks(value);
					if(value.length() > 2 && value.substr(value.length() - 2, 2) == "()") value = value.substr(0, value.length() - 2);
					if(!CALCULATOR->getActiveFunction(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
						show_message(_("Function not found."), GTK_WINDOW(gtk_builder_get_object(buttonsedit_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_VARIABLE: {
					if(!CALCULATOR->getActiveVariable(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
						show_message(_("Variable not found."), GTK_WINDOW(gtk_builder_get_object(buttonsedit_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_UNIT: {
					if(!CALCULATOR->getActiveUnit(value)) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
						show_message(_("Unit not found."), GTK_WINDOW(gtk_builder_get_object(buttonsedit_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
				case SHORTCUT_TYPE_META_MODE: {
					if(mode_index(value, false) == (size_t) -1) {
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
						show_message(_("Mode not found."), GTK_WINDOW(gtk_builder_get_object(buttonsedit_builder, "shortcuts_dialog")));
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
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
						show_message(_("Unsupported base."), GTK_WINDOW(gtk_builder_get_object(buttonsedit_builder, "shortcuts_dialog")));
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
						gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_entry_value")));
						show_message(_("Unsupported value."), GTK_WINDOW(gtk_builder_get_object(buttonsedit_builder, "shortcuts_dialog")));
						goto run_shortcuts_dialog;
					}
					break;
				}
			}
		} else {
			value = "";
		}
		custom_buttons[i].type[b_i] = type;
		custom_buttons[i].value[b_i] = value;
		update_custom_buttons(i);
		update_custom_buttons_edit(i, false);
	}
	gtk_widget_hide(d);
}
void on_buttons_edit_button_1_clicked(GtkButton*, gpointer user_data) {
	on_buttonsedit_button_x_clicked(0);
}
void on_buttons_edit_button_2_clicked(GtkButton*, gpointer user_data) {
	on_buttonsedit_button_x_clicked(1);
}
void on_buttons_edit_button_3_clicked(GtkButton*, gpointer user_data) {
	on_buttonsedit_button_x_clicked(2);
}
void on_buttons_edit_button_defaults_clicked(GtkButton *w, gpointer user_data) {
	int i = 0;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEdit));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(!gtk_tree_selection_get_selected(select, &model, &iter)) return;
	gtk_tree_model_get(model, &iter, 0, &i, -1);
	custom_buttons[i].type[0] = -1;
	custom_buttons[i].value[0] = "";
	custom_buttons[i].type[1] = -1;
	custom_buttons[i].value[1] = "";
	custom_buttons[i].type[2] = -1;
	custom_buttons[i].value[2] = "";
	custom_buttons[i].text = "";
	update_custom_buttons(i);
	update_custom_buttons_edit(i, true);
}
void on_buttons_edit_treeview_row_activated(GtkTreeView *w, GtkTreePath *path, GtkTreeViewColumn *column, gpointer) {
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(tButtonsEdit_store), &iter, path)) return;
	int i = 0;
	gtk_tree_model_get(GTK_TREE_MODEL(tButtonsEdit_store), &iter, 0, &i, -1);
	if(column == gtk_tree_view_get_column(w, 0)) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_entry_label")));
	} else if(column == gtk_tree_view_get_column(w, 1)) {
		on_buttons_edit_button_1_clicked(NULL, NULL);
	} else if(column == gtk_tree_view_get_column(w, 2)) {
		on_buttons_edit_button_2_clicked(NULL, NULL);
	} else if(column == gtk_tree_view_get_column(w, 3)) {
		on_buttons_edit_button_3_clicked(NULL, NULL);
	}
}

void update_custom_buttons_edit(int index, bool update_label_entry) {
	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tButtonsEdit_store), &iter);
	int i = 0;
	do {
		gtk_tree_model_get(GTK_TREE_MODEL(tButtonsEdit_store), &iter, 0, &i, -1);
		if(i == 0 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("↑ ↓", _("Cycle through previous expression"), "-")
		else if(i == 1 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("← →", _("Move cursor left or right"), _("Move cursor to beginning or end"))
		else if(i == 2 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("%", CALCULATOR->v_percent->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tButtonsEdit).c_str(), CALCULATOR->v_permille->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tButtonsEdit).c_str())
		else if(i == 3 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("±", _("Uncertainty/interval"), _("Relative error"), _("Interval"))
		else if(i == 4 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3B("label_comma", _("Argument separator"), _("Blank space"), _("New line"))
		else if(i == 5 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("(x)", _("Smart parentheses"), _("Vector brackets"))
		else if(i == 6 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("(", _("Left parenthesis"), _("Left vector bracket"))
		else if(i == 7 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2(")", _("Right parenthesis"), _("Right vector bracket"))
		else if(i == 8 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("0", "0", "x^0", CALCULATOR->getDegUnit()->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tButtonsEdit).c_str())
		else if(i == 9 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("1", "1", "x^1", "1/x")
		else if(i == 10 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("2", "2", "x^2", "1/2")
		else if(i == 11 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("3", "3", "x^3", "1/3")
		else if(i == 12 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("4", "4", "x^4", "1/4")
		else if(i == 13 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("5", "5", "x^5", "1/5")
		else if(i == 14 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("6", "6", "x^6", "1/6")
		else if(i == 15 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("7", "7", "x^7", "1/7")
		else if(i == 16 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("8", "8", "x^8", "1/8")
		else if(i == 17 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("9", "9", "x^9", "1/9")
		else if(i == 18 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3B("label_dot", _("Decimal point"), _("Blank space"), _("New line"))
		else if(i == 19 && (index == i || index < 0)) {
			MathFunction *f = CALCULATOR->getActiveFunction("exp10");
			SET_BUTTONS_EDIT_ITEM_3B("label_exp", "10^x", CALCULATOR->f_exp->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tButtonsEdit).c_str(), (f ? f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tButtonsEdit).c_str() : "-"));
		} else if(i == 20 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2("x^y", _("Raise"), CALCULATOR->f_sqrt->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tButtonsEdit).c_str())
		else if(i == 21 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2B("label_divide", _("Divide"), "1/x")
		else if(i == 22 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2B("label_times", _("Multiply"), _("Bitwise Exclusive OR"))
		else if(i == 23 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3B("label_add", _("Add"), _("M+ (memory plus)"), _("Bitwise AND"))
		else if(i == 24 && (index == i || index < 0)) {
			MathFunction *f = CALCULATOR->getActiveFunction("neg");
			SET_BUTTONS_EDIT_ITEM_3B("label_sub", _("Subtract"), f ? f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tButtonsEdit).c_str() : "-", _("Bitwise OR"));
		} else if(i == 25 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2B("label_ac", _("Clear"), _("MC (memory clear)"))
		else if(i == 26 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3B("label_del", _("Delete"), _("Backspace"), _("M− (memory minus)"))
		else if(i == 27 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_2B("label_ans", _("Previous result"), _("Previous result (static)"))
		else if(i == 28 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_3("=", _("Calculate expression"), _("MR (memory recall)"), _("MS (memory store)"))
		else if(i == 29 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C1")
		else if(i == 30 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C2")
		else if(i == 31 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C3")
		else if(i == 32 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C4")
		else if(i == 33 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C5")
		else if(i == 34 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C6")
		else if(i == 35 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C7")
		else if(i == 36 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C8")
		else if(i == 37 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C9")
		else if(i == 38 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C10")
		else if(i == 39 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C11")
		else if(i == 40 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C12")
		else if(i == 41 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C13")
		else if(i == 42 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C14")
		else if(i == 43 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C15")
		else if(i == 44 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C16")
		else if(i == 45 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C17")
		else if(i == 46 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C18")
		else if(i == 47 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C19")
		else if(i == 48 && (index == i || index < 0)) SET_BUTTONS_EDIT_ITEM_C("C20")
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tButtonsEdit_store), &iter));
	on_tButtonsEdit_update_selection(gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEdit)), update_label_entry);
}
GtkWidget* get_buttons_edit_dialog(void) {
	if(!buttonsedit_builder) {

		buttonsedit_builder = getBuilder("buttonsedit.ui");
		g_assert(buttonsedit_builder != NULL);

		g_assert(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_dialog") != NULL);

		tButtonsEdit = GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_treeview"));

		tButtonsEdit_store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tButtonsEdit), GTK_TREE_MODEL(tButtonsEdit_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEdit));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Label"), renderer, "text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEdit), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Left-click"), renderer, "text", 2, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEdit), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Right-click"), renderer, "text", 3, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEdit), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Middle-click"), renderer, "text", 4, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEdit), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tButtonsEdit_selection_changed), NULL);

		GtkTreeIter iter;
		for(int i = 29; i <= 33; i++) {
			gtk_list_store_append(tButtonsEdit_store, &iter);
			gtk_list_store_set(tButtonsEdit_store, &iter, 0, i, -1);
		}
		for(int i = 0; i < 20; i++) {
			gtk_list_store_append(tButtonsEdit_store, &iter);
			gtk_list_store_set(tButtonsEdit_store, &iter, 0, i, -1);
		}
		for(int i = 34; i <= 48; i++) {
			gtk_list_store_append(tButtonsEdit_store, &iter);
			gtk_list_store_set(tButtonsEdit_store, &iter, 0, i, -1);
		}
		gtk_tree_selection_unselect_all(selection);

		update_custom_buttons_edit();

		tButtonsEditType = GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "shortcuts_type_treeview"));

		tButtonsEditType_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tButtonsEditType), GTK_TREE_MODEL(tButtonsEditType_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tButtonsEditType));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Action"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tButtonsEditType), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tButtonsEditType_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tButtonsEditType_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);

		gtk_list_store_append(tButtonsEditType_store, &iter);
		gtk_list_store_set(tButtonsEditType_store, &iter, 0, _("None"), 1, -2, -1);
		for(int i = 0; i <= SHORTCUT_TYPE_QUIT; i++) {
			gtk_list_store_append(tButtonsEditType_store, &iter);
			gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(i), 1, i, -1);
			if(i == SHORTCUT_TYPE_RPN_MODE) {
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_CHAIN_MODE), 1, SHORTCUT_TYPE_CHAIN_MODE, -1);
			} else if(i == SHORTCUT_TYPE_COPY_RESULT) {
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_INSERT_RESULT), 1, SHORTCUT_TYPE_INSERT_RESULT, -1);
			} else if(i == SHORTCUT_TYPE_HISTORY_SEARCH) {
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_HISTORY_CLEAR), 1, SHORTCUT_TYPE_HISTORY_CLEAR, -1);
			} else if(i == SHORTCUT_TYPE_MINIMAL) {
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_ALWAYS_ON_TOP), 1, SHORTCUT_TYPE_ALWAYS_ON_TOP, -1);
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_DO_COMPLETION), 1, SHORTCUT_TYPE_DO_COMPLETION, -1);
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION), 1, SHORTCUT_TYPE_ACTIVATE_FIRST_COMPLETION, -1);
			} else if(i == SHORTCUT_TYPE_SIMPLE_NOTATION) {
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_PRECISION), 1, SHORTCUT_TYPE_PRECISION, -1);
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_MIN_DECIMALS), 1, SHORTCUT_TYPE_MIN_DECIMALS, -1);
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_MAX_DECIMALS), 1, SHORTCUT_TYPE_MAX_DECIMALS, -1);
				gtk_list_store_append(tButtonsEditType_store, &iter);
				gtk_list_store_set(tButtonsEditType_store, &iter, 0, shortcut_type_text(SHORTCUT_TYPE_MINMAX_DECIMALS), 1, SHORTCUT_TYPE_MINMAX_DECIMALS, -1);
			}
			if(i == 0) gtk_tree_selection_select_iter(selection, &iter);
		}

		gtk_builder_add_callback_symbols(buttonsedit_builder, "on_buttons_edit_entry_label_changed", G_CALLBACK(on_buttons_edit_entry_label_changed), "on_buttons_edit_button_1_clicked", G_CALLBACK(on_buttons_edit_button_1_clicked), "on_buttons_edit_button_2_clicked", G_CALLBACK(on_buttons_edit_button_2_clicked), "on_buttons_edit_button_3_clicked", G_CALLBACK(on_buttons_edit_button_3_clicked), "on_buttons_edit_button_defaults_clicked", G_CALLBACK(on_buttons_edit_button_defaults_clicked), "on_buttons_edit_treeview_row_activated", G_CALLBACK(on_buttons_edit_treeview_row_activated), "on_buttonsedit_type_treeview_row_activated", G_CALLBACK(on_buttonsedit_type_treeview_row_activated), "on_buttonsedit_entry_value_activate", G_CALLBACK(on_buttonsedit_entry_value_activate), NULL);
		gtk_builder_connect_signals(buttonsedit_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_dialog"));
}

void edit_buttons(GtkWindow *parent) {
	bool set_sr = !buttonsedit_builder;
	GtkWidget *dialog = get_buttons_edit_dialog();
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(buttonsedit_builder, "buttons_edit_treeview")));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_widget_show(dialog);
	if(set_sr) {
		gint w;
		gtk_window_get_size(GTK_WINDOW(dialog), &w, NULL);
		gtk_widget_set_size_request(dialog, w, -1);
	}
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}
