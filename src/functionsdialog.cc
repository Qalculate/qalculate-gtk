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
#include "insertfunctiondialog.h"
#include "functioneditdialog.h"
#include "functionsdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *functions_builder = NULL;
GtkWidget *tFunctionCategories;
GtkWidget *tFunctions;
GtkListStore *tFunctions_store;
GtkTreeModel *tFunctions_store_filter;
GtkTreeStore *tFunctionCategories_store;

string selected_function_category;
MathFunction *selected_function;
gint functions_width = -1, functions_height = -1, functions_hposition = -1, functions_vposition = -1;

void on_tFunctionCategories_selection_changed(GtkTreeSelection *treeselection, gpointer);
void on_tFunctions_selection_changed(GtkTreeSelection *treeselection, gpointer);
void on_functions_entry_search_changed(GtkEntry *w, gpointer);

bool read_functions_dialog_settings_line(string &svar, string&, int &v) {
	if(svar == "functions_width") {
		functions_width = v;
	} else if(svar == "functions_height") {
		functions_height = v;
	} else if(svar == "functions_hpanel_position") {
		functions_hposition = v;
	} else if(svar == "functions_vpanel_position") {
		functions_vposition = v;
	} else {
		return false;
	}
	return true;
}
void write_functions_dialog_settings(FILE *file) {
	if(functions_width > -1) fprintf(file, "functions_width=%i\n", functions_width);
	if(functions_height > -1) fprintf(file, "functions_height=%i\n", functions_height);
	if(functions_hposition > -1) fprintf(file, "functions_hpanel_position=%i\n", functions_hposition);
	if(functions_vposition > -1) fprintf(file, "functions_vpanel_position=%i\n", functions_vposition);
}

MathFunction *get_selected_function() {
	return selected_function;
}

void update_functions_tree() {
	if(!functions_builder) return;
	GtkTreeIter iter, iter2, iter3;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tFunctionCategories));
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctionCategories_selection_changed, NULL);
	gtk_tree_store_clear(tFunctionCategories_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctionCategories_selection_changed, NULL);
	gtk_tree_store_append(tFunctionCategories_store, &iter3, NULL);
	gtk_tree_store_set(tFunctionCategories_store, &iter3, 0, _("All"), 1, _("All"), -1);
	string str;
	tree_struct *item, *item2;
	function_cats.it = function_cats.items.begin();
	if(function_cats.it != function_cats.items.end()) {
		item = &*function_cats.it;
		++function_cats.it;
		item->it = item->items.begin();
	} else {
		item = NULL;
	}
	str = "";
	iter2 = iter3;
	while(item) {
		gtk_tree_store_append(tFunctionCategories_store, &iter, &iter2);
		str += "/";
		str += item->item;
		gtk_tree_store_set(tFunctionCategories_store, &iter, 0, item->item.c_str(), 1, str.c_str(), -1);
		if(str == selected_function_category) {
			EXPAND_TO_ITER(model, tFunctionCategories, iter)
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
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
	EXPAND_ITER(model, tFunctionCategories, iter3)
	if(!user_functions.empty()) {
		gtk_tree_store_append(tFunctionCategories_store, &iter, NULL);
		EXPAND_TO_ITER(model, tFunctionCategories, iter)
		gtk_tree_store_set(tFunctionCategories_store, &iter, 0, _("User functions"), 1, _("User functions"), -1);
		if(selected_function_category == _("User functions")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
		}
	}
	if(!function_cats.objects.empty()) {
		//add "Uncategorized" category if there are functions without category
		gtk_tree_store_append(tFunctionCategories_store, &iter, &iter3);
		EXPAND_TO_ITER(model, tFunctionCategories, iter)
		gtk_tree_store_set(tFunctionCategories_store, &iter, 0, _("Uncategorized"), 1, _("Uncategorized"), -1);
		if(selected_function_category == _("Uncategorized")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
		}
	}
	if(!ia_functions.empty()) {
		//add "Inactive" category if there are inactive functions
		gtk_tree_store_append(tFunctionCategories_store, &iter, NULL);
		EXPAND_TO_ITER(model, tFunctionCategories, iter)
		gtk_tree_store_set(tFunctionCategories_store, &iter, 0, _("Inactive"), 1, _("Inactive"), -1);
		if(selected_function_category == _("Inactive")) {
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
		}
	}
	if(!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &model, &iter)) {
		//if no category has been selected (previously selected has been renamed/deleted), select "All"
		selected_function_category = _("All");
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter3);
	}
}

