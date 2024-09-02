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
#include "expressionedit.h"
#include "exportcsvdialog.h"
#include "variableeditdialog.h"
#include "variablesdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *variables_builder = NULL;
GtkWidget *tVariableCategories;
GtkWidget *tVariables;
GtkListStore *tVariables_store;
GtkTreeModel *tVariables_store_filter;
GtkTreeStore *tVariableCategories_store;
string selected_variable_category;
Variable *selected_variable = NULL;

gint variables_width = -1, variables_height = -1, variables_hposition = -1, variables_vposition = -1;

void on_tVariableCategories_selection_changed(GtkTreeSelection *treeselection, gpointer);
void on_tVariables_selection_changed(GtkTreeSelection *treeselection, gpointer);
void on_variables_entry_search_changed(GtkEntry *w, gpointer);

bool read_variables_dialog_settings_line(string &svar, string&, int &v) {
	if(svar == "variables_width") {
		variables_width = v;
	} else if(svar == "variables_height") {
		variables_height = v;
	} else if(svar == "variables_panel_position") {
		variables_hposition = v;
	} else if(svar == "variables_vpanel_position") {
		variables_vposition = v;
	} else if(svar == "variables_hpanel_position") {
		variables_hposition = v;
	} else {
		return false;
	}
	return true;
}
void write_variables_dialog_settings(FILE *file) {
	if(variables_height > -1) fprintf(file, "variables_height=%i\n", variables_height);
	if(variables_width > -1) fprintf(file, "variables_width=%i\n", variables_width);
	if(variables_hposition > -1) fprintf(file, "variables_hpanel_position=%i\n", variables_hposition);
	if(variables_vposition > -1) fprintf(file, "variables_vpanel_position=%i\n", variables_vposition);
}

Variable *get_selected_variable() {
	return selected_variable;
}
void update_variables_tree() {
	if(!variables_builder) return;
	GtkTreeIter iter, iter2, iter3;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tVariableCategories));
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariableCategories_selection_changed, NULL);
	gtk_tree_store_clear(tVariableCategories_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariableCategories_selection_changed, NULL);
	gtk_tree_store_append(tVariableCategories_store, &iter3, NULL);
	gtk_tree_store_set(tVariableCategories_store, &iter3, 0, _("All"), 1, _("All"), -1);
	string str;
	tree_struct *item, *item2;
	variable_cats.it = variable_cats.items.begin();
	if(variable_cats.it != variable_cats.items.end()) {
		item = &*variable_cats.it;
		++variable_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	iter2 = iter3;
	while(item) {
		gtk_tree_store_append(tVariableCategories_store, &iter, &iter2);
		str += "/";
		str += item->item;
		gtk_tree_store_set(tVariableCategories_store, &iter, 0, item->item.c_str(), 1, str.c_str(), -1);
		if(str == selected_variable_category) {
			EXPAND_TO_ITER(model, tVariableCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
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
	EXPAND_ITER(model, tVariableCategories, iter3)
	if(!user_variables.empty()) {
		gtk_tree_store_append(tVariableCategories_store, &iter, NULL);
		EXPAND_TO_ITER(model, tVariableCategories, iter)
		gtk_tree_store_set(tVariableCategories_store, &iter, 0, _("User variables"), 1, _("User variables"), -1);
		if(selected_variable_category == _("User variables")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
		}
	}
	if(!variable_cats.objects.empty()) {
		//add "Uncategorized" category if there are variables without category
		gtk_tree_store_append(tVariableCategories_store, &iter, &iter3);
		EXPAND_TO_ITER(model, tVariableCategories, iter)
		gtk_tree_store_set(tVariableCategories_store, &iter, 0, _("Uncategorized"), 1, _("Uncategorized"), -1);
		if(selected_variable_category == _("Uncategorized")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
		}
	}
	if(!ia_variables.empty()) {
		//add "Inactive" category if there are inactive variables
		gtk_tree_store_append(tVariableCategories_store, &iter, NULL);
		EXPAND_TO_ITER(model, tVariableCategories, iter)
		gtk_tree_store_set(tVariableCategories_store, &iter, 0, _("Inactive"), 1, _("Inactive"), -1);
		if(selected_variable_category == _("Inactive")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
		}
	}
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &model, &iter)) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_variable_category = _("All");
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter3);
	}
}

