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
#include "dataseteditdialog.h"
#include "datasetsdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *datasets_builder = NULL;
GtkWidget *tDatasets;
GtkWidget *tDataObjects;
GtkListStore *tDatasets_store;
GtkListStore *tDataObjects_store;

DataObject *selected_dataobject = NULL;
DataSet *selected_dataset = NULL;

gint datasets_width = -1, datasets_height = -1, datasets_hposition = -1, datasets_vposition1 = -1, datasets_vposition2 = -1;

void on_tDataObjects_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);
void on_tDatasets_selection_changed(GtkTreeSelection *treeselection, gpointer user_data);
void edit_dataobject(DataSet *ds, DataObject *o = NULL, GtkWidget *win = NULL);

void update_datasets_tree() {
	if(!datasets_builder) return;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDatasets));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDatasets_selection_changed, NULL);
	gtk_list_store_clear(tDatasets_store);
	DataSet *ds;
	bool b = false;
	for(size_t i = 1; ; i++) {
		ds = CALCULATOR->getDataSet(i);
		if(!ds) break;
		gtk_list_store_append(tDatasets_store, &iter);
		gtk_list_store_set(tDatasets_store, &iter, 0, ds->title().c_str(), 1, (gpointer) ds, -1);
		if(ds == selected_dataset) {
			g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDatasets_selection_changed, NULL);
			gtk_tree_selection_select_iter(select, &iter);
			g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDatasets_selection_changed, NULL);
			b = true;
		}
	}
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDatasets_selection_changed, NULL);
	if(!b) {
		gtk_tree_selection_unselect_all(select);
		selected_dataset = NULL;
	}
}

void on_tDatasets_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model, *model2;
	GtkTreeIter iter, iter2;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects));
	g_signal_handlers_block_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDataObjects_selection_changed, NULL);
	gtk_tree_selection_unselect_all(select);
	gtk_list_store_clear(tDataObjects_store);
	g_signal_handlers_unblock_matched((gpointer) select, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, (gpointer) on_tDataObjects_selection_changed, NULL);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		DataSet *ds = NULL;
		gtk_tree_model_get(model, &iter, 1, &ds, -1);
		selected_dataset = ds;
		if(!ds) return;
		DataObjectIter it;
		DataPropertyIter pit;
		DataProperty *dp;
		DataObject *o = ds->getFirstObject(&it);
		bool b = false;
		while(o) {
			b = true;
			gtk_list_store_append(tDataObjects_store, &iter2);
			dp = ds->getFirstProperty(&pit);
			size_t index = 0;
			while(dp) {
				if(!dp->isHidden() && dp->isKey()) {
					gtk_list_store_set(tDataObjects_store, &iter2, index, o->getPropertyDisplayString(dp).c_str(), -1);
					index++;
					if(index > 2) break;
				}
				dp = ds->getNextProperty(&pit);
			}
			while(index < 3) {
				gtk_list_store_set(tDataObjects_store, &iter2, index, "", -1);
				index++;
			}
			gtk_list_store_set(tDataObjects_store, &iter2, 3, (gpointer) o, -1);
			if(o == selected_dataobject) {
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects)), &iter2);
			}
			o = ds->getNextObject(&it);
		}
		if(b && (!selected_dataobject || !gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects)), &model2, &iter2))) {
			gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tDataObjects_store), &iter2);
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects)), &iter2);
		}
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasets_builder, "datasets_textview_description")));
		gtk_text_buffer_set_text(buffer, "", -1);
		GtkTextIter iter;
		string str, str2;
		if(!ds->description().empty()) {
			str = ds->description();
			str += "\n";
			str += "\n";
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
		}
		str = _("Properties");
		str += "\n";
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
		dp = ds->getFirstProperty(&pit);
		while(dp) {
			if(!dp->isHidden()) {
				str = "";
				if(!dp->title(false).empty()) {
					str += dp->title();
					str += ": ";
				}
				for(size_t i = 1; i <= dp->countNames(); i++) {
					if(i > 1) str += ", ";
					str += dp->getName(i);
				}
				if(dp->isKey()) {
					str += " (";
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
			dp = ds->getNextProperty(&pit);
		}
		str = "\n";
		str += _("Data Retrieval Function");
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", NULL);
		Argument *arg;
		Argument default_arg;
		const ExpressionName *ename = &ds->preferredName(false, true, false, false, &can_display_unicode_string_function, (void*) gtk_builder_get_object(datasets_builder, "datasets_textview_description"));
		str = "\n";
		str += ename->formattedName(TYPE_FUNCTION, true);
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "bold", "italic", NULL);
		str = "";
		int iargs = ds->maxargs();
		if(iargs < 0) {
			iargs = ds->minargs() + 1;
			if((int) ds->lastArgumentDefinitionIndex() > iargs) iargs = (int) ds->lastArgumentDefinitionIndex();
		}
		str += "(";
		if(iargs != 0) {
			for(int i2 = 1; i2 <= iargs; i2++) {
				if(i2 > ds->minargs()) {
					str += "[";
				}
				if(i2 > 1) {
					str += CALCULATOR->getComma();
					str += " ";
				}
				arg = ds->getArgumentDefinition(i2);
				if(arg && !arg->name().empty()) {
					str2 = arg->name();
				} else {
					str2 = _("argument");
					if(i2 > 1 || ds->maxargs() != 1) {
						str2 += " ";
						str2 += i2s(i2);
					}
				}
				str += str2;
				if(i2 > ds->minargs()) {
					str += "]";
				}
			}
			if(ds->maxargs() < 0) {
				str += CALCULATOR->getComma();
				str += " …";
			}
		}
		str += ")";
		for(size_t i2 = 1; i2 <= ds->countNames(); i2++) {
			if(&ds->getName(i2) != ename) {
				str += "\n";
				str += ds->getName(i2).formattedName(TYPE_FUNCTION, true);
			}
		}
		str += "\n\n";
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, str.c_str(), -1, "italic", NULL);
		if(!ds->copyright().empty()) {
			str = "\n";
			str = ds->copyright();
			str += "\n";
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert(buffer, &iter, str.c_str(), -1);
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editset")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delset")), ds->isLocal());
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_newobject")), TRUE);
	} else {
		gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasets_builder, "datasets_textview_description"))), "", -1);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editset")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delset")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_newobject")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editobject")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delobject")), FALSE);
		selected_dataset = NULL;
	}
}

