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
#include "expressionedit.h"
#include "uniteditdialog.h"
#include "unitsdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

enum {
	UNITS_TITLE_COLUMN,
	UNITS_POINTER_COLUMN,
	UNITS_FLAG_COLUMN,
	UNITS_VISIBLE_COLUMN,
	UNITS_VISIBLE_COLUMN_CONVERT,
	UNITS_N_COLUMNS
};

GtkBuilder *units_builder = NULL;
GtkWidget *tUnitCategories;
GtkWidget *tUnits;
GtkListStore *tUnits_store;
GtkTreeModel *tUnits_store_filter, *units_convert_filter;
GtkTreeStore *tUnitCategories_store;
GtkTreeViewColumn *units_flag_column;
GtkWidget *units_convert_view, *units_convert_window, *units_convert_scrolled;
GtkCellRenderer *units_convert_flag_renderer;

bool block_unit_convert = false;
string old_fromValue, old_toValue;
string selected_unit_category;
Unit *selected_unit, *selected_to_unit;

gint units_width = -1, units_height = -1, units_hposition = -1, units_vposition = -1;

void on_tUnits_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);
void on_tUnitCategories_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);
void on_units_convert_view_row_activated(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn*, gpointer);
void on_units_entry_search_changed(GtkEntry *w, gpointer);
void on_units_convert_search_changed(GtkEntry *w, gpointer);
void convert_in_wUnits(int toFrom = -1);

Unit *get_selected_unit() {
	return selected_unit;
}
Unit *get_selected_to_unit() {
	return selected_to_unit;
}
void update_units_tree() {
	if(!units_builder) return;
	GtkTreeIter iter, iter2, iter3;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tUnitCategories));
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitCategories_selection_changed, NULL);
	gtk_tree_store_clear(tUnitCategories_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnitCategories_selection_changed, NULL);
	gtk_tree_store_append(tUnitCategories_store, &iter3, NULL);
	gtk_tree_store_set(tUnitCategories_store, &iter3, 0, _("All"), 1, _("All"), -1);
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
	while(item) {
		gtk_tree_store_append(tUnitCategories_store, &iter, &iter2);
		str += "/";
		str += item->item;
		gtk_tree_store_set(tUnitCategories_store, &iter, 0, item->item.c_str(), 1, str.c_str(), -1);
		if(str == selected_unit_category) {
			EXPAND_TO_ITER(model, tUnitCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
		}
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
	EXPAND_ITER(model, tUnitCategories, iter3)
	if(!user_units.empty()) {
		gtk_tree_store_append(tUnitCategories_store, &iter, NULL);
		EXPAND_TO_ITER(model, tUnitCategories, iter)
		gtk_tree_store_set(tUnitCategories_store, &iter, 0, _("User units"), 1, _("User units"), -1);
		if(selected_unit_category == _("User units")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
		}
	}
	if(!unit_cats.objects.empty()) {
		//add "Uncategorized" category if there are units without category
		gtk_tree_store_append(tUnitCategories_store, &iter, &iter3);
		gtk_tree_store_set(tUnitCategories_store, &iter, 0, _("Uncategorized"), 1, _("Uncategorized"), -1);
		if(selected_unit_category == _("Uncategorized")) {
			EXPAND_TO_ITER(model, tUnitCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
		}
	}
	if(!ia_units.empty()) {
		gtk_tree_store_append(tUnitCategories_store, &iter, NULL);
		gtk_tree_store_set(tUnitCategories_store, &iter, 0, _("Inactive"), 1, _("Inactive"), -1);
		if(selected_unit_category == _("Inactive")) {
			EXPAND_TO_ITER(model, tUnitCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
		}
	}
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &model, &iter)) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_unit_category = _("All");
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter3);
	}
}

void setUnitTreeItem(GtkTreeIter &iter2, Unit *u) {
	gtk_list_store_append(tUnits_store, &iter2);
	//display descriptive name (title), or name if no title defined
	gtk_list_store_set(tUnits_store, &iter2, UNITS_TITLE_COLUMN, u->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tUnits).c_str(), UNITS_POINTER_COLUMN, (gpointer) u, UNITS_VISIBLE_COLUMN, TRUE, UNITS_VISIBLE_COLUMN_CONVERT, TRUE, -1);
	if(u->isCurrency()) {
		unordered_map<string, cairo_surface_t*>::const_iterator it_flag = flag_surfaces.find(u->referenceName());
		if(it_flag != flag_surfaces.end()) {
			gtk_list_store_set(tUnits_store, &iter2, UNITS_FLAG_COLUMN, it_flag->second, -1);
		}
	}
	GtkTreeIter iter;
	if(u == selected_unit && gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(tUnits_store_filter), &iter, &iter2)) {
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &iter);
	}
}