void setVariableTreeItem(GtkTreeIter &iter2, Variable *v) {
	gtk_list_store_append(tVariables_store, &iter2);
	gtk_list_store_set(tVariables_store, &iter2, 0, v->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tVariables).c_str(), 1, (gpointer) v, 2, TRUE, -1);
	GtkTreeIter iter;
	if(v == selected_variable && gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(tVariables_store_filter), &iter, &iter2)) {
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &iter);
	}
}

void on_tVariableCategories_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model, *model2;
	GtkTreeIter iter, iter2;
	bool no_cat = false, b_all = false, b_inactive = false, b_user = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variables_entry_search_changed, NULL);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variables_builder, "variables_entry_search")), "");
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_variables_entry_search_changed, NULL);
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariables_selection_changed, NULL);
	gtk_list_store_clear(tVariables_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariables_selection_changed, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_edit")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_insert")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_delete")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_deactivate")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_export")), FALSE);

	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, -1);
		selected_variable_category = gstr;
		if(selected_variable_category == _("All")) {
			b_all = true;
		} else if(selected_variable_category == _("Uncategorized")) {
			no_cat = true;
		} else if(selected_variable_category == _("Inactive")) {
			b_inactive = true;
		} else if(selected_variable_category == _("User variables")) {
			b_user = true;
		}

		if(!b_user && !b_all && !no_cat && !b_inactive && selected_variable_category[0] == '/') {
			string str = selected_variable_category.substr(1, selected_variable_category.length() - 1);
			ExpressionItem *o;
			size_t l1 = str.length(), l2;
			for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
				o = CALCULATOR->variables[i];
				l2 = o->category().length();
				if(o->isActive() && (l2 == l1 || (l2 > l1 && o->category()[l1] == '/')) && o->category().substr(0, l1) == str) {
					setVariableTreeItem(iter2, CALCULATOR->variables[i]);
				}
			}
		} else {
			for(size_t i = 0; i < CALCULATOR->variables.size(); i++) {
				if((b_inactive && !CALCULATOR->variables[i]->isActive()) || (CALCULATOR->variables[i]->isActive() && (b_all || (no_cat && CALCULATOR->variables[i]->category().empty()) || (b_user && CALCULATOR->variables[i]->isLocal()) || (!b_inactive && CALCULATOR->variables[i]->category() == selected_variable_category)))) {
					setVariableTreeItem(iter2, CALCULATOR->variables[i]);
				}
			}
		}

		if(!selected_variable || !gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &model2, &iter2)) {
			if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tVariables_store_filter), &iter2)) {
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &iter2);
			}
		}
		g_free(gstr);

	} else {
		selected_variable_category = "";
	}

}