void update_dataobjects() {
	on_tDatasets_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDatasets)), NULL);
}

void on_dataset_button_function_clicked(GtkButton *w, gpointer user_data) {
	DataProperty *dp = (DataProperty*) user_data;
	DataObject *o = selected_dataobject;
	DataSet *ds = NULL;
	if(o) ds = dp->parentSet();
	if(ds && o) {
		string str = ds->preferredDisplayName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) w).name;
		str += "(";
		str += o->getProperty(ds->getPrimaryKeyProperty());
		str += CALCULATOR->getComma();
		str += " ";
		str += dp->getName();
		str += ")";
		insert_text(str.c_str());
	}
}
void on_tDataObjects_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *ptable = GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_grid_properties"));
	GList *childlist = gtk_container_get_children(GTK_CONTAINER(ptable));
	for(guint i = 0; ; i++) {
		GtkWidget *w = (GtkWidget*) g_list_nth_data(childlist, i);
		if(!w) break;
		gtk_widget_destroy(w);
	}
	g_list_free(childlist);
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		DataObject *o = NULL;
		gtk_tree_model_get(model, &iter, 3, &o, -1);
		selected_dataobject = o;
		if(!o) return;
		DataSet *ds = o->parentSet();
		if(!ds) return;
		DataPropertyIter it;
		DataProperty *dp = ds->getFirstProperty(&it);
		string sval;
		int rows = 1;
		gtk_grid_remove_column(GTK_GRID(ptable), 0);
		gtk_grid_remove_column(GTK_GRID(ptable), 1);
		gtk_grid_remove_column(GTK_GRID(ptable), 2);
		GtkWidget *button, *label;
		string str;
		while(dp) {
			if(!dp->isHidden()) {
				sval = o->getPropertyDisplayString(dp);
				if(!sval.empty()) {
					label = gtk_label_new(NULL);
					str = "<span weight=\"bold\">"; str += dp->title(); str += ":"; str += "</span>";
					gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), FALSE);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
					gtk_widget_set_margin_end(label, 20);
#else
					gtk_widget_set_margin_right(label, 20);
#endif
					gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1 , 1);
					label = gtk_label_new(NULL);
					gtk_widget_set_hexpand(label, TRUE);
					gtk_label_set_markup(GTK_LABEL(label), sval.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
					gtk_grid_attach(GTK_GRID(ptable), label, 1, rows - 1, 1, 1);
					button = gtk_button_new();
					gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("edit-paste", GTK_ICON_SIZE_BUTTON));
					gtk_widget_set_halign(button, GTK_ALIGN_END);
					//gtk_widget_set_valign(button, GTK_ALIGN_CENTER);
					gtk_grid_attach(GTK_GRID(ptable), button, 2, rows - 1, 1, 1);
					g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_dataset_button_function_clicked), (gpointer) dp);
					rows++;
				}
			}
			dp = ds->getNextProperty(&it);
		}
		gtk_widget_show_all(ptable);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editobject")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delobject")), o->isUserModified());
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_editobject")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_button_delobject")), FALSE);
		selected_dataobject = NULL;
	}
}