void on_tUnitCategories_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model, *model2;
	GtkTreeIter iter, iter2;
	//make sure that no unit conversion is done in the dialog until everthing is updated
	block_unit_convert = true;
	bool no_cat = false, b_all = false, b_inactive = false, b_user = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_units_entry_search_changed, NULL);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), "");
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_units_entry_search_changed, NULL);
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_units_convert_search_changed, NULL);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_convert_search")), "");
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_units_convert_search_changed, NULL);
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnits_selection_changed, NULL);
	gtk_list_store_clear(tUnits_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnits_selection_changed, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_edit")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_insert")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_delete")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_deactivate")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_convert_to")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_frame_convert")), FALSE);
	bool b_sel = false;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		bool b_currencies = false;
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, -1);
		selected_unit_category = gstr;
		if(selected_unit_category == _("All")) {
			b_all = true;
		} else if(selected_unit_category == _("Uncategorized")) {
			no_cat = true;
		} else if(selected_unit_category == _("Inactive")) {
			b_inactive = true;
		} else if(selected_unit_category == _("User units")) {
			b_user = true;
		}
		if(!b_user && !b_all && !no_cat && !b_inactive && selected_unit_category[0] == '/') {
			string str = selected_unit_category.substr(1, selected_unit_category.length() - 1);
			ExpressionItem *o;
			size_t l1 = str.length(), l2;
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				o = CALCULATOR->units[i];
				l2 = o->category().length();
				if(o->isActive() && (l2 == l1 || (l2 > l1 && o->category()[l1] == '/')) && o->category().substr(0, l1) == str) {
					if(CALCULATOR->units[i]->isCurrency()) b_currencies = true;
					setUnitTreeItem(iter2, CALCULATOR->units[i]);
					if(!b_sel && selected_to_unit == CALCULATOR->units[i]) b_sel = true;
				}
			}
		} else {
			for(size_t i = 0; i < CALCULATOR->units.size(); i++) {
				if((b_inactive && !CALCULATOR->units[i]->isActive()) || (CALCULATOR->units[i]->isActive() && (b_all || (no_cat && CALCULATOR->units[i]->category().empty()) || (b_user && CALCULATOR->units[i]->isLocal()) || (!b_inactive && CALCULATOR->units[i]->category() == selected_unit_category)))) {
					if(!b_all && !no_cat && !b_inactive && !b_currencies && CALCULATOR->units[i]->isCurrency()) b_currencies = true;
					setUnitTreeItem(iter2, CALCULATOR->units[i]);
					if(!b_sel && selected_to_unit == CALCULATOR->units[i]) b_sel = true;
				}
			}
		}
		if(!selected_unit || !gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &model2, &iter2)) {
			if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnits_store_filter), &iter2)) {
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &iter2);
			}
		}
		gtk_tree_view_column_set_visible(units_flag_column, b_currencies);
		gtk_cell_renderer_set_visible(units_convert_flag_renderer, b_currencies);
		g_free(gstr);
	} else {
		selected_unit_category = "";
	}
	if(!b_sel) {
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(units_convert_filter), &iter2)) {
			GtkTreePath *path = gtk_tree_model_get_path(units_convert_filter, &iter2);
			on_units_convert_view_row_activated(GTK_TREE_VIEW(units_convert_view), path, NULL, NULL);
			gtk_tree_path_free(path);
		}
	}
	block_unit_convert = false;
	//update conversion display
	convert_in_wUnits();
}

void on_tUnits_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		Unit *u;
		gtk_tree_model_get(model, &iter, UNITS_POINTER_COLUMN, &u, -1);
		selected_unit = u;
		if(CALCULATOR->stillHasUnit(u)) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(units_builder, "units_textview_description")));
			gtk_text_buffer_set_text(buffer, "", -1);
			GtkTextIter iter;
			string str, str2;
			if(u->subtype() != SUBTYPE_COMPOSITE_UNIT) {
				const ExpressionName *ename = &u->preferredName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(units_builder, "units_textview_description"));
				str += ename->formattedName(TYPE_UNIT, true);
				gtk_text_buffer_get_end_iter(buffer, &iter);
				gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
				str = "";
				for(size_t i2 = 1; i2 <= u->countNames(); i2++) {
					if(&u->getName(i2) != ename) {
						str += ", ";
						str += u->getName(i2).formattedName(TYPE_UNIT, true);
					}
				}
				str += "\n\n";
			}
			bool is_approximate = false;
			PrintOptions po = printops;
			po.can_display_unicode_string_arg = (void*) gtk_builder_get_object(units_builder, "units_textview_description");
			po.is_approximate = &is_approximate;
			po.allow_non_usable = true;
			po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
			po.base = 10;
			po.number_fraction_format = FRACTION_DECIMAL_EXACT;
			po.use_unit_prefixes = false;
			if(u->subtype() == SUBTYPE_ALIAS_UNIT) {
				AliasUnit *au = (AliasUnit*) u;
				MathStructure m(1, 1, 0), mexp(1, 1, 0);
				if(au->hasNonlinearExpression()) {
					m.set("x");
					if(au->expression().find("\\y") != string::npos) mexp.set("y");
					str += "x ";
					str += u->preferredDisplayName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(units_builder, "units_textview_description")).formattedName(TYPE_UNIT, true);
					if(au->expression().find("\\y") != string::npos) str += "^y";
					str += " ";
				}
				au->convertToFirstBaseUnit(m, mexp);
				if(au->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) m.multiply(((CompositeUnit*) au->firstBaseUnit())->generateMathStructure());
				else m.multiply(au->firstBaseUnit());
				if(!mexp.isOne()) m.last() ^= mexp;
				if(m.isApproximate() || is_approximate) str += SIGN_ALMOST_EQUAL " ";
				else str += "= ";
				m.format(po);
				str += m.print(po);
				if(au->hasNonlinearExpression() && !au->inverseExpression().empty()) {
					str += "\n";
					m.set("x");
					if(au->inverseExpression().find("\\y") != string::npos) mexp.set("y");
					else mexp.set(1, 1, 0);
					str += "x ";
					bool b_y = au->inverseExpression().find("\\y") != string::npos;
					if(au->firstBaseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
						if(b_y) str += "(";
						MathStructure m2(((CompositeUnit*) au->firstBaseUnit())->generateMathStructure());
						m2.format(po);
						str += m2.print(po);
						if(b_y) str += ")^y";
					} else {
						str += au->firstBaseUnit()->preferredDisplayName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(units_builder, "units_textview_description")).formattedName(TYPE_UNIT, true);
						if(b_y) str += "^y";
					}
					str += " ";
					au->convertFromFirstBaseUnit(m, mexp);
					m.multiply(au);
					if(!mexp.isOne()) m.last() ^= mexp;
					if(m.isApproximate() || is_approximate) str += SIGN_ALMOST_EQUAL " ";
					else str += "= ";
					m.format(po);
					str += m.print(po);
				}
			} else if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				str += "= ";
				MathStructure m(((CompositeUnit*) u)->generateMathStructure());
				m.format(po);
				str += m.print(po);
			}
			if(!u->description().empty()) {
				str += "\n\n";
				str += u->description();
			}
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_frame_convert")), TRUE);
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_from_unit")), u->print(true, printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) gtk_builder_get_object(units_builder, "units_label_from_unit")).c_str());
			//user cannot delete global definitions
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_delete")), u->isLocal());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_convert_to")), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_insert")), u->isActive());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_edit")), !u->isBuiltin());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_deactivate")), TRUE);
			if(u->isActive()) {
				gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(units_builder, "units_buttonlabel_deactivate")), _("Deacti_vate"));
			} else {
				gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(units_builder, "units_buttonlabel_deactivate")), _("Acti_vate"));
			}
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_edit")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_insert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_delete")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_deactivate")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_button_convert_to")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_frame_convert")), FALSE);
		selected_unit = NULL;
	}
	if(!block_unit_convert) convert_in_wUnits();
}