void setFunctionTreeItem(GtkTreeIter &iter2, MathFunction *f) {
	gtk_list_store_append(tFunctions_store, &iter2);
	gtk_list_store_set(tFunctions_store, &iter2, 0, f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) tFunctions).c_str(), 1, (gpointer) f, 2, TRUE, -1);
	GtkTreeIter iter;
	if(f == selected_function && gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(tFunctions_store_filter), &iter, &iter2)) {
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &iter);
	}
}

void on_tFunctionCategories_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model, *model2;
	GtkTreeIter iter, iter2;
	bool no_cat = false, b_all = false, b_inactive = false, b_user = false;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_functions_entry_search_changed, NULL);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functions_builder, "functions_entry_search")), "");
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_functions_entry_search_changed, NULL);
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctions_selection_changed, NULL);
	gtk_list_store_clear(tFunctions_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctions_selection_changed, NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_edit")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_insert")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_delete")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_deactivate")), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_apply")), FALSE);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr;
		gtk_tree_model_get(model, &iter, 1, &gstr, -1);
		selected_function_category = gstr;
		if(selected_function_category == _("All")) {
			b_all = true;
		} else if(selected_function_category == _("Uncategorized")) {
			no_cat = true;
		} else if(selected_function_category == _("Inactive")) {
			b_inactive = true;
		} else if(selected_function_category == _("User functions")) {
			b_user = true;
		}
		if(!b_user && !b_all && !no_cat && !b_inactive && selected_function_category[0] == '/') {
			string str = selected_function_category.substr(1, selected_function_category.length() - 1);
			ExpressionItem *o;
			size_t l1 = str.length(), l2;
			for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
				o = CALCULATOR->functions[i];
				l2 = o->category().length();
				if(o->isActive() && (l2 == l1 || (l2 > l1 && o->category()[l1] == '/')) && o->category().substr(0, l1) == str) {
					setFunctionTreeItem(iter2, CALCULATOR->functions[i]);
				}
			}
		} else {
			for(size_t i = 0; i < CALCULATOR->functions.size(); i++) {
				if((b_inactive && !CALCULATOR->functions[i]->isActive()) || (CALCULATOR->functions[i]->isActive() && (b_all || (no_cat && CALCULATOR->functions[i]->category().empty()) || (b_user && CALCULATOR->functions[i]->isLocal()) || (!b_inactive && CALCULATOR->functions[i]->category() == selected_function_category)))) {
					setFunctionTreeItem(iter2, CALCULATOR->functions[i]);
				}
			}
		}
		if(!selected_function || !gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &model2, &iter2)) {
			if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctions_store_filter), &iter2)) {
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &iter2);
			}
		}
		g_free(gstr);
	} else {
		selected_function_category = "";
	}
}

