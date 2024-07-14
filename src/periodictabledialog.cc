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
#include "periodictabledialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *periodictable_builder = NULL;

vector<GtkWidget*> ewindows;
vector<DataObject*> eobjects;

void on_element_button_function_clicked(GtkButton *w, gpointer user_data) {
	DataProperty *dp = (DataProperty*) user_data;
	DataSet *ds = NULL;
	DataObject *o = NULL;
	GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(w));
	for(size_t i = 0; i < ewindows.size(); i++) {
		if(ewindows[i] == win) {
			o = eobjects[i];
			break;
		}
	}
	if(dp) ds = dp->parentSet();
	if(ds && o) {
		string str = ds->preferredInputName(printops.abbreviate_names, printops.use_unicode_signs, false, false, &can_display_unicode_string_function, (void*) expressiontext).formattedName(TYPE_FUNCTION, true);
		str += "(";
		str += o->getProperty(ds->getPrimaryKeyProperty());
		str += CALCULATOR->getComma();
		str += " ";
		str += dp->getName();
		str += ")";
		insert_text(str.c_str());
	}
}
void on_element_button_close_clicked(GtkButton *w, gpointer user_data) {
	GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(w));
	for(size_t i = 0; i < ewindows.size(); i++) {
		if(ewindows[i] == win) {
			ewindows.erase(ewindows.begin() + i);
			eobjects.erase(eobjects.begin() + i);
			break;
		}
	}
	gtk_widget_destroy((GtkWidget*) user_data);
}
void on_element_button_clicked(GtkButton*, gpointer user_data) {
	DataObject *e = (DataObject*) user_data;
	if(e) {
		DataSet *ds = e->parentSet();
		if(!ds) return;
		GtkWidget *dialog = gtk_dialog_new();
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
		ewindows.push_back(dialog);
		eobjects.push_back(e);
		GtkWidget *close_button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("_Close"), GTK_RESPONSE_CLOSE);
		g_signal_connect(G_OBJECT(close_button), "clicked", G_CALLBACK(on_element_button_close_clicked), (gpointer) dialog);
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gtk_builder_get_object(periodictable_builder, "periodic_dialog")));
		gtk_window_set_title(GTK_WINDOW(dialog), _("Element Data"));
		gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
		GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
		gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox);

		GtkWidget *vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_box_pack_start(GTK_BOX(vbox), vbox2, FALSE, TRUE, 0);

		DataProperty *p_number = ds->getProperty("number");
		DataProperty *p_symbol = ds->getProperty("symbol");
		DataProperty *p_class = ds->getProperty("class");
		DataProperty *p_name = ds->getProperty("name");

		GtkWidget *label;
		label = gtk_label_new(NULL);
		string str = "<span size=\"large\">"; str += e->getProperty(p_number); str += "</span>";
		gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_END); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, TRUE, 0);
		label = gtk_label_new(NULL);
		str = "<span size=\"xx-large\">"; str += e->getProperty(p_symbol); str += "</span>";
		gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, TRUE, 0);
		label = gtk_label_new(NULL);
		str = "<span size=\"x-large\">"; str += e->getProperty(p_name); str += "</span>  ";
		gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox2), label, FALSE, TRUE, 0);

		GtkWidget *button;
		GtkWidget *ptable = gtk_grid_new();
		gtk_grid_set_column_spacing(GTK_GRID(ptable), 6);
		gtk_box_pack_start(GTK_BOX(vbox), ptable, FALSE, TRUE, 0);
		int rows = 0;

		int group = s2i(e->getProperty(p_class));
		if(group > 0) {
			rows++;
			label = gtk_label_new(NULL);
			str = "<span weight=\"bold\">"; str += _("Classification"); str += ":"; str += "</span>";
			gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), FALSE);
			gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1, 1);
			label = gtk_label_new(NULL);
			switch(group) {
				case ALKALI_METALS: {gtk_label_set_markup(GTK_LABEL(label), _("Alkali Metal")); break;}
				case ALKALI_EARTH_METALS: {gtk_label_set_markup(GTK_LABEL(label), _("Alkaline-Earth Metal")); break;}
				case LANTHANIDES: {gtk_label_set_markup(GTK_LABEL(label), _("Lanthanide")); break;}
				case ACTINIDES: {gtk_label_set_markup(GTK_LABEL(label), _("Actinide")); break;}
				case TRANSITION_METALS: {gtk_label_set_markup(GTK_LABEL(label), _("Transition Metal")); break;}
				case METALS: {gtk_label_set_markup(GTK_LABEL(label), _("Metal")); break;}
				case METALLOIDS: {gtk_label_set_markup(GTK_LABEL(label), _("Metalloid")); break;}
				case NONMETALS: {gtk_label_set_markup(GTK_LABEL(label), _("Polyatomic Non-Metal")); break;}
				case HALOGENS: {gtk_label_set_markup(GTK_LABEL(label), _("Diatomic Non-Metal")); break;}
				case NOBLE_GASES: {gtk_label_set_markup(GTK_LABEL(label), _("Noble Gas")); break;}
				case TRANSACTINIDES: {gtk_label_set_markup(GTK_LABEL(label), _("Unknown chemical properties")); break;}
				default: {gtk_label_set_markup(GTK_LABEL(label), _("Unknown")); break;}
			}
			gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