void convert_in_wUnits(int toFrom) {
	//units
	Unit *uFrom = get_selected_unit();
	Unit *uTo = get_selected_to_unit();

	if(uFrom && uTo) {
		//values
		const gchar *fromValue = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")));
		const gchar *toValue = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")));
		old_fromValue = fromValue;
		old_toValue = toValue;
		//determine conversion direction
		bool b = false;
		if(toFrom > 0) {
			if(CALCULATOR->timedOutString() == toValue) return;
			if(uFrom == uTo) {
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")), toValue);
			} else {
				EvaluationOptions eo;
				eo.approximation = APPROXIMATION_APPROXIMATE;
				eo.parse_options = evalops.parse_options;
				eo.parse_options.base = 10;
				if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
				if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
				eo.parse_options.read_precision = DONT_READ_PRECISION;
				PrintOptions po;
				po.is_approximate = &b;
				po.number_fraction_format = FRACTION_DECIMAL;
				po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
				CALCULATOR->resetExchangeRatesUsed();
				block_error();
				MathStructure v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(toValue, eo.parse_options), uTo, uFrom, 1500, eo);
				if(!v_mstruct.isAborted() && check_exchange_rates(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")))) v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(toValue, eo.parse_options), uTo, uFrom, 1500, eo);
				if(v_mstruct.isAborted()) {
					old_fromValue = CALCULATOR->timedOutString();
				} else {
					old_fromValue = CALCULATOR->print(v_mstruct, 300, po);
				}
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")), old_fromValue.c_str());
				b = b || v_mstruct.isApproximate();
				display_errors(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
				unblock_error();
			}
		} else {
			if(CALCULATOR->timedOutString() == fromValue) return;
			if(uFrom == uTo) {
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")), fromValue);
			} else {
				EvaluationOptions eo;
				eo.approximation = APPROXIMATION_APPROXIMATE;
				eo.parse_options = evalops.parse_options;
				eo.parse_options.base = 10;
				if(eo.parse_options.parsing_mode == PARSING_MODE_RPN || eo.parse_options.parsing_mode == PARSING_MODE_CHAIN) eo.parse_options.parsing_mode = PARSING_MODE_ADAPTIVE;
				if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
				eo.parse_options.read_precision = DONT_READ_PRECISION;
				PrintOptions po;
				po.is_approximate = &b;
				po.number_fraction_format = FRACTION_DECIMAL;
				po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS;
				CALCULATOR->resetExchangeRatesUsed();
				block_error();
				MathStructure v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(fromValue, eo.parse_options), uFrom, uTo, 1500, eo);
				if(!v_mstruct.isAborted() && check_exchange_rates(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")))) v_mstruct = CALCULATOR->convert(CALCULATOR->unlocalizeExpression(fromValue, eo.parse_options), uFrom, uTo, 1500, eo);
				if(v_mstruct.isAborted()) {
					old_toValue = CALCULATOR->timedOutString();
				} else {
					old_toValue = CALCULATOR->print(v_mstruct, 300, po);
				}
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")), old_toValue.c_str());
				b = b || v_mstruct.isApproximate();
				display_errors(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
				unblock_error();
			}
		}
		if(b && printops.use_unicode_signs && can_display_unicode_string_function(SIGN_ALMOST_EQUAL, (void*) gtk_builder_get_object(units_builder, "units_label_equals"))) {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_equals")), SIGN_ALMOST_EQUAL);
		} else {
			gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_equals")), "=");
		}
	}
}

void on_units_convert_view_row_activated(GtkTreeView*, GtkTreePath *path, GtkTreeViewColumn*, gpointer) {
	GtkTreeIter iter;
	gtk_tree_model_get_iter(units_convert_filter, &iter, path);
	Unit *u = NULL;
	gtk_tree_model_get(units_convert_filter, &iter, UNITS_POINTER_COLUMN, &u, -1);
	if(u) {
		gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_to_unit")), u->print(true, printops.abbreviate_names, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) gtk_builder_get_object(units_builder, "units_label_to_unit")).c_str());
		selected_to_unit = u;
		convert_in_wUnits();
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_convert_to_button")), FALSE);
	gtk_widget_hide(units_convert_window);
}

void on_units_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
}

void on_units_toggle_button_from_toggled(GtkToggleButton *togglebutton, gpointer) {
	if(gtk_toggle_button_get_active(togglebutton)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_toggle_button_to")), FALSE);
		convert_in_wUnits();
	}
}

void on_units_button_convert_clicked(GtkButton*, gpointer) {
	convert_in_wUnits();
}

void on_units_toggle_button_to_toggled(GtkToggleButton *togglebutton, gpointer) {
	if(gtk_toggle_button_get_active(togglebutton)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_toggle_button_from")), FALSE);
		convert_in_wUnits();
	}
}

void on_units_entry_from_val_activate(GtkEntry*, gpointer) {
	convert_in_wUnits(0);
}
void on_units_entry_to_val_activate(GtkEntry*, gpointer) {
	convert_in_wUnits(1);
}

gboolean on_units_dialog_button_press_event(GtkWidget*, GdkEventButton *event, gpointer) {
	gtk_widget_hide(units_convert_window);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_convert_to_button")), FALSE);
	return FALSE;
}
gboolean on_units_dialog_delete_event() {
	gtk_widget_hide(units_convert_window);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_convert_to_button")), FALSE);
	return FALSE;
}

gboolean on_units_entry_from_val_focus_out_event(GtkEntry*, GdkEventFocus*, gpointer) {
	if(old_fromValue != gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")))) convert_in_wUnits(0);
	return FALSE;
}
gboolean on_units_entry_to_val_focus_out_event(GtkEntry*, GdkEventFocus*, gpointer) {
	if(old_toValue != gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")))) convert_in_wUnits(1);
	return FALSE;
}

bool units_convert_ignore_enter = false, units_convert_hover_blocked = false;

gboolean on_units_convert_view_enter_notify_event(GtkWidget*, GdkEventCrossing*, gpointer) {
	return units_convert_ignore_enter;
}
gboolean on_units_convert_view_motion_notify_event(GtkWidget*, GdkEventMotion*, gpointer) {
	units_convert_ignore_enter = FALSE;
	if(units_convert_hover_blocked) {
		gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(units_convert_view), TRUE);
		units_convert_hover_blocked = false;
	}
	return FALSE;
}
gboolean on_units_convert_window_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer) {
	if(!gtk_widget_get_mapped(units_convert_window)) return FALSE;
	gtk_widget_event(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_to_button")), (GdkEvent*) event);
	return TRUE;
}
gboolean on_units_convert_window_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer) {
	if(!gtk_widget_get_mapped(units_convert_window)) return FALSE;
	gtk_widget_hide(units_convert_window);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_convert_to_button")), FALSE);
	return TRUE;
}
void units_convert_resize_popup() {

	int matches = gtk_tree_model_iter_n_children(units_convert_filter, NULL);

	gint x, y;
	gint items, height = 0, items_y = 0, height_diff;
	GdkDisplay *display;

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	GdkMonitor *monitor;
#endif
	GdkRectangle area, rect;
	GtkAllocation alloc;
	GdkWindow *window;
	GtkRequisition popup_req;
	GtkRequisition tree_req;
	GtkTreePath *path;
	gboolean above;
	GtkTreeViewColumn *column;

	gtk_widget_get_allocation(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_to_button")), &alloc);
	window = gtk_widget_get_window(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_to_button")));
	gdk_window_get_origin(window, &x, &y);
	x += alloc.x;
	y += alloc.y;

	gtk_widget_realize(units_convert_view);
	while(gtk_events_pending()) gtk_main_iteration();
	gtk_tree_view_columns_autosize(GTK_TREE_VIEW(units_convert_view));
	column = gtk_tree_view_get_column(GTK_TREE_VIEW(units_convert_view), 0);

	gtk_widget_get_preferred_size(units_convert_view, &tree_req, NULL);
	gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &height_diff);

	path = gtk_tree_path_new_from_indices(0, -1);
	gtk_tree_view_get_cell_area(GTK_TREE_VIEW(units_convert_view), path, column, &rect);
	gtk_tree_path_free(path);
	items_y = rect.y;
	height_diff -= rect.height;
	if(height_diff < 2) height_diff = 2;

	display = gtk_widget_get_display(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_to_button")));
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 22
	monitor = gdk_display_get_monitor_at_window(display, window);
	gdk_monitor_get_workarea(monitor, &area);
#else
	GdkScreen *screen = gdk_display_get_default_screen(display);
	gdk_screen_get_monitor_workarea(screen, gdk_screen_get_monitor_at_window(screen, window), &area);
#endif

	items = matches;
	if(items > 20) items = 20;
	if(items > 0) {
		path = gtk_tree_path_new_from_indices(items - 1, -1);
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(units_convert_view), path, column, &rect);
		gtk_tree_path_free(path);
		height = rect.y + rect.height - items_y + height_diff;
	}
	while(items > 0 && ((y > area.height / 2 && area.y + y < height) || (y <= area.height / 2 && area.height - y < height))) {
		items--;
		path = gtk_tree_path_new_from_indices(items - 1, -1);
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(units_convert_view), path, column, &rect);
		gtk_tree_path_free(path);
		height = rect.y + rect.height - items_y + height_diff;
	}

	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(units_convert_scrolled), height);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(units_convert_scrolled), GTK_POLICY_NEVER, matches > 20 ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER);


	if(items <= 0) gtk_widget_hide(units_convert_scrolled);
	else gtk_widget_show(units_convert_scrolled);

	gtk_widget_get_preferred_size(units_convert_window, &popup_req, NULL);

	if(popup_req.width < rect.width + 2) popup_req.width = rect.width + 2;
	if(popup_req.width < alloc.width) {
		popup_req.width = alloc.width;
		gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_search")), popup_req.width, -1);
	}

	if(x < area.x) x = area.x;
	else if(x + popup_req.width > area.x + area.width) x = area.x + area.width - popup_req.width;

	if(y + alloc.height + popup_req.height <= area.y + area.height || y - area.y < (area.y + area.height) - (y + alloc.height)) {
		y += alloc.height;
		above = FALSE;
	} else {
		path = gtk_tree_path_new_from_indices(matches - 1, -1);
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(units_convert_view), path, column, &rect);
		gtk_tree_path_free(path);
		height = rect.y + rect.height + height_diff;
		path = gtk_tree_path_new_from_indices(matches - items, -1);
		gtk_tree_view_get_cell_area(GTK_TREE_VIEW(units_convert_view), path, column, &rect);
		gtk_tree_path_free(path);
		height -= rect.y;
		gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(units_convert_scrolled), height);
		y -= popup_req.height;
		above = TRUE;
	}

	if(matches > 0) {
		path = gtk_tree_path_new_from_indices(above ? matches - 1 : 0, -1);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(units_convert_view), path, NULL, FALSE, 0.0, 0.0);
		gtk_tree_path_free(path);
	}

	gtk_window_move(GTK_WINDOW(units_convert_window), x, y);

}
void on_units_convert_to_button_toggled(GtkToggleButton *w, gpointer) {
	if(gtk_toggle_button_get_active(w)) {
		units_convert_ignore_enter = TRUE;
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_convert_search")), "");
		units_convert_resize_popup();
		if(!gtk_widget_is_visible(units_convert_window)) {
			gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(units_convert_view), TRUE);
			gtk_window_set_transient_for(GTK_WINDOW(units_convert_window), GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog")));
			gtk_window_group_add_window(gtk_window_get_group(GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog"))), GTK_WINDOW(units_convert_window));
			gtk_window_set_screen(GTK_WINDOW(units_convert_window), gtk_widget_get_screen(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_to_button"))));
			gtk_widget_show(units_convert_window);
		}
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(units_convert_view)));
		while(gtk_events_pending()) gtk_main_iteration();
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(units_convert_view)));
	} else {
		gtk_widget_hide(units_convert_window);
	}
}