void on_tFunctions_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		MathFunction *f;
		gtk_tree_model_get(model, &iter, 1, &f, -1);
		//remember the new selection
		selected_function = f;
		if(CALCULATOR->stillHasFunction(f)) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functions_builder, "functions_textview_description")));
			gtk_text_buffer_set_text(buffer, "", -1);
			GtkTextIter iter;
			Argument *arg;
			Argument default_arg;
			string str, str2;
			const ExpressionName *ename = &f->preferredName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(functions_builder, "functions_textview_description"));
			str += ename->formattedName(TYPE_FUNCTION, true);
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", "italic", NULL);
			str = "";
			int iargs = f->maxargs();
			if(iargs < 0) {
				iargs = f->minargs() + 1;
				if((int) f->lastArgumentDefinitionIndex() > iargs) iargs = (int) f->lastArgumentDefinitionIndex();
			}
			str += "(";
			if(iargs != 0) {
				for(int i2 = 1; i2 <= iargs; i2++) {
					if(i2 > f->minargs()) {
						str += "[";
					}
					if(i2 > 1) {
						str += CALCULATOR->getComma();
						str += " ";
					}
					arg = f->getArgumentDefinition(i2);
					if(arg && !arg->name().empty()) {
						str2 = arg->name();
					} else {
						str2 = _("argument");
						if(i2 > 1 || f->maxargs() != 1) {
							str2 += " ";
							str2 += i2s(i2);
						}
					}
					str += str2;
					if(i2 > f->minargs()) {
						str += "]";
					}
				}
				if(f->maxargs() < 0) {
					str += CALCULATOR->getComma();
					str += " …";
				}
			}
			str += ")";
			for(size_t i2 = 1; i2 <= f->countNames(); i2++) {
				if(&f->getName(i2) != ename) {
					str += "\n";
					str += f->getName(i2).formattedName(TYPE_FUNCTION, true);
				}
			}
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "italic", NULL);
			str = "";
			str += "\n";
			if(f->subtype() == SUBTYPE_DATA_SET) {
				str += "\n";
				gchar *gstr = g_strdup_printf(_("Retrieves data from the %s data set for a given object and property. If \"info\" is typed as property, a dialog window will pop up with all properties of the object."), f->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) gtk_builder_get_object(functions_builder, "functions_textview_description")).c_str());
				str += gstr;
				g_free(gstr);
				str += "\n";
			}
			if(!f->description().empty()) {
				str += "\n";
				str += f->description();
				str += "\n";
			}
			if(!f->example(true).empty()) {
				str += "\n";
				str += _("Example:");
				str += " ";
				str += f->example(false, ename->formattedName(TYPE_FUNCTION, true));
				str += "\n";
			}
			if(f->subtype() == SUBTYPE_DATA_SET && !((DataSet*) f)->copyright().empty()) {
				str += "\n";
				str += ((DataSet*) f)->copyright();
				str += "\n";
			}
			if(printops.use_unicode_signs) {
				gsub(">=", SIGN_GREATER_OR_EQUAL, str);
				gsub("<=", SIGN_LESS_OR_EQUAL, str);
				gsub("!=", SIGN_NOT_EQUAL, str);
				gsub("...", "…", str);
			}
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
			if(iargs) {
				str = "\n";
				str += _("Arguments");
				str += "\n";
				gtk_text_buffer_get_end_iter(buffer, &iter);
				gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
				for(int i2 = 1; i2 <= iargs; i2++) {
					arg = f->getArgumentDefinition(i2);
					if(arg && !arg->name().empty()) {
						str = arg->name();
					} else {
						str = i2s(i2);
					}
					str += ": ";
					if(arg) {
						str2 = arg->printlong();
					} else {
						str2 = default_arg.printlong();
					}
					if(printops.use_unicode_signs) {
						gsub(">=", SIGN_GREATER_OR_EQUAL, str2);
						gsub("<=", SIGN_LESS_OR_EQUAL, str2);
						gsub("!=", SIGN_NOT_EQUAL, str2);
						gsub("...", "…", str2);
					}
					if(i2 > f->minargs()) {
						str2 += " (";
						//optional argument
						str2 += _("optional");
						if(!f->getDefaultValue(i2).empty() && f->getDefaultValue(i2) != "\"\"") {
							str2 += ", ";
							//argument default, in description
							str2 += _("default: ");
							str2 += localize_expression(f->getDefaultValue(i2));
						}
						str2 += ")";
					}
					str2 += "\n";
					gtk_text_buffer_get_end_iter(buffer, &iter);
					gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
					gtk_text_buffer_get_end_iter(buffer, &iter);
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str2.c_str(), -1, "italic", NULL);
				}
			}
			if(!f->condition().empty()) {
				str = "\n";
				str += _("Requirement");
				str += ": ";
				str += f->printCondition();
				if(printops.use_unicode_signs) {
					gsub(">=", SIGN_GREATER_OR_EQUAL, str);
					gsub("<=", SIGN_LESS_OR_EQUAL, str);
					gsub("!=", SIGN_NOT_EQUAL, str);
				}
				str += "\n";
				gtk_text_buffer_get_end_iter(buffer, &iter);
				gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
			}
			if(f->subtype() == SUBTYPE_DATA_SET) {
				DataSet *ds = (DataSet*) f;
				str = "\n";
				str += _("Properties");
				str += "\n";
				gtk_text_buffer_get_end_iter(buffer, &iter);
				gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
				DataPropertyIter it;
				DataProperty *dp = ds->getFirstProperty(&it);
				while(dp) {
					if(!dp->isHidden()) {
						if(!dp->title(false).empty()) {
							str = dp->title();
							str += ": ";
						} else {
							str = "";
						}
						for(size_t i = 1; i <= dp->countNames(); i++) {
							if(i > 1) str += ", ";
							str += dp->getName(i);
						}
						if(dp->isKey()) {
							str += " (";
							//indicating that the property is a data set key
							str += _("key");
							str += ")";
						}
						str += "\n";
						gtk_text_buffer_get_end_iter(buffer, &iter);
						gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
						if(!dp->description().empty()) {
							str = dp->description();
							str += "\n";
							gtk_text_buffer_get_end_iter(buffer, &iter);
							gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "italic", NULL);
						}
					}
					dp = ds->getNextProperty(&it);
				}
			}
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_edit")), !f->isBuiltin());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_deactivate")), TRUE);
			if(f->isActive()) {
				gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(functions_builder, "functions_buttonlabel_deactivate")), _("Deacti_vate"));
			} else {
				gtk_label_set_text_with_mnemonic(GTK_LABEL(gtk_builder_get_object(functions_builder, "functions_buttonlabel_deactivate")), _("Acti_vate"));
			}
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_insert")), f->isActive());
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_apply")), f->isActive() && ((f->minargs() <= 1 && f != CALCULATOR->f_logn) || rpn_mode));
			//user cannot delete global definitions
			gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_delete")), f->isLocal());
		}
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_edit")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_insert")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_delete")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_button_deactivate")), FALSE);
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functions_builder, "functions_textview_description"))), "", -1);
		selected_function = NULL;
	}
}