void on_tVariables_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		Variable *v;
		gtk_tree_model_get(model, &iter, 1, &v, -1);
		if(!CALCULATOR->stillHasVariable(v)) {
			show_message(_("Variable does not exist anymore."), GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")));
			selected_variable = NULL;
			update_vmenu();
			return;
		}
		//remember selection
		selected_variable = v;
		if(CALCULATOR->stillHasVariable(v)) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(variables_builder, "variables_textview_description")));
			gtk_text_buffer_set_text(buffer, "", -1);
			GtkTextIter iter;
			string str, str2;
			const ExpressionName *ename = &v->preferredName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(variables_builder, "variables_textview_description"));
			str += ename->formattedName(TYPE_VARIABLE, true);
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
			str = "";
			for(size_t i2 = 1; i2 <= v->countNames(); i2++) {
				if(&v->getName(i2) != ename) {
					str += ", ";
					str += v->getName(i2).formattedName(TYPE_VARIABLE, true);
				}
			}
			str += "\n\n";
			if(v->isKnown()) {
				bool is_approximate = false;
				if(((KnownVariable*) v)->get().isMatrix() && ((KnownVariable*) v)->get().columns() * ((KnownVariable*) v)->get().rows() > 16) {
					str += _("a matrix");
				} else if(((KnownVariable*) v)->get().isVector() && ((KnownVariable*) v)->get().size() > 10) {
					str += _("a vector");
				} else {
					PrintOptions po = printops;
					po.can_display_unicode_string_arg = (void*) gtk_builder_get_object(variables_builder, "variables_textview_description");
					po.interval_display = INTERVAL_DISPLAY_PLUSMINUS;
					po.base = 10;
					po.number_fraction_format = FRACTION_DECIMAL_EXACT;
					po.allow_non_usable = true;
					po.is_approximate = &is_approximate;
					if(v->isApproximate() || is_approximate) {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_ALMOST_EQUAL, po.can_display_unicode_string_arg))) {
							str += SIGN_ALMOST_EQUAL " ";
						} else {
							str += "= ";
							str += _("approx.");
						}
					} else {
						str += "= ";
					}
					str += CALCULATOR->print(((KnownVariable*) v)->get(), 1000, po);
				}
			} else {
				if(((UnknownVariable*) v)->assumptions()) {
					string value;
					if(((UnknownVariable*) v)->assumptions()->type() != ASSUMPTION_TYPE_BOOLEAN) {
						switch(((UnknownVariable*) v)->assumptions()->sign()) {
							case ASSUMPTION_SIGN_POSITIVE: {value = _("positive"); break;}
							case ASSUMPTION_SIGN_NONPOSITIVE: {value = _("non-positive"); break;}
							case ASSUMPTION_SIGN_NEGATIVE: {value = _("negative"); break;}
							case ASSUMPTION_SIGN_NONNEGATIVE: {value = _("non-negative"); break;}
							case ASSUMPTION_SIGN_NONZERO: {value = _("non-zero"); break;}
							default: {}
						}
					}
					if(!value.empty() && ((UnknownVariable*) v)->assumptions()->type() != ASSUMPTION_TYPE_NONE) value += " ";
					switch(((UnknownVariable*) v)->assumptions()->type()) {
						case ASSUMPTION_TYPE_INTEGER: {value += _("integer"); break;}
						case ASSUMPTION_TYPE_BOOLEAN: {value += _("boolean"); break;}
						case ASSUMPTION_TYPE_RATIONAL: {value += _("rational"); break;}
						case ASSUMPTION_TYPE_REAL: {value += _("real"); break;}
						case ASSUMPTION_TYPE_COMPLEX: {value += _("complex"); break;}
						case ASSUMPTION_TYPE_NUMBER: {value += _("number"); break;}
						case ASSUMPTION_TYPE_NONMATRIX: {value += _("not matrix"); break;}
						default: {}
					}
					if(value.empty()) value = _("unknown");
					str += value;
				} else {
					str += _("Default assumptions");
				}
			}
			if(!v->description().empty()) {
				str += "\n\n";
				str += v->description();
			}
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_edit")), !v->isBuiltin() && !is_answer_variable(v) && !is_memory_variable(v));
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_insert")), v->isActive());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_deactivate")), !is_answer_variable(v) && !is_memory_variable(v));
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_export")), v->isKnown());
			if(v->isActive()) {
				gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(variables_builder, "variables_buttonlabel_deactivate")), _("Deacti_vate"));
			} else {
				gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(variables_builder, "variables_buttonlabel_deactivate")), _("Acti_vate"));
			}
			//user cannot delete global definitions
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_delete")), v->isLocal() && !is_answer_variable(v) && !is_memory_variable(v) && v != CALCULATOR->v_x && v != CALCULATOR->v_y && v != CALCULATOR->v_z);
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_edit")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_insert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_delete")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_deactivate")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_button_export")), FALSE);
		selected_variable = NULL;
	}
}