void on_units_button_new_clicked(GtkButton*, gpointer) {
	if(selected_unit_category.empty() || selected_unit_category[0] != '/') {
		edit_unit("", NULL, GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog")));
	} else {
		//fill in category field with selected category
		edit_unit(selected_unit_category.substr(1, selected_unit_category.length() - 1).c_str(), NULL, GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog")));
	}
}

void on_units_button_edit_clicked(GtkButton*, gpointer) {
	Unit *u = get_selected_unit();
	if(u) {
		edit_unit("", u, GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog")));
	}
}

void on_units_button_insert_clicked(GtkButton*, gpointer) {
	Unit *u = get_selected_unit();
	if(u) {
		if(u->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			PrintOptions po = printops;
			po.is_approximate = NULL;
			po.can_display_unicode_string_arg = (void*) expressiontext;
			string str = ((CompositeUnit*) u)->print(po, false, TAG_TYPE_HTML, true);
			insert_text(str.c_str());
		} else {
			insert_text(u->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, true, false, &can_display_unicode_string_function, (void*) expressiontext).formattedName(TYPE_UNIT, true).c_str());
		}
	}
}

void on_units_button_convert_to_clicked(GtkButton*, gpointer) {
	if(b_busy) return;
	Unit *u = get_selected_unit();
	if(u) convert_result_to_unit(u);
}

void on_units_button_deactivate_clicked(GtkButton*, gpointer) {
	Unit *u = get_selected_unit();
	if(u) {
		u->setActive(!u->isActive());
		update_umenus();
	}
}

void on_units_button_delete_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	Unit *u = get_selected_unit();
	if(!u || !u->isLocal()) return;
	if(u->isUsedByOtherUnits()) {
		//do not delete units that are used by other units
		show_message(_("Cannot delete unit as it is needed by other units."), GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));
		return;
	}
	u->destroy();
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &model, &iter)) {
		//reselect selected unit category
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		string str = selected_unit_category;
		unit_removed(u);
		if(str == selected_unit_category) gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), path);
		gtk_tree_path_free(path);
	} else {
		unit_removed(u);
	}
}

