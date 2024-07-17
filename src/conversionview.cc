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
#include <cairo/cairo-gobject.h>

#include "support.h"
#include "settings.h"
#include "util.h"
#include "conversionview.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

#include "unordered_map_define.h"

extern GtkBuilder *main_builder;

GtkWidget *tUnitSelectorCategories;
GtkWidget *tUnitSelector;
GtkListStore *tUnitSelector_store;
GtkTreeModel *tUnitSelector_store_filter;
GtkTreeStore *tUnitSelectorCategories_store;
GtkTreeViewColumn *flag_column;

string selected_unit_selector_category;
bool block_unit_selector_convert = false;
int block_conversion_category_switch = 0;
bool keep_unit_selection = false;
unordered_map<string, GtkTreeIter> convert_category_map;

extern unordered_map<string, cairo_surface_t*> flag_surfaces;

void on_convert_entry_search_changed(GtkEntry *w, gpointer);
void on_tUnitSelector_selection_changed(GtkTreeSelection *treeselection, gpointer);
void on_tUnitSelectorCategories_selection_changed(GtkTreeSelection *treeselection, gpointer);
void convert_from_convert_entry_unit();

void update_unit_selector_tree() {
	GtkTreeIter iter, iter2, iter3;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tUnitSelectorCategories));
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelectorCategories_selection_changed, NULL);
	gtk_tree_store_clear(tUnitSelectorCategories_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelectorCategories_selection_changed, NULL);
	gtk_tree_store_append(tUnitSelectorCategories_store, &iter3, NULL);
	gtk_tree_store_set(tUnitSelectorCategories_store, &iter3, 0, _("All"), 1, _("All"), -1);
	string str;
	tree_struct *item, *item2;
	unit_cats.it = unit_cats.items.begin();
	if(unit_cats.it != unit_cats.items.end()) {
		item = &*unit_cats.it;
		++unit_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	iter2 = iter3;
	convert_category_map.clear();
	while(item) {
		gtk_tree_store_append(tUnitSelectorCategories_store, &iter, &iter2);
		if(!str.empty()) str += "/";
		str += item->item;
		gtk_tree_store_set(tUnitSelectorCategories_store, &iter, 0, item->item.c_str(), 1, str.c_str(), -1);
		if(str == selected_unit_selector_category) {
			EXPAND_TO_ITER(model, tUnitSelectorCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
		}
		convert_category_map[str] = iter;
		while(item && item->it == item->items.end()) {
			size_t str_i = str.rfind("/");
			if(str_i == string::npos) {
				str = "";
			} else {
				str = str.substr(0, str_i);
			}
			item = item->parent;
			gtk_tree_model_iter_parent(model, &iter2, &iter);
			iter = iter2;
		}
		if(item) {
			item2 = &*item->it;
			if(item->it == item->items.begin()) iter2 = iter;
			++item->it;
			item = item2;
			item->it = item->items.begin();
		}
	}
	EXPAND_ITER(model, tUnitSelectorCategories, iter3)
	if(!unit_cats.objects.empty()) {
		//add "Uncategorized" category if there are units without category
		gtk_tree_store_append(tUnitSelectorCategories_store, &iter, &iter3);
		gtk_tree_store_set(tUnitSelectorCategories_store, &iter, 0, _("Uncategorized"), 1, _("Uncategorized"), -1);
		convert_category_map[_("Uncategorized")] = iter;
		if(selected_unit_selector_category == _("Uncategorized")) {
			EXPAND_TO_ITER(model, tUnitSelectorCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
		}
	}
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &model, &iter)) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_unit_selector_category = _("All");
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter3);
	}
}

void setUnitSelectorTreeItem(GtkTreeIter &iter2, Unit *u) {
	gtk_list_store_append(tUnitSelector_store, &iter2);
	string snames, sbase;
	if(u->isCurrency()) {
		unordered_map<string, cairo_surface_t*>::const_iterator it_flag = flag_surfaces.find(u->referenceName());
		gtk_list_store_set(tUnitSelector_store, &iter2, 0, u->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tUnitSelector).c_str(), 1, (gpointer) u, 2, it_flag == flag_surfaces.end() ? NULL : it_flag->second, 3, TRUE, -1);
	} else {
		gtk_list_store_set(tUnitSelector_store, &iter2, 0, u->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tUnitSelector).c_str(), 1, (gpointer) u, 3, TRUE, -1);
	}
}