#if GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 12
			gtk_widget_set_margin_end(label, 10);
#else
			gtk_widget_set_margin_right(label, 10);
#endif
			gtk_grid_attach(GTK_GRID(ptable), label, 1, rows - 1, 1, 1);
		}

		DataPropertyIter it;
		DataProperty *dp = ds->getFirstProperty(&it);
		string sval;
		while(dp) {
			if(!dp->isHidden() && dp != p_number && dp != p_class && dp != p_symbol && dp != p_name) {
				sval = e->getPropertyDisplayString(dp);
				if(!sval.empty()) {
					rows++;
					label = gtk_label_new(NULL);
					str = "<span weight=\"bold\">"; str += dp->title(); str += ":"; str += "</span>";
					gtk_label_set_markup(GTK_LABEL(label), str.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), FALSE);
					gtk_grid_attach(GTK_GRID(ptable), label, 0, rows - 1, 1, 1);
					label = gtk_label_new(NULL);
					gtk_label_set_markup(GTK_LABEL(label), sval.c_str()); gtk_widget_set_halign(label, GTK_ALIGN_START); gtk_label_set_selectable(GTK_LABEL(label), TRUE);
					gtk_grid_attach(GTK_GRID(ptable), label, 1, rows - 1, 1, 1);
					button = gtk_button_new();
					gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("edit-paste", GTK_ICON_SIZE_BUTTON));
					gtk_grid_attach(GTK_GRID(ptable), button, 2, rows - 1, 1, 1);
					g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_element_button_function_clicked), (gpointer) dp);
				}
			}
			dp = ds->getNextProperty(&it);
		}

		gtk_widget_show_all(dialog);

	}
}