void on_units_entry_search_changed(GtkEntry *w, gpointer) {
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnits_selection_changed, NULL);
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnits_store), &iter)) return;
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	do {
		bool b = str.empty();
		Unit *u = NULL;
		if(!b) gtk_tree_model_get(GTK_TREE_MODEL(tUnits_store), &iter, UNITS_POINTER_COLUMN, &u, -1);
		if(u) b = name_matches(u, str) || title_matches(u, str) || country_matches(u, str);
		gtk_list_store_set(tUnits_store, &iter, UNITS_VISIBLE_COLUMN, b, -1);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tUnits_store), &iter));
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tUnits_selection_changed, NULL);
	if(str.empty()) {
		gtk_widget_grab_focus(tUnits);
	} else {
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnits_store_filter), &iter)) {
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)));
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits)), &iter);
			GtkTreePath *path = gtk_tree_model_get_path(tUnits_store_filter, &iter);
			if(path) {
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(tUnits), path, NULL, FALSE);
				gtk_tree_path_free(path);
			}
		}
	}
}

void on_units_convert_search_changed(GtkEntry *w, gpointer) {
	GtkTreeIter iter;
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnits_store), &iter)) return;
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	do {
		bool b = str.empty();
		Unit *u = NULL;
		if(!b) gtk_tree_model_get(GTK_TREE_MODEL(tUnits_store), &iter, UNITS_POINTER_COLUMN, &u, -1);
		if(u) {
			b = name_matches(u, str) || title_matches(u, str) || country_matches(u, str);
		}
		gtk_list_store_set(tUnits_store, &iter, UNITS_VISIBLE_COLUMN_CONVERT, b, -1);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tUnits_store), &iter));
	if(!str.empty()) {
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(units_convert_filter), &iter)) {
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(units_convert_view)));
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(units_convert_view)), &iter);
		}
	}
	while(gtk_events_pending()) gtk_main_iteration();
	//if(gtk_widget_is_visible(units_convert_window)) units_convert_resize_popup();
}