/*
	generate the unit tree in conversion tab when category selection has changed
*/
void on_tUnitSelectorCategories_selection_changed(GtkTreeSelection *treeselection, gpointer) {

	block_unit_selector_convert = true;

	GtkTreeModel *model;
	GtkTreeIter iter, iter2;

	bool no_cat = false, b_all = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_convert_entry_search_changed, NULL);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_search")), "");
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_convert_entry_search_changed, NULL);
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelector_selection_changed, NULL);
	gtk_list_store_clear(tUnitSelector_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelector_selection_changed, NULL);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, -1);
		selected_unit_selector_category = gstr;
		if(selected_unit_selector_category == _("All")) {
			b_all = true;
		} else if(selected_unit_selector_category == _("Uncategorized")) {
			no_cat = true;
		}
		bool b_currencies = false;
		if(!b_all && !no_cat && selected_unit_selector_category[0] == '/') {
			string str = selected_unit_selector_category.substr(1, selected_unit_selector_category.length() - 1);
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				if(CALCULATOR->units[i]->isActive() && (!CALCULATOR->units[i]->isHidden() || CALCULATOR->units[i]->isCurrency()) && CALCULATOR->units[i]->category().substr(0, selected_unit_selector_category.length() - 1) == str) {
					if(!b_currencies && CALCULATOR->units[i]->isCurrency()) b_currencies = true;
					setUnitSelectorTreeItem(iter2, CALCULATOR->units[i]);
				}
			}
		} else {
			bool list_empty = true;
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				if(CALCULATOR->units[i]->isActive() && (!CALCULATOR->units[i]->isHidden() || CALCULATOR->units[i]->isCurrency()) && (b_all || (no_cat && CALCULATOR->units[i]->category().empty()) || CALCULATOR->units[i]->category() == selected_unit_selector_category)) {
					if(!b_currencies && !b_all && !no_cat && CALCULATOR->units[i]->isCurrency()) b_currencies = true;
					setUnitSelectorTreeItem(iter2, CALCULATOR->units[i]);
					list_empty = false;
				}
			}
			bool collapse_all = true;
			if(list_empty && !b_all && !no_cat) {
				for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
					if(CALCULATOR->units[i]->isActive() && (!CALCULATOR->units[i]->isHidden() || CALCULATOR->units[i]->isCurrency()) && CALCULATOR->units[i]->category().length() > selected_unit_selector_category.length() && CALCULATOR->units[i]->category()[selected_unit_selector_category.length()] == '/' && CALCULATOR->units[i]->category().substr(0, selected_unit_selector_category.length()) == selected_unit_selector_category) {
						if(!b_currencies && !b_all && !no_cat && CALCULATOR->units[i]->isCurrency()) b_currencies = true;
						setUnitSelectorTreeItem(iter2, CALCULATOR->units[i]);
					}
				}
			} else if(!b_all && !no_cat) {
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				collapse_all = !gtk_tree_view_expand_row(GTK_TREE_VIEW(tUnitSelectorCategories), path, FALSE);
				gtk_tree_path_free(path);
			}
			if(collapse_all) {
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				if(gtk_tree_path_get_depth(path) == 2) {
					GtkTreeIter iter3;
					gtk_tree_model_get_iter_first(model, &iter3);
					if(gtk_tree_model_iter_children(model, &iter2, &iter3)) {
						do {
							GtkTreePath *path2 = gtk_tree_model_get_path(model, &iter2);
							if(gtk_tree_path_compare(path, path2) != 0) gtk_tree_view_collapse_row(GTK_TREE_VIEW(tUnitSelectorCategories), path2);
							gtk_tree_path_free(path2);
						} while(gtk_tree_model_iter_next(model, &iter2));
					}
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tUnitSelectorCategories), path, NULL, FALSE, 0, 0);
				}
				gtk_tree_path_free(path);
			}
		}
		gtk_tree_view_column_set_visible(flag_column, b_currencies);
		g_free(gstr);
	} else {
		selected_unit_selector_category = "";
	}
	block_unit_selector_convert = false;

}