/*
	"New" button clicked in function manager -- open new function dialog
*/
void on_functions_button_new_clicked(GtkButton*, gpointer) {
	if(selected_function_category.empty() || selected_function_category[0] != '/') {
		edit_function("", NULL, GTK_WINDOW(gtk_builder_get_object(functions_builder, "functions_dialog")));
	} else {
		//fill in category field with selected category
		edit_function(selected_function_category.substr(1, selected_function_category.length() - 1).c_str(), NULL, GTK_WINDOW(gtk_builder_get_object(functions_builder, "functions_dialog")));
	}
}

void on_functions_button_edit_clicked(GtkButton*, gpointer) {
	MathFunction *f = get_selected_function();
	if(f) {
		edit_function("", f, GTK_WINDOW(gtk_builder_get_object(functions_builder, "functions_dialog")));
	}
}

void on_functions_button_insert_clicked(GtkButton*, gpointer) {
	insert_function(get_selected_function(), GTK_WINDOW(gtk_builder_get_object(functions_builder, "functions_dialog")));
}

void on_functions_button_apply_clicked(GtkButton*, gpointer) {
	apply_function(get_selected_function());
}

void on_functions_button_delete_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	MathFunction *f = get_selected_function();
	if(!f || !f->isLocal()) return;
	//ensure removal of all references in Calculator
	f->destroy();
	//update menus and trees
	if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &model, &iter)) {
		//reselected selected function category
		GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
		string str = selected_function_category;
		function_removed(f);
		if(str == selected_function_category) {
			gtk_tree_selection_select_path(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), path);
		}
		gtk_tree_path_free(path);
	} else {
		function_removed(f);
	}
}

void on_functions_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog")));
}

void on_functions_button_deactivate_clicked(GtkButton*, gpointer) {
	MathFunction *f = get_selected_function();
	if(f) {
		f->setActive(!f->isActive());
		update_fmenu();
	}
}

gboolean on_functions_dialog_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	guint keyval = 0;
	gdk_event_get_keyval((GdkEvent*) event, &keyval);
	if(gtk_widget_has_focus(GTK_WIDGET(tFunctions)) && gdk_keyval_to_unicode(keyval) > 32) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_entry_search")));
	}
	if(gtk_widget_has_focus(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_entry_search")))) {
		if(keyval == GDK_KEY_Escape) {
			gtk_widget_hide(o);
			return TRUE;
		}
		if(keyval == GDK_KEY_Up || keyval == GDK_KEY_Down || keyval == GDK_KEY_Page_Up || keyval == GDK_KEY_Page_Down || keyval == GDK_KEY_KP_Page_Up || keyval == GDK_KEY_KP_Page_Down) {
			gtk_widget_grab_focus(GTK_WIDGET(tFunctions));
		}
	}
	return FALSE;
}