gboolean on_units_dialog_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(gtk_widget_has_focus(GTK_WIDGET(tUnits)) && gdk_keyval_to_unicode(event->keyval) > 32) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_entry_search")));
	}
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_entry_search")))) {
		if(event->keyval == GDK_KEY_Escape) {
			gtk_widget_hide(o);
			return TRUE;
		}
		if(event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_Page_Up || event->keyval == GDK_KEY_Page_Down || event->keyval == GDK_KEY_KP_Page_Up || event->keyval == GDK_KEY_KP_Page_Down) {
			gtk_widget_grab_focus(GTK_WIDGET(tUnits));
		}
	}
	return FALSE;
}
gboolean on_units_convert_to_button_focus_out_event(GtkWidget*, GdkEvent*, gpointer) {
	gtk_widget_hide(units_convert_window);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_convert_to_button")), FALSE);
	return FALSE;
}
gboolean on_units_convert_to_button_key_press_event(GtkWidget*, GdkEventKey *event, gpointer) {
	if(!gtk_widget_get_visible(units_convert_window)) return FALSE;
	switch(event->keyval) {
		case GDK_KEY_Escape: {
			gtk_widget_hide(units_convert_window);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(units_builder, "units_convert_to_button")), FALSE);
			return TRUE;
			break;
		}
		case GDK_KEY_KP_Enter: {}
		case GDK_KEY_ISO_Enter: {}
		case GDK_KEY_Return: {
			GtkTreeIter iter;
			if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(units_convert_view)), NULL, &iter)) {
				GtkTreePath *path = gtk_tree_model_get_path(units_convert_filter, &iter);
				on_units_convert_view_row_activated(GTK_TREE_VIEW(units_convert_view), path, NULL, NULL);
				gtk_tree_path_free(path);
				return TRUE;
			}
		}
		case GDK_KEY_BackSpace: {}
		case GDK_KEY_Delete: {
			string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_convert_search")));
			if(str.length() > 0) {
				str = str.substr(0, str.length() - 1);
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_convert_search")), str.c_str());
			}
			return TRUE;
		}
		case GDK_KEY_Down: {}
		case GDK_KEY_End: {}
		case GDK_KEY_Home: {}
		case GDK_KEY_KP_Page_Up: {}
		case GDK_KEY_Page_Up: {}
		case GDK_KEY_KP_Page_Down: {}
		case GDK_KEY_Page_Down: {}
		case GDK_KEY_Up: {
			GtkTreeIter iter;
			GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(units_convert_view));
			bool b = false;
			if(event->keyval == GDK_KEY_Up) {
				if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
					if(gtk_tree_model_iter_previous(units_convert_filter, &iter)) b = true;
					else gtk_tree_selection_unselect_all(selection);
				} else {
					gint rows = gtk_tree_model_iter_n_children(units_convert_filter, NULL);
					if(rows > 0) {
						GtkTreePath *path = gtk_tree_path_new_from_indices(rows - 1, -1);
						gtk_tree_model_get_iter(units_convert_filter, &iter, path);
						gtk_tree_path_free(path);
						b = true;
					}
				}
			} else if(event->keyval == GDK_KEY_Down) {
				if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
					if(gtk_tree_model_iter_next(units_convert_filter, &iter)) b = true;
					else gtk_tree_selection_unselect_all(selection);
				} else {
					if(gtk_tree_model_get_iter_first(units_convert_filter, &iter)) b = true;
				}
			} else if(event->keyval == GDK_KEY_End) {
				gint rows = gtk_tree_model_iter_n_children(units_convert_filter, NULL);
				if(rows > 0) {
					GtkTreePath *path = gtk_tree_path_new_from_indices(rows - 1, -1);
					gtk_tree_model_get_iter(units_convert_filter, &iter, path);
					gtk_tree_path_free(path);
					b = true;
				}
			} else if(event->keyval == GDK_KEY_Home) {
				if(gtk_tree_model_get_iter_first(units_convert_filter, &iter)) b = true;
			} else if(event->keyval == GDK_KEY_KP_Page_Down || event->keyval == GDK_KEY_Page_Down) {
				if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
					b = true;
					for(size_t i = 0; i < 20; i++) {
						if(!gtk_tree_model_iter_next(units_convert_filter, &iter)) {
							b = false;
							gint rows = gtk_tree_model_iter_n_children(units_convert_filter, NULL);
							if(rows > 0) {
								GtkTreePath *path = gtk_tree_path_new_from_indices(rows - 1, -1);
								gtk_tree_model_get_iter(units_convert_filter, &iter, path);
								gtk_tree_path_free(path);
								b = true;
							}
							break;
						}
					}
				}
			} else if(event->keyval == GDK_KEY_KP_Page_Up || event->keyval == GDK_KEY_Page_Up) {
				if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
					b = true;
					for(size_t i = 0; i < 20; i++) {
						if(!gtk_tree_model_iter_previous(units_convert_filter, &iter)) {
							b = false;
							if(gtk_tree_model_get_iter_first(units_convert_filter, &iter)) b = true;
							break;
						}
					}
				}
			}
			if(b) {
				gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(units_convert_view), FALSE);
				units_convert_hover_blocked = true;
				GtkTreePath *path = gtk_tree_model_get_path(units_convert_filter, &iter);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(units_convert_view), path, NULL, FALSE, 0.0, 0.0);
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_iter(selection, &iter);
				gtk_tree_path_free(path);
			}
			return TRUE;
		}
	}
	if(gdk_keyval_to_unicode(event->keyval) > 32) {
		gchar buffer[10];
		buffer[g_unichar_to_utf8(gdk_keyval_to_unicode(event->keyval), buffer)] = '\0';
		string str = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_convert_search")));
		str += buffer;
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_convert_search")), str.c_str());
		return TRUE;
	}
	return FALSE;
}