void on_tUnitSelector_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		Unit *u;
		gtk_tree_model_get(model, &iter, 1, &u, -1);
		keep_unit_selection = true;
		if(CALCULATOR->stillHasUnit(u)) {
			if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				PrintOptions po = printops;
				po.is_approximate = NULL;
				po.can_display_unicode_string_arg = (void*) gtk_builder_get_object(main_builder, "convert_entry_unit");
				po.abbreviate_names = true;
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit")), ((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true, false).c_str());
			} else {
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit")), u->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(main_builder, "convert_entry_unit")).formattedName(TYPE_UNIT, true).c_str());
			}
			if(!block_unit_selector_convert) convert_from_convert_entry_unit();
		}
		keep_unit_selection = false;
	} else {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit")), "");
	}
}

void on_convert_treeview_category_row_expanded(GtkTreeView *tree_view, GtkTreeIter*, GtkTreePath *path, gpointer) {
	if(gtk_tree_path_get_depth(path) != 2) return;
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
	GtkTreeIter iter2, iter3;
	gtk_tree_model_get_iter_first(model, &iter3);
	if(gtk_tree_model_iter_children(model, &iter2, &iter3)) {
		do {
			GtkTreePath *path2 = gtk_tree_model_get_path(model, &iter2);
			if(gtk_tree_path_compare(path, path2) != 0) gtk_tree_view_collapse_row(GTK_TREE_VIEW(tUnitSelectorCategories), path2);
			gtk_tree_path_free(path2);
		} while(gtk_tree_model_iter_next(model, &iter2));
	}
	gtk_tree_view_scroll_to_cell(tree_view, path, NULL, FALSE, 0, 0);
}
void on_convert_entry_unit_changed(GtkEditable *w, gpointer) {
	bool b = gtk_entry_get_text_length(GTK_ENTRY(w)) > 0;
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, b ? "edit-clear-symbolic" : NULL);
	gtk_entry_set_icon_tooltip_text(GTK_ENTRY(w), GTK_ENTRY_ICON_SECONDARY, b ? _("Clear expression") : NULL);
	if(enable_tooltips != 1) gtk_widget_set_has_tooltip(GTK_WIDGET(w), FALSE);
	if(!keep_unit_selection) gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector)));
}
Unit *popup_convert_unit = NULL;
void update_convert_popup() {
	GtkTreeIter iter_sel;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	GtkTreeModel *model;
	Unit *u_sel = popup_convert_unit;
	if(!u_sel && gtk_tree_selection_get_selected(select, &model, &iter_sel)) gtk_tree_model_get(model, &iter_sel, 1, &u_sel, -1);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_convert_insert")), u_sel != NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(main_builder, "popup_menu_convert_convert")), u_sel != NULL);
}
void on_popup_menu_convert_insert_activate(GtkMenuItem*, gpointer) {
	GtkTreeIter iter_sel;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	GtkTreeModel *model;
	Unit *u = popup_convert_unit;
	if(!u && gtk_tree_selection_get_selected(select, &model, &iter_sel)) gtk_tree_model_get(model, &iter_sel, 1, &u, -1);
	insert_unit(u, true);
}
void on_popup_menu_convert_convert_activate(GtkMenuItem*, gpointer) {
	GtkTreeIter iter_sel;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	GtkTreeModel *model;
	Unit *u = popup_convert_unit;
	if(!u && gtk_tree_selection_get_selected(select, &model, &iter_sel)) gtk_tree_model_get(model, &iter_sel, 1, &u, -1);
	if(u) {
		keep_unit_selection = true;
		for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
			if(CALCULATOR->units[i] == u) {
				if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
					PrintOptions po = printops;
					po.is_approximate = NULL;
					po.can_display_unicode_string_arg = (void*) gtk_builder_get_object(main_builder, "convert_entry_unit");
					gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit")), ((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true, false).c_str());
				} else {
					gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit")), u->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(main_builder, "convert_entry_unit")).formattedName(TYPE_UNIT, true).c_str());
				}
				if(!block_unit_selector_convert) convert_from_convert_entry_unit();
			}
		}
		keep_unit_selection = false;
	}
}
gboolean on_convert_treeview_unit_button_press_event(GtkWidget *w, GdkEventButton *event, gpointer) {
	GtkTreePath *path = NULL;
	if(event->type == GDK_BUTTON_PRESS && event->button == 2) {
		if(b_busy) return TRUE;
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(w), event->x, event->y, &path, NULL, NULL, NULL)) {
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter(tUnitSelector_store_filter, &iter, path)) {
				Unit *u = NULL;
				gtk_tree_model_get(tUnitSelector_store_filter, &iter, 1, &u, -1);
				insert_unit(u, true);
				gtk_tree_path_free(path);
				return TRUE;
			}
			gtk_tree_path_free(path);
		}
	} else if(gdk_event_triggers_context_menu((GdkEvent*) event) && event->type == GDK_BUTTON_PRESS) {
		if(b_busy) return TRUE;
		if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(w), event->x, event->y, &path, NULL, NULL, NULL)) {
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter(tUnitSelector_store_filter, &iter, path)) {
				gtk_tree_model_get(tUnitSelector_store_filter, &iter, 1, &popup_convert_unit, -1);
			} else {
				popup_convert_unit = NULL;
			}
			gtk_tree_path_free(path);
		}

		update_convert_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
		gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_convert")), (GdkEvent*) event);