void on_variables_entry_search_changed(GtkEntry *w, gpointer) {
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariables_selection_changed, NULL);
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tVariables_store), &iter)) return;
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	do {
		bool b = str.empty();
		Variable *u = NULL;
		if(!b) gtk_tree_model_get(GTK_TREE_MODEL(tVariables_store), &iter, 1, &u, -1);
		if(u) b = name_matches(u, str) || title_matches(u, str);
		gtk_list_store_set(tVariables_store, &iter, 2, b, -1);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tVariables_store), &iter));
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tVariables_selection_changed, NULL);
	if(str.empty()) {
		gtk_widget_grab_focus(tVariables);
	} else {
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tVariables_store_filter), &iter)) {
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)));
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &iter);
			GtkTreePath *path = gtk_tree_model_get_path(tVariables_store_filter, &iter);
			if(path) {
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(tVariables), path, NULL, FALSE);
				gtk_tree_path_free(path);
			}
		}
	}
}

/*
	"New" button clicked in variable manager -- open new variable dialog
*/
void on_variables_button_new_clicked(GtkButton*, gpointer) {
	if(selected_variable_category.empty() || selected_variable_category[0] != '/') {
		edit_variable(NULL, NULL, NULL, GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")));
	} else {
		//fill in category field with selected category
		edit_variable(selected_variable_category.substr(1, selected_variable_category.length() - 1).c_str(), NULL, NULL, GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")));
	}
}

/*
	"Edit" button clicked in variable manager -- open edit dialog for selected variable
*/
void on_variables_button_edit_clicked(GtkButton*, gpointer) {
	Variable *v = get_selected_variable();
	if(v) {
		if(!CALCULATOR->stillHasVariable(v)) {
			show_message(_("Variable does not exist anymore."), GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")));
			update_vmenu();
			return;
		}
		edit_variable(NULL, v, NULL, GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")));
	}
}

/*
	"Insert" button clicked in variable manager -- insert variable name in expression entry
*/
void on_variables_button_insert_clicked(GtkButton*, gpointer) {
	Variable *v = get_selected_variable();
	if(v) {
		if(!CALCULATOR->stillHasVariable(v)) {
			show_message(_("Variable does not exist anymore."), GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")));
			update_vmenu();
			return;
		}
		gchar *gstr = g_strdup(v->preferredInputName(printops.abbreviate_names, true, false, false, &can_display_unicode_string_function, (void*) expression_edit_widget()).formattedName(TYPE_VARIABLE, true).c_str());
		insert_text(gstr);
		g_free(gstr);
	}
}

void on_variables_button_delete_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	Variable *v = get_selected_variable();
	if(!v) return;
	if(!CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."), GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")));
		update_vmenu();
		return;
	}
	if(!v->isLocal()) return;
	v->destroy();
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), &model, &iter)) {
		//reselect selected variable category
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		string str = selected_variable_category;
		variable_removed(v);
		if(str == selected_variable_category) gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables)), path);
		gtk_tree_path_free(path);
	} else {
		variable_removed(v);
	}
}

void on_variables_button_export_clicked(GtkButton*, gpointer) {
	Variable *v = get_selected_variable();
	if(v && !CALCULATOR->stillHasVariable(v)) {
		show_message(_("Variable does not exist anymore."), GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")));
		update_vmenu();
		return;
	}
	if(v && v->isKnown()) {
		export_csv_file(GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")), (KnownVariable*) v);
	}
}

/*
	"Close" button clicked in variable manager -- hide
*/
void on_variables_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));
}