GtkWidget* get_units_dialog(void) {

	if(!units_builder) {

		units_builder = getBuilder("units.ui");
		g_assert(units_builder != NULL);

		selected_unit_category = _("All");
		selected_unit = NULL;
		selected_to_unit = NULL;

		g_assert(gtk_builder_get_object(units_builder, "units_dialog") != NULL);

		tUnitCategories = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_treeview_category"));
		tUnits = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_treeview_unit"));

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION < 14
		if(!gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "pan-down-symbolic")) {
			GtkWidget *arrow_down = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
			gtk_widget_set_size_request(GTK_WIDGET(arrow_down), 18, 18);
			gtk_widget_show(arrow_down);
			gtk_widget_destroy(GTK_WIDGET(gtk_builder_get_object(units_builder, "image_to_unit")));
			gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(units_builder, "units_to_box")), arrow_down);
		}
#endif

		tUnits_store = gtk_list_store_new(UNITS_N_COLUMNS, G_TYPE_STRING, G_TYPE_POINTER, CAIRO_GOBJECT_TYPE_SURFACE, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
		tUnits_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tUnits_store), NULL);
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tUnits_store_filter), UNITS_VISIBLE_COLUMN);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tUnits), GTK_TREE_MODEL(tUnits_store_filter));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnits));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
		gtk_cell_renderer_set_padding(renderer, 4, 0);
		units_flag_column = gtk_tree_view_column_new_with_attributes(_("Flag"), renderer, "surface", UNITS_FLAG_COLUMN, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tUnits), units_flag_column);
		renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", UNITS_TITLE_COLUMN, NULL);
		gtk_tree_view_column_set_sort_column_id(column, UNITS_TITLE_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tUnits), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tUnits_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnits_store), UNITS_TITLE_COLUMN, string_sort_func, GINT_TO_POINTER(UNITS_TITLE_COLUMN), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnits_store), UNITS_TITLE_COLUMN, GTK_SORT_ASCENDING);

		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tUnits), FALSE);

		tUnitCategories_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tUnitCategories), GTK_TREE_MODEL(tUnitCategories_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tUnitCategories), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tUnitCategories_selection_changed), NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tUnitCategories_store), 0, category_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tUnitCategories_store), 0, GTK_SORT_ASCENDING);

		units_convert_window = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_window"));
		units_convert_scrolled = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_scrolled"));
		units_convert_view = GTK_WIDGET(gtk_builder_get_object(units_builder, "units_convert_view"));
		units_convert_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tUnits_store), NULL);
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(units_convert_filter), UNITS_VISIBLE_COLUMN_CONVERT);
		gtk_tree_view_set_model(GTK_TREE_VIEW(units_convert_view), GTK_TREE_MODEL(units_convert_filter));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(units_convert_view));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		units_convert_flag_renderer = gtk_cell_renderer_pixbuf_new();
		GtkCellArea *area = gtk_cell_area_box_new();
		gtk_cell_area_box_set_spacing(GTK_CELL_AREA_BOX(area), 12);
		gtk_cell_area_box_pack_start(GTK_CELL_AREA_BOX(area), units_convert_flag_renderer, FALSE, TRUE, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), units_convert_flag_renderer, "surface", UNITS_FLAG_COLUMN, NULL);
		renderer = gtk_cell_renderer_text_new();
		gtk_cell_area_box_pack_start(GTK_CELL_AREA_BOX(area), renderer, TRUE, TRUE, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(area), renderer, "text", UNITS_TITLE_COLUMN, NULL);
		column = gtk_tree_view_column_new_with_area(area);
		gtk_tree_view_column_set_sort_column_id(column, UNITS_TITLE_COLUMN);
		gtk_tree_view_append_column(GTK_TREE_VIEW(units_convert_view), column);