#else
		gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_convert")), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
		return TRUE;
	}
	return FALSE;
}
gboolean on_convert_treeview_unit_popup_menu(GtkWidget*, gpointer) {
	if(b_busy) return TRUE;
	popup_convert_unit = NULL;
	update_convert_popup();
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	gtk_menu_popup_at_pointer(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_convert")), NULL);
#else
	gtk_menu_popup(GTK_MENU(gtk_builder_get_object(main_builder, "popup_menu_convert")), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
#endif
	return TRUE;
}
void on_convert_entry_search_changed(GtkEntry *w, gpointer) {
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelector_selection_changed, NULL);
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitSelector_store), &iter)) return;
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	do {
		bool b = str.empty();
		Unit *u = NULL;
		if(!b) gtk_tree_model_get(GTK_TREE_MODEL(tUnitSelector_store), &iter, 1, &u, -1);
		if(u) {
			b = name_matches(u, str) || title_matches(u, str) || country_matches(u, str);
		}
		gtk_list_store_set(tUnitSelector_store, &iter, 3, b, -1);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tUnitSelector_store), &iter));
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitSelector_selection_changed, NULL);
	if(!str.empty()) {
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitSelector_store_filter), &iter)) {
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector)));
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector)), &iter);
			GtkTreePath *path = gtk_tree_model_get_path(tUnitSelector_store_filter, &iter);
			if(path) {
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(tUnitSelector), path, NULL, FALSE);
				gtk_tree_path_free(path);
			}
		}
		gint start_pos = 0, end_pos = 0;
		gtk_editable_get_selection_bounds(GTK_EDITABLE(w), &start_pos, &end_pos);
		gtk_widget_grab_focus(GTK_WIDGET(w));
		gtk_editable_select_region(GTK_EDITABLE(w), start_pos, end_pos);
	}
}
void convert_from_convert_entry_unit() {
	block_conversion_category_switch++;
	string ceu_str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit")));
	if(set_missing_prefixes && !ceu_str.empty()) {
		remove_blank_ends(ceu_str);
		if(!ceu_str.empty() && ceu_str[0] != '0' && ceu_str[0] != '?' && ceu_str[0] != '+' && ceu_str[0] != '-' && (ceu_str.length() == 1 || ceu_str[1] != '?')) {
			ceu_str = "?" + ceu_str;
		}
	}
	convert_result_to_unit_expression(ceu_str);
	block_conversion_category_switch--;
}
void on_convert_button_set_missing_prefixes_toggled(GtkToggleButton *w, gpointer) {
	set_missing_prefixes = gtk_toggle_button_get_active(w);
	if(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit"))) != 0) {
		convert_from_convert_entry_unit();
	}
}
void on_convert_button_continuous_conversion_toggled(GtkToggleButton *w, gpointer) {
	continuous_conversion = gtk_toggle_button_get_active(w);
}
void on_convert_button_convert_clicked(GtkButton*, gpointer) {
	convert_from_convert_entry_unit();
	focus_keeping_selection();
}
void on_convert_entry_unit_activate(GtkEntry*, gpointer) {
	convert_from_convert_entry_unit();
	focus_keeping_selection();
}
void on_convert_entry_unit_icon_release(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent*, gpointer) {
	switch(icon_pos) {
		case GTK_ENTRY_ICON_PRIMARY: {
			break;
		}
		case GTK_ENTRY_ICON_SECONDARY: {
			gtk_editable_delete_text(GTK_EDITABLE(entry), 0, -1);
			break;
		}
	}
}
void update_conversion_view_selection(const MathStructure *m) {
	if(!block_conversion_category_switch) {
		Unit *u = CALCULATOR->findMatchingUnit(*m);
		if(u && !u->category().empty()) {
			string s_cat = u->category();
			for(size_t i = 0; i < alt_volcats.size(); i++) {
				if(s_cat == alt_volcats[i]) {s_cat = volume_cat; break;}
			}
			if(s_cat.empty()) s_cat = _("Uncategorized");
			if(s_cat != selected_unit_selector_category) {
				GtkTreeIter iter = convert_category_map[s_cat];
				GtkTreePath *path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
				gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tUnitSelectorCategories), path);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tUnitSelectorCategories), path, NULL, TRUE, 0.5, 0);
				gtk_tree_path_free(path);
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories)), &iter);
			}
		}
		if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")))) {
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector)));
		}
	}
}
void focus_conversion_entry() {
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_entry_unit")));
}
const gchar *current_conversion_expression() {
	return gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(main_builder, "convert_entry_unit")));
}