GtkWidget* get_periodic_dialog(void) {
	if(!periodictable_builder) {

		periodictable_builder = getBuilder("periodictable.ui");
		g_assert(periodictable_builder != NULL);

		g_assert(gtk_builder_get_object(periodictable_builder, "periodic_dialog") != NULL);

		gtk_builder_connect_signals(periodictable_builder, NULL);

		DataSet *dc = CALCULATOR->getDataSet("atom");
		if(!dc) {
			update_window_properties(GTK_WIDGET(gtk_builder_get_object(periodictable_builder, "periodic_dialog")), false);
			return GTK_WIDGET(gtk_builder_get_object(periodictable_builder, "periodic_dialog"));
		}

		DataObject *e;
		GtkWidget *e_button;
		GtkGrid *e_table = GTK_GRID(gtk_builder_get_object(periodictable_builder, "periodic_table"));
		string tip;
		GtkCssProvider *e_style[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
		GtkCssProvider *l_style = NULL;
		DataProperty *p_xpos = dc->getProperty("x_pos");
		DataProperty *p_ypos = dc->getProperty("y_pos");
		DataProperty *p_weight = dc->getProperty("weight");
		DataProperty *p_number = dc->getProperty("number");
		DataProperty *p_symbol = dc->getProperty("symbol");
		DataProperty *p_class = dc->getProperty("class");
		DataProperty *p_name = dc->getProperty("name");
		int x_pos = 0, y_pos = 0, group = 0;
		string weight;
		l_style = gtk_css_provider_new();
		for(size_t i3 = 0; i3 < 12; i3++) {
			e_style[i3] = gtk_css_provider_new();
		}
		for(size_t i = 1; i < 120; i++) {
			e = dc->getObject(i2s(i));
			if(e) {
				x_pos = s2i(e->getProperty(p_xpos));
				y_pos = s2i(e->getProperty(p_ypos));
			}
			if(e && x_pos > 0 && x_pos <= 18 && y_pos > 0 && y_pos <= 10) {
				e_button = gtk_button_new();
				gtk_button_set_relief(GTK_BUTTON(e_button), GTK_RELIEF_HALF);
				gtk_container_add(GTK_CONTAINER(e_button), gtk_label_new(e->getProperty(p_symbol).c_str()));
				group = s2i(e->getProperty(p_class));
				if(group > 0 && group <= 11) gtk_style_context_add_provider(gtk_widget_get_style_context(e_button), GTK_STYLE_PROVIDER(e_style[group - 1]), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				else gtk_style_context_add_provider(gtk_widget_get_style_context(e_button), GTK_STYLE_PROVIDER(e_style[11]), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
				if(x_pos > 2) gtk_grid_attach(e_table, e_button, x_pos + 1, y_pos, 1, 1);
				else gtk_grid_attach(e_table, e_button, x_pos, y_pos, 1, 1);
				tip = e->getProperty(p_number);
				tip += " ";
				tip += e->getProperty(p_name);
				weight = e->getPropertyDisplayString(p_weight);
				if(!weight.empty() && weight != "-") {
					tip += "\n";
					tip += weight;
				}
				gtk_widget_set_tooltip_text(e_button, tip.c_str());
				gtk_widget_show_all(e_button);
				g_signal_connect((gpointer) e_button, "clicked", G_CALLBACK(on_element_button_clicked), (gpointer) e);
			}
		}
		gtk_css_provider_load_from_data(l_style, "* {color: #000000;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[0], "* {color: #000000; background-image: none; background-color: #eeccee;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[1], "* {color: #000000; background-image: none; background-color: #ddccee;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[2], "* {color: #000000; background-image: none; background-color: #ccddff;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[3], "* {color: #000000; background-image: none; background-color: #ddeeff;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[4], "* {color: #000000; background-image: none; background-color: #cceeee;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[5], "* {color: #000000; background-image: none; background-color: #bbffbb;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[6], "* {color: #000000; background-image: none; background-color: #eeffdd;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[7], "* {color: #000000; background-image: none; background-color: #ffffaa;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[8], "* {color: #000000; background-image: none; background-color: #ffddaa;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[9], "* {color: #000000; background-image: none; background-color: #ffccdd;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[10], "* {color: #000000; background-image: none; background-color: #aaeedd;}", -1, NULL);
		gtk_css_provider_load_from_data(e_style[11], "* {color: #000000; background-image: none;}", -1, NULL);
	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(periodictable_builder, "periodic_dialog")), true);

	return GTK_WIDGET(gtk_builder_get_object(periodictable_builder, "periodic_dialog"));
}

void show_periodic_table(GtkWindow *parent) {
	GtkWidget *dialog = get_periodic_dialog();
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_widget_show(dialog);
	gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
}