#if GTK_MAJOR_VERSION == 3 && GTK_MINOR_VERSION >= 16
		gtk_label_set_width_chars(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_to_unit")), 20);
		gtk_label_set_xalign(GTK_LABEL(gtk_builder_get_object(units_builder, "units_label_to_unit")), 0.0);
#else
		gint w;
		PangoLayout *layout = gtk_widget_create_pango_layout(GTK_WIDGET((gtk_builder_get_object(units_builder, "units_label_to_unit"))), "AAAAAAAAAAAAAAAAAAAA");
		pango_layout_get_pixel_size(layout, &w, NULL);
		gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_to_box")), w + 16, -1);
#endif

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(units_builder, "units_textview_description")));
		gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);

		if(units_width > 0 && units_height > 0) {
			gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog")), units_width, units_height);
			if(units_vposition <= 0) units_vposition = units_height / 3 * 2;
		}
		if(units_hposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(units_builder, "units_hpaned")), units_hposition);
		if(units_vposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(units_builder, "units_vpaned")), units_vposition);

		gtk_builder_add_callback_symbols(units_builder, "on_units_dialog_button_press_event", G_CALLBACK(on_units_dialog_button_press_event), "on_units_dialog_delete_event", G_CALLBACK(on_units_dialog_delete_event), "on_units_dialog_key_press_event", G_CALLBACK(on_units_dialog_key_press_event), "on_units_button_convert_clicked", G_CALLBACK(on_units_button_convert_clicked), "on_units_entry_to_val_activate", G_CALLBACK(on_units_entry_to_val_activate), "on_units_entry_to_val_focus_out_event", G_CALLBACK(on_units_entry_to_val_focus_out_event), "on_math_entry_key_press_event", G_CALLBACK(on_math_entry_key_press_event), "on_units_entry_from_val_activate", G_CALLBACK(on_units_entry_from_val_activate), "on_units_entry_from_val_focus_out_event", G_CALLBACK(on_units_entry_from_val_focus_out_event), "on_units_convert_to_button_focus_out_event", G_CALLBACK(on_units_convert_to_button_focus_out_event), "on_units_convert_to_button_key_press_event", G_CALLBACK(on_units_convert_to_button_key_press_event), "on_units_convert_to_button_toggled", G_CALLBACK(on_units_convert_to_button_toggled), "on_units_entry_search_changed", G_CALLBACK(on_units_entry_search_changed), "on_units_button_new_clicked", G_CALLBACK(on_units_button_new_clicked), "on_units_button_edit_clicked", G_CALLBACK(on_units_button_edit_clicked), "on_units_button_delete_clicked", G_CALLBACK(on_units_button_delete_clicked), "on_units_button_deactivate_clicked", G_CALLBACK(on_units_button_deactivate_clicked), "on_units_button_insert_clicked", G_CALLBACK(on_units_button_insert_clicked), "on_units_button_convert_to_clicked", G_CALLBACK(on_units_button_convert_to_clicked), "on_units_convert_window_button_press_event", G_CALLBACK(on_units_convert_window_button_press_event), "on_units_convert_window_key_press_event", G_CALLBACK(on_units_convert_window_key_press_event), "on_units_convert_view_enter_notify_event", G_CALLBACK(on_units_convert_view_enter_notify_event), "on_units_convert_view_motion_notify_event", G_CALLBACK(on_units_convert_view_motion_notify_event), "on_units_convert_view_row_activated", G_CALLBACK(on_units_convert_view_row_activated), "on_units_convert_search_changed", G_CALLBACK(on_units_convert_search_changed), NULL);
		gtk_builder_connect_signals(units_builder, NULL);

		update_units_tree();

		gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object(units_builder, "units_entry_from_val")), "1");
		gtk_entry_set_text (GTK_ENTRY (gtk_builder_get_object(units_builder, "units_entry_to_val")), "1");
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_from_val")), 1.0);
		gtk_entry_set_alignment(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_to_val")), 1.0);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(units_builder, "units_dialog"));
}

void manage_units(GtkWindow *parent, const gchar *str, bool show_currencies) {
	GtkWidget *dialog = get_units_dialog();
	if(!gtk_widget_is_visible(dialog)) {
		gtk_widget_grab_focus(tUnits);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), "");
		gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
		gtk_widget_show(dialog);
		fix_deactivate_label_width(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_buttonlabel_deactivate")));
	}
	if(str) {
		if(show_currencies) {
			string s_cat = CALCULATOR->u_euro->category();
			GtkTreeIter iter1, iter;
			if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter1) && gtk_tree_model_iter_children(GTK_TREE_MODEL(tUnitCategories_store), &iter, &iter1)) {
				do {
					gchar *gstr;
					gtk_tree_model_get(GTK_TREE_MODEL(tUnitCategories_store), &iter, 0, &gstr, -1);
					if(s_cat == gstr) {
						gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
						g_free(gstr);
						break;
					}
					g_free(gstr);
				} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tUnitCategories_store), &iter));
			}
		} else {
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tUnitCategories_store), &iter)) {
				GtkTreeIter iter2 = iter;
				while(!gtk_tree_model_iter_has_child(GTK_TREE_MODEL(tUnitCategories_store), &iter) && gtk_tree_model_iter_next(GTK_TREE_MODEL(tUnitCategories_store), &iter2)) {
					iter = iter2;
				}
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tUnitCategories)), &iter);
			}
		}
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(units_builder, "units_entry_search")), str);
	}
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}

void update_units_settings() {
	if(units_builder) {
		gint w = 0, h = 0;
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(units_builder, "units_dialog")), &w, &h);
		units_width = w;
		units_height = h;
		units_hposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(units_builder, "units_hpaned")));
		units_vposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(units_builder, "units_vpaned")));
	}
}
void units_font_updated() {
	if(units_builder) fix_deactivate_label_width(GTK_WIDGET(gtk_builder_get_object(units_builder, "units_buttonlabel_deactivate")));
}