void on_datasets_button_newset_clicked(GtkButton*, gpointer) {
	edit_dataset(NULL, GTK_WINDOW(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}
void on_datasets_button_editset_clicked(GtkButton*, gpointer) {
	edit_dataset(selected_dataset, GTK_WINDOW(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}
void on_datasets_button_delset_clicked(GtkButton*, gpointer) {
	if(selected_dataset && selected_dataset->isLocal()) {
		selected_dataset->destroy();
		selected_dataobject = NULL;
		function_removed(selected_dataset);
		selected_dataset = NULL;
		update_datasets_tree();
		on_tDatasets_selection_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(tDatasets)), NULL);
	}
}
void on_datasets_button_newobject_clicked(GtkButton*, gpointer) {
	edit_dataobject(selected_dataset, NULL, GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}
void on_datasets_button_editobject_clicked(GtkButton*, gpointer) {
	edit_dataobject(selected_dataset, selected_dataobject, GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}
void on_datasets_button_delobject_clicked(GtkButton*, gpointer) {
	if(selected_dataset && selected_dataobject) {
		selected_dataset->delObject(selected_dataobject);
		selected_dataobject = NULL;
		update_dataobjects();
	}
}
void on_datasets_button_close_clicked(GtkButton*, gpointer) {
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));
}

void on_dataobject_changed() {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "dataobject_edit_button_ok")), TRUE);
}

GtkWidget* get_datasets_dialog(void) {

	if(!datasets_builder) {

		datasets_builder = getBuilder("datasets.ui");
		g_assert(datasets_builder != NULL);

		g_assert(gtk_builder_get_object(datasets_builder, "datasets_dialog") != NULL);

		tDatasets = GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_treeview_datasets"));
		tDataObjects = GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_treeview_objects"));

		tDataObjects_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tDataObjects), GTK_TREE_MODEL(tDataObjects_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDataObjects));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Key 1", renderer, "text", 0, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataObjects), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("Key 2", renderer, "text", 1, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 1);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataObjects), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("Key 3", renderer, "text", 2, NULL);
		gtk_tree_view_column_set_sort_column_id(column, 2);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDataObjects), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tDataObjects_selection_changed), NULL);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tDataObjects_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tDataObjects_store), 0, GTK_SORT_ASCENDING);

		tDatasets_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tDatasets), GTK_TREE_MODEL(tDatasets_store));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tDatasets));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Data Set"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tDatasets), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tDatasets_selection_changed), NULL);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(tDatasets_store), 0, string_sort_func, GINT_TO_POINTER(0), NULL);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tDatasets_store), 0, GTK_SORT_ASCENDING);

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(datasets_builder, "datasets_textview_description")));
		gtk_text_buffer_create_tag(buffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
		gtk_text_buffer_create_tag(buffer, "italic", "style", PANGO_STYLE_ITALIC, NULL);

		if(datasets_width > 0 && datasets_height > 0) gtk_window_resize(GTK_WINDOW(gtk_builder_get_object(datasets_builder, "datasets_dialog")), datasets_width, datasets_height);
		if(datasets_hposition > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_hpaned")), datasets_hposition);
		if(datasets_vposition1 > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_vpaned1")), datasets_vposition1);
		if(datasets_vposition2 > 0) gtk_paned_set_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_vpaned2")), datasets_vposition2);

#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "vbox1")), 6);
		gtk_widget_set_margin_end(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "vbox2")), 6);
#else
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "vbox1")), 6);
		gtk_widget_set_margin_right(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "vbox2")), 6);
#endif

		gtk_builder_add_callback_symbols(datasets_builder, "on_datasets_button_newset_clicked", G_CALLBACK(on_datasets_button_newset_clicked), "on_datasets_button_editset_clicked", G_CALLBACK(on_datasets_button_editset_clicked), "on_datasets_button_delset_clicked", G_CALLBACK(on_datasets_button_delset_clicked), "on_datasets_button_newobject_clicked", G_CALLBACK(on_datasets_button_newobject_clicked), "on_datasets_button_editobject_clicked", G_CALLBACK(on_datasets_button_editobject_clicked), "on_datasets_button_delobject_clicked", G_CALLBACK(on_datasets_button_delobject_clicked), NULL);
		gtk_builder_connect_signals(datasets_builder, NULL);

		update_datasets_tree();

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(datasets_builder, "datasets_dialog"));
}

GtkWidget* get_dataobject_edit_dialog(void) {
	update_window_properties(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "dataobject_edit_dialog")));
	return GTK_WIDGET(gtk_builder_get_object(datasets_builder, "dataobject_edit_dialog"));
}