void create_conversion_view() {
	tUnitSelectorCategories = GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_category"));
	tUnitSelector = GTK_WIDGET(gtk_builder_get_object(main_builder, "convert_treeview_unit"));

	tUnitSelector_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_POINTER, CAIRO_GOBJECT_TYPE_SURFACE, G_TYPE_BOOLEAN);
	tUnitSelector_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tUnitSelector_store), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tUnitSelector_store_filter), 3);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnitSelector_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnitSelector_store), 0, GTK_SORT_ASCENDING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tUnitSelector), GTK_TREE_MODEL(tUnitSelector_store_filter));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelector));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_padding(renderer, 4, 0);
	flag_column = gtk_tree_view_column_new_with_attributes(_("Flag"), renderer, "surface", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tUnitSelector), flag_column);
	renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 0, NULL);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tUnitSelector), column);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tUnitSelector_selection_changed), NULL);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tUnitSelector), FALSE);

	tUnitSelectorCategories_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tUnitSelectorCategories), GTK_TREE_MODEL(tUnitSelectorCategories_store));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitSelectorCategories));
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tUnitSelectorCategories), column);
	g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tUnitSelectorCategories_selection_changed), NULL);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnitSelectorCategories_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnitSelectorCategories_store), 0, GTK_SORT_ASCENDING);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_continuous_conversion")), continuous_conversion);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(main_builder, "convert_button_set_missing_prefixes")), set_missing_prefixes);

	gtk_builder_add_callback_symbols(main_builder, "on_convert_entry_unit_activate", G_CALLBACK(on_convert_entry_unit_activate), "on_convert_entry_unit_changed", G_CALLBACK(on_convert_entry_unit_changed), "on_convert_entry_unit_icon_release", G_CALLBACK(on_convert_entry_unit_icon_release), "on_convert_treeview_category_row_expanded", G_CALLBACK(on_convert_treeview_category_row_expanded), "on_convert_treeview_unit_button_press_event", G_CALLBACK(on_convert_treeview_unit_button_press_event), "on_convert_treeview_unit_popup_menu", G_CALLBACK(on_convert_treeview_unit_popup_menu), "on_convert_entry_search_changed", G_CALLBACK(on_convert_entry_search_changed), "on_convert_button_convert_clicked", G_CALLBACK(on_convert_button_convert_clicked), "on_convert_button_continuous_conversion_toggled", G_CALLBACK(on_convert_button_continuous_conversion_toggled), "on_convert_button_set_missing_prefixes_toggled", G_CALLBACK(on_convert_button_set_missing_prefixes_toggled), "on_popup_menu_convert_insert_activate", G_CALLBACK(on_popup_menu_convert_insert_activate), "on_popup_menu_convert_convert_activate", G_CALLBACK(on_popup_menu_convert_convert_activate), NULL);
}