void on_functions_entry_search_changed(GtkEntry *w, gpointer) {
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctions_selection_changed, NULL);
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctions_store), &iter)) return;
	string str = gtk_entry_get_text(w);
	remove_blank_ends(str);
	do {
		bool b = str.empty();
		MathFunction *u = NULL;
		if(!b) gtk_tree_model_get(GTK_TREE_MODEL(tFunctions_store), &iter, 1, &u, -1);
		if(u) b = name_matches(u, str) || title_matches(u, str);
		gtk_list_store_set(tFunctions_store, &iter, 2, b, -1);
	} while(gtk_tree_model_iter_next(GTK_TREE_MODEL(tFunctions_store), &iter));
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tFunctions_selection_changed, NULL);
	if(str.empty()) {
		gtk_widget_grab_focus(tFunctions);
	} else {
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctions_store_filter), &iter)) {
			gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)));
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions)), &iter);
			GtkTreePath *path = gtk_tree_model_get_path(tFunctions_store_filter, &iter);
			if(path) {
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(tFunctions), path, NULL, FALSE);
				gtk_tree_path_free(path);
			}
		}
	}
}

GtkWidget* get_functions_dialog(void) {

	if(!functions_builder) {

		functions_builder =  getBuilder("functions.ui");
		g_assert(functions_builder != NULL);

		selected_function_category = _("All");
		selected_function = NULL;

		g_assert(gtk_builder_get_object(functions_builder, "functions_dialog") != NULL);

		tFunctionCategories = GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_treeview_category"));
		tFunctions = GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_treeview_function"));

		tFunctions_store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);
		tFunctions_store_filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(tFunctions_store), NULL);
		gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(tFunctions_store_filter), 2);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tFunctions), GTK_TREE_MODEL(tFunctions_store_filter));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctions));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Function"), renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctions), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tFunctions_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tFunctions_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tFunctions_store), 0, GTK_SORT_ASCENDING);

		gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tFunctions), FALSE);

		tFunctionCategories_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tFunctionCategories), GTK_TREE_MODEL(tFunctionCategories_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Category"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tFunctionCategories), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tFunctionCategories_selection_changed), NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tFunctionCategories_store), 0, category_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tFunctionCategories_store), 0, GTK_SORT_ASCENDING);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(functions_builder, "functions_textview_description")));
		gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);

		if(functions_width > 0 && functions_height > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(functions_builder, "functions_dialog")), functions_width, functions_height);
		if(functions_hposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(functions_builder, "functions_hpaned")), functions_hposition);
		if(functions_vposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(functions_builder, "functions_vpaned")), functions_vposition);

		gtk_builder_add_callback_symbols(functions_builder, "on_functions_dialog_key_press_event", G_CALLBACK(on_functions_dialog_key_press_event), "on_functions_entry_search_changed", G_CALLBACK(on_functions_entry_search_changed), "on_functions_button_new_clicked", G_CALLBACK(on_functions_button_new_clicked), "on_functions_button_edit_clicked", G_CALLBACK(on_functions_button_edit_clicked), "on_functions_button_delete_clicked", G_CALLBACK(on_functions_button_delete_clicked), "on_functions_button_deactivate_clicked", G_CALLBACK(on_functions_button_deactivate_clicked), "on_functions_button_insert_clicked", G_CALLBACK(on_functions_button_insert_clicked), "on_functions_button_apply_clicked", G_CALLBACK(on_functions_button_apply_clicked), NULL);
		gtk_builder_connect_signals(functions_builder, NULL);

		update_functions_tree();

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_dialog"));
}

void manage_functions(GtkWindow *parent, const gchar *str) {
	GtkWidget *dialog = get_functions_dialog();
	if(!gtk_widget_is_visible(dialog)) {
		gtk_widget_grab_focus(tFunctions);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functions_builder, "functions_entry_search")), "");
		gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
		gtk_widget_show(dialog);
		fix_deactivate_label_width(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_buttonlabel_deactivate")));
	}
	if(str) {
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tFunctionCategories_store), &iter)) {
			GtkTreeIter iter2 = iter;
			while(!gtk_tree_model_iter_has_child(GTK_TREE_MODEL(tFunctionCategories_store), &iter) && gtk_tree_model_iter_next(GTK_TREE_MODEL(tFunctionCategories_store), &iter2)) {
				iter = iter2;
			}
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tFunctionCategories)), &iter);
		}
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(functions_builder, "functions_entry_search")), str);
	}
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}

void update_functions_settings() {
	if(functions_builder) {
		int w = 0, h = 0;
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(functions_builder, "functions_dialog")), &w, &h);
		functions_width = w;
		functions_height = h;
		functions_hposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(functions_builder, "functions_hpaned")));
		functions_vposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(functions_builder, "functions_vpaned")));
	}
}
void functions_font_updated() {
	if(functions_builder) fix_deactivate_label_width(GTK_WIDGET(gtk_builder_get_object(functions_builder, "functions_buttonlabel_deactivate")));
}