gboolean on_variables_dialog_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(gtk_widget_has_focus(GTK_WIDGET(tVariables)) && gdk_keyval_to_unicode(event->keyval) > 32) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_entry_search")));
	}
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_entry_search")))) {
		if(event->keyval == GDK_KEY_Escape) {
			gtk_widget_hide(o);
			return TRUE;
		}
		if(event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_Page_Up || event->keyval == GDK_KEY_Page_Down || event->keyval == GDK_KEY_KP_Page_Up || event->keyval == GDK_KEY_KP_Page_Down) {
			gtk_widget_grab_focus(GTK_WIDGET(tVariables));
		}
	}
	return FALSE;
}
void on_variables_button_deactivate_clicked(GtkButton*, gpointer) {
	Variable *v = get_selected_variable();
	if(v) {
		v->setActive(!v->isActive());
		update_vmenu();
	}
}
GtkWidget* get_variables_dialog(void) {
	if(!variables_builder) {

		variables_builder = getBuilder("variables.ui");
		g_assert(variables_builder != NULL);

		selected_variable_category = _("All");
		selected_variable = NULL;

		g_assert(gtk_builder_get_object(variables_builder, "variables_dialog") != NULL);

		tVariableCategories = GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_treeview_category"));
		tVariables = GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_treeview_variable"));

		tVariables_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);
		tVariables_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tVariables_store), NULL);
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tVariables_store_filter), 2);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tVariables), GTK_TREE_MODEL(tVariables_store_filter));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariables));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Variable"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tVariables), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tVariables_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tVariables_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tVariables_store), 0, GTK_SORT_ASCENDING);

		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tVariables), FALSE);

		tVariableCategories_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tVariableCategories), GTK_TREE_MODEL(tVariableCategories_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tVariableCategories), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tVariableCategories_selection_changed), NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tVariableCategories_store), 0, category_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tVariableCategories_store), 0, GTK_SORT_ASCENDING);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(variables_builder, "variables_textview_description")));
		gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);

		if(variables_width > 0 && variables_height > 0) {
			gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")), variables_width, variables_height);
			if(variables_vposition <= 0) variables_vposition = variables_height / 3 * 2;
		}
		if(variables_hposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(variables_builder, "variables_hpaned")), variables_hposition);
		if(variables_vposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(variables_builder, "variables_vpaned")), variables_vposition);

		gtk_builder_add_callback_symbols(variables_builder, "on_variables_dialog_key_press_event", G_CALLBACK(on_variables_dialog_key_press_event), "on_variables_entry_search_changed", G_CALLBACK(on_variables_entry_search_changed), "on_variables_button_new_clicked", G_CALLBACK(on_variables_button_new_clicked), "on_variables_button_edit_clicked", G_CALLBACK(on_variables_button_edit_clicked), "on_variables_button_delete_clicked", G_CALLBACK(on_variables_button_delete_clicked), "on_variables_button_deactivate_clicked", G_CALLBACK(on_variables_button_deactivate_clicked), "on_variables_button_insert_clicked", G_CALLBACK(on_variables_button_insert_clicked), "on_variables_button_export_clicked", G_CALLBACK(on_variables_button_export_clicked), NULL);
		gtk_builder_connect_signals(variables_builder, NULL);

		update_variables_tree();

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_dialog"));
}

void manage_variables(GtkWindow *parent, const gchar *str) {
	GtkWidget *dialog = get_variables_dialog();
	if(!gtk_widget_is_visible(dialog)) {
		gtk_widget_grab_focus(tVariables);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variables_builder, "variables_entry_search")), "");
		gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
		gtk_widget_show(dialog);
		fix_deactivate_label_width(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_buttonlabel_deactivate")));
	}
	if(str) {
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tVariableCategories_store), &iter)) {
			GtkTreeIter iter2 = iter;
			while(!gtk_tree_model_iter_has_child(GTK_TREE_MODEL(tVariableCategories_store), &iter) && gtk_tree_model_iter_next(GTK_TREE_MODEL(tVariableCategories_store), &iter2)) {
				iter = iter2;
			}
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tVariableCategories)), &iter);
		}
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(variables_builder, "variables_entry_search")), str);
	}
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}
void update_variables_settings() {
	if(variables_builder) {
		gint w = 0, h = 0;
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(variables_builder, "variables_dialog")), &w, &h);
		variables_width = w;
		variables_height = h;
		variables_hposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(variables_builder, "variables_hpaned")));
		variables_vposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(variables_builder, "variables_vpaned")));
	}
}
void variables_font_updated() {
	if(variables_builder) fix_deactivate_label_width(GTK_WIDGET(gtk_builder_get_object(variables_builder, "variables_buttonlabel_deactivate")));
}