void edit_dataobject(DataSet *ds, DataObject *o, GtkWidget *win) {
	if(!ds) return;
	GtkWidget *dialog = get_dataobject_edit_dialog();
	if(o) {
		gtk_window_set_title(GTK_WINDOW(dialog), _("Edit Data Object"));
	} else {
		gtk_window_set_title(GTK_WINDOW(dialog), _("New Data Object"));
	}
	if(win) gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	GtkWidget *ptable = GTK_WIDGET(gtk_builder_get_object(datasets_builder, "dataobject_edit_grid"));
	GList *childlist = gtk_container_get_children(GTK_CONTAINER(ptable));
	for(guint i = 0; ; i++) {
		GtkWidget *w = (GtkWidget*) g_list_nth_data(childlist, i);
		if(!w) break;
		gtk_widget_destroy(w);
	}
	g_list_free(childlist);
	DataPropertyIter it;
	DataProperty *dp = ds->getFirstProperty(&it);
	string sval;
	int rows = 1;
	gtk_grid_remove_column(GTK_GRID(ptable), 0);
	gtk_grid_remove_column(GTK_GRID(ptable), 1);
	gtk_grid_remove_column(GTK_GRID(ptable), 2);
	gtk_grid_remove_column(GTK_GRID(ptable), 3);
	gtk_grid_set_column_spacing(GTK_GRID(ptable), 20);
	GtkWidget *label, *entry, *om;
	vector<GtkWidget*> value_entries;
	vector<GtkWidget*> approx_menus;
	string str;
	while(dp) {
		label = gtk_label_new(dp->title().c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1, 1);

		entry = gtk_entry_new();
		value_entries.push_back(entry);
		int iapprox = -1;
		if(o) {
			gtk_entry_set_text(GTK_ENTRY(entry), localize_expression(o->getProperty(dp, &iapprox)).c_str());
		}
		g_signal_connect(entry, "changed", G_CALLBACK(on_dataobject_changed), NULL);
		gtk_grid_attach(GTK_GRID(ptable), entry, 1, rows - 1, 1, 1);

		label = gtk_label_new(localize_expression(dp->getUnitString(), true).c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START);
		gtk_grid_attach(GTK_GRID(ptable), label, 2, rows - 1, 1, 1);

		if(dp->propertyType() == PROPERTY_STRING) {
			approx_menus.push_back(NULL);
		} else {
			om = gtk_combo_box_text_new();
			approx_menus.push_back(om);
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(om), NULL, _("Default"));
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(om), NULL, _("Approximate"));
			gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(om), NULL, _("Exact"));
			gtk_combo_box_set_active(GTK_COMBO_BOX(om), iapprox + 1);
			g_signal_connect(om, "changed", G_CALLBACK(on_dataobject_changed), NULL);
			gtk_grid_attach(GTK_GRID(ptable), om, 3, rows - 1, 1, 1);
		}

		rows++;
		dp = ds->getNextProperty(&it);
	}
	if(value_entries.size() > 0) gtk_widget_grab_focus(value_entries[0]);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(datasets_builder, "dataobject_edit_button_ok")), FALSE);
	gtk_widget_show_all(ptable);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		bool new_object = (o == NULL);
		if(new_object) {
			o = new DataObject(ds);
			ds->addObject(o);
		}
		dp = ds->getFirstProperty(&it);
		size_t i = 0;
		string val;
		while(dp) {
			val = unlocalize_expression(gtk_entry_get_text(GTK_ENTRY(value_entries[i])));
			remove_blank_ends(val);
			if(!val.empty()) {
				o->setProperty(dp, val, approx_menus[i] ? gtk_combo_box_get_active(GTK_COMBO_BOX(approx_menus[i])) - 1 : -1);
			} else if(!new_object) {
				o->eraseProperty(dp);
			}
			dp = ds->getNextProperty(&it);
			i++;
		}
		o->setUserModified();
		selected_dataobject = o;
		GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(tDataObjects));
		gdouble pos = 0;
		if(adj) pos = gtk_adjustment_get_value(adj);
		update_dataobjects();
		while(gtk_events_pending()) gtk_main_iteration();
		if(adj) gtk_adjustment_set_value(adj, pos);
	}
	gtk_widget_hide(dialog);
}

void update_datasets_settings() {
	if(datasets_builder) {
		gint w = 0, h = 0;
		gtk_window_get_size(GTK_WINDOW(gtk_builder_get_object(datasets_builder, "datasets_dialog")), &w, &h);
		datasets_width = w;
		datasets_height = h;
		datasets_hposition = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_hpaned")));
		datasets_vposition1 = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_vpaned1")));
		datasets_vposition2 = gtk_paned_get_position(GTK_PANED(gtk_builder_get_object(datasets_builder, "datasets_vpaned2")));
	}
}

void manage_datasets(GtkWindow *parent) {
	GtkWidget *dialog = get_datasets_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_widget_show(dialog);
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}
