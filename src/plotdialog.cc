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
#include "openhelp.h"
#include "plotdialog.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

GtkBuilder *plot_builder = NULL;
GtkWidget *tPlotFunctions;
GtkListStore *tPlotFunctions_store;

PlotLegendPlacement default_plot_legend_placement = PLOT_LEGEND_TOP_RIGHT;
bool default_plot_display_grid = true;
bool default_plot_full_border = false;
string default_plot_min = "0";
string default_plot_max = "10";
string default_plot_step = "1";
int default_plot_sampling_rate = 1001;
int default_plot_linewidth = 2;
int default_plot_complex = -1;
bool default_plot_use_sampling_rate = true;
bool default_plot_rows = false;
int default_plot_type = 0;
PlotStyle default_plot_style = PLOT_STYLE_LINES;
PlotSmoothing default_plot_smoothing = PLOT_SMOOTHING_NONE;
string default_plot_variable = "x";
bool default_plot_color = true;
int max_plot_time = 5;

enum {
	SMOOTHING_MENU_NONE,
	SMOOTHING_MENU_UNIQUE,
	SMOOTHING_MENU_CSPLINES,
	SMOOTHING_MENU_BEZIER,
	SMOOTHING_MENU_SBEZIER
};

enum {
	PLOTSTYLE_MENU_LINES,
	PLOTSTYLE_MENU_POINTS,
	PLOTSTYLE_MENU_LINESPOINTS,
	PLOTSTYLE_MENU_BOXES,
	PLOTSTYLE_MENU_HISTEPS,
	PLOTSTYLE_MENU_STEPS,
	PLOTSTYLE_MENU_CANDLESTICKS,
	PLOTSTYLE_MENU_DOTS,
	PLOTSTYLE_MENU_POLAR
};

enum {
	PLOTLEGEND_MENU_NONE,
	PLOTLEGEND_MENU_TOP_LEFT,
	PLOTLEGEND_MENU_TOP_RIGHT,
	PLOTLEGEND_MENU_BOTTOM_LEFT,
	PLOTLEGEND_MENU_BOTTOM_RIGHT,
	PLOTLEGEND_MENU_BELOW,
	PLOTLEGEND_MENU_OUTSIDE
};

bool read_plot_settings_line(string &svar, string &svalue, int &v) {
	if(svar == "plot_legend_placement") {
		if(v >= PLOT_LEGEND_NONE && v <= PLOT_LEGEND_OUTSIDE) default_plot_legend_placement = (PlotLegendPlacement) v;
	} else if(svar == "plot_style") {
		if(v >= PLOT_STYLE_LINES && v <= PLOT_STYLE_POLAR) default_plot_style = (PlotStyle) v;
	} else if(svar == "plot_smoothing") {
		if(v >= PLOT_SMOOTHING_NONE && v <= PLOT_SMOOTHING_SBEZIER) default_plot_smoothing = (PlotSmoothing) v;
	} else if(svar == "plot_display_grid") {
		default_plot_display_grid = v;
	} else if(svar == "plot_full_border") {
		default_plot_full_border = v;
	} else if(svar == "plot_min") {
		default_plot_min = svalue;
	} else if(svar == "plot_max") {
		default_plot_max = svalue;
	} else if(svar == "plot_step") {
		default_plot_step = svalue;
	} else if(svar == "plot_sampling_rate") {
		default_plot_sampling_rate = v;
	} else if(svar == "plot_use_sampling_rate") {
		default_plot_use_sampling_rate = v;
	} else if(svar == "plot_complex") {
		default_plot_complex = v;
	} else if(svar == "plot_variable") {
		default_plot_variable = svalue;
	} else if(svar == "plot_rows") {
		default_plot_rows = v;
	} else if(svar == "plot_type") {
		default_plot_type = v;
	} else if(svar == "plot_color") {
		if(version_numbers[0] > 2 || (version_numbers[0] == 2 && (version_numbers[1] > 2 || (version_numbers[1] == 2 && version_numbers[2] > 1)))) {
			default_plot_color = v;
		}
	} else if(svar == "plot_linewidth") {
		default_plot_linewidth = v;
	} else if(svar == "max_plot_time") {
		max_plot_time = v;
	} else {
		return false;
	}
	return true;
}
void write_plot_settings(FILE *file) {
	fprintf(file, "plot_legend_placement=%i\n", default_plot_legend_placement);
	fprintf(file, "plot_style=%i\n", default_plot_style);
	fprintf(file, "plot_smoothing=%i\n", default_plot_smoothing);
	fprintf(file, "plot_display_grid=%i\n", default_plot_display_grid);
	fprintf(file, "plot_full_border=%i\n", default_plot_full_border);
	fprintf(file, "plot_min=%s\n", default_plot_min.c_str());
	fprintf(file, "plot_max=%s\n", default_plot_max.c_str());
	fprintf(file, "plot_step=%s\n", default_plot_step.c_str());
	fprintf(file, "plot_sampling_rate=%i\n", default_plot_sampling_rate);
	fprintf(file, "plot_use_sampling_rate=%i\n", default_plot_use_sampling_rate);
	if(default_plot_complex >= 0) fprintf(file, "plot_complex=%i\n", default_plot_complex);
	fprintf(file, "plot_variable=%s\n", default_plot_variable.c_str());
	fprintf(file, "plot_rows=%i\n", default_plot_rows);
	fprintf(file, "plot_type=%i\n", default_plot_type);
	fprintf(file, "plot_color=%i\n", default_plot_color);
	fprintf(file, "plot_linewidth=%i\n", default_plot_linewidth);
	if(max_plot_time != 5) fprintf(file, "max_plot_time=%i\n", max_plot_time);
}

void on_tPlotFunctions_selection_changed(GtkTreeSelection *treeselection, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(treeselection, &model, &iter)) {
		gchar *gstr1, *gstr2, *gstr3;
		gint type, smoothing, style, axis, rows;
		gtk_tree_model_get(model, &iter, 0, &gstr1, 1, &gstr2, 2, &style, 3, &smoothing, 4, &type, 5, &axis, 6, &rows, 9, &gstr3, -1);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")), gstr2);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")), gstr3);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_title")), gstr1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), style);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), smoothing);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")), type == 1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")), type == 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_yaxis1")), axis != 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_yaxis2")), axis == 2);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), rows);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_remove")), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_modify")), TRUE);
		g_free(gstr1);
		g_free(gstr2);
		g_free(gstr3);
	} else {
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")), "");
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")), "");
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_modify")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_remove")), FALSE);
	}
}
void on_plot_dialog_hide(GtkWidget*, gpointer) {
	default_plot_display_grid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_grid")));
	default_plot_full_border = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_full_border")));
	default_plot_rows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")));
	default_plot_color = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_color")));
	default_plot_min = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_min")));
	default_plot_max = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_max")));
	default_plot_step = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_step")));
	default_plot_variable = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
	default_plot_use_sampling_rate = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_steps")));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")))) {
		default_plot_type = 1;
	} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")))) {
		default_plot_type = 2;
	} else {
		default_plot_type = 0;
	}
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")))) {
		case PLOTLEGEND_MENU_NONE: {default_plot_legend_placement = PLOT_LEGEND_NONE; break;}
		case PLOTLEGEND_MENU_TOP_LEFT: {default_plot_legend_placement = PLOT_LEGEND_TOP_LEFT; break;}
		case PLOTLEGEND_MENU_TOP_RIGHT: {default_plot_legend_placement = PLOT_LEGEND_TOP_RIGHT; break;}
		case PLOTLEGEND_MENU_BOTTOM_LEFT: {default_plot_legend_placement = PLOT_LEGEND_BOTTOM_LEFT; break;}
		case PLOTLEGEND_MENU_BOTTOM_RIGHT: {default_plot_legend_placement = PLOT_LEGEND_BOTTOM_RIGHT; break;}
		case PLOTLEGEND_MENU_BELOW: {default_plot_legend_placement = PLOT_LEGEND_BELOW; break;}
		case PLOTLEGEND_MENU_OUTSIDE: {default_plot_legend_placement = PLOT_LEGEND_OUTSIDE; break;}
	}
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")))) {
		case SMOOTHING_MENU_NONE: {default_plot_smoothing = PLOT_SMOOTHING_NONE; break;}
		case SMOOTHING_MENU_UNIQUE: {default_plot_smoothing = PLOT_SMOOTHING_UNIQUE; break;}
		case SMOOTHING_MENU_CSPLINES: {default_plot_smoothing = PLOT_SMOOTHING_CSPLINES; break;}
		case SMOOTHING_MENU_BEZIER: {default_plot_smoothing = PLOT_SMOOTHING_BEZIER; break;}
		case SMOOTHING_MENU_SBEZIER: {default_plot_smoothing = PLOT_SMOOTHING_SBEZIER; break;}
	}
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")))) {
		case PLOTSTYLE_MENU_LINES: {default_plot_style = PLOT_STYLE_LINES; break;}
		case PLOTSTYLE_MENU_POINTS: {default_plot_style = PLOT_STYLE_POINTS; break;}
		case PLOTSTYLE_MENU_LINESPOINTS: {default_plot_style = PLOT_STYLE_POINTS_LINES; break;}
		case PLOTSTYLE_MENU_BOXES: {default_plot_style = PLOT_STYLE_BOXES; break;}
		case PLOTSTYLE_MENU_HISTEPS: {default_plot_style = PLOT_STYLE_HISTOGRAM; break;}
		case PLOTSTYLE_MENU_STEPS: {default_plot_style = PLOT_STYLE_STEPS; break;}
		case PLOTSTYLE_MENU_CANDLESTICKS: {default_plot_style = PLOT_STYLE_CANDLESTICKS; break;}
		case PLOTSTYLE_MENU_DOTS: {default_plot_style = PLOT_STYLE_DOTS; break;}
		case PLOTSTYLE_MENU_POLAR: {default_plot_style = PLOT_STYLE_POLAR; break;}
	}
	default_plot_sampling_rate = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps")));
	default_plot_linewidth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_linewidth")));
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	while(b) {
		MathStructure *y_vector, *x_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 7, &x_vector, 8, &y_vector, -1);
		if(y_vector) delete y_vector;
		if(x_vector) delete x_vector;
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_save")), false);
	CALCULATOR->closeGnuplot();
}
bool generate_plot(PlotParameters &pp, vector<MathStructure> &y_vectors, vector<MathStructure> &x_vectors, vector<PlotDataParameters*> &pdps) {
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	if(!b) {
		return false;
	}
	while(b) {
		int count = 1;
		gchar *gstr1, *gstr2;
		gint type = 0, style = 0, smoothing = 0, axis = 1, rows = 0;
		MathStructure *y_vector, *x_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 0, &gstr1, 1, &gstr2, 2, &style, 3, &smoothing, 4, &type, 5, &axis, 6, &rows, 7, &x_vector, 8, &y_vector, -1);
		if(type == 1) {
			if(y_vector->isMatrix()) {
				count = 0;
				if(rows) {
					for(size_t i = 1; i <= y_vector->rows(); i++) {
						y_vectors.push_back(m_undefined);
						y_vector->rowToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						count++;
					}
				} else {
					for(size_t i = 1; i <= y_vector->columns(); i++) {
						y_vectors.push_back(m_undefined);
						y_vector->columnToVector(i, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						count++;
					}
				}
			} else if(y_vector->isVector()) {
				y_vectors.push_back(*y_vector);
				x_vectors.push_back(m_undefined);
			} else {
				y_vectors.push_back(*y_vector);
				y_vectors[y_vectors.size() - 1].transform(STRUCT_VECTOR);
				x_vectors.push_back(m_undefined);
			}
		} else if(type == 2) {
			if(y_vector->isMatrix()) {
				count = 0;
				if(rows) {
					for(size_t i = 1; i <= y_vector->rows(); i += 2) {
						y_vectors.push_back(m_undefined);
						y_vector->rowToVector(i + 1, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						y_vector->rowToVector(i, x_vectors[x_vectors.size() - 1]);
						count++;
					}
				} else {
					for(size_t i = 1; i <= y_vector->columns(); i += 2) {
						y_vectors.push_back(m_undefined);
						y_vector->columnToVector(i + 1, y_vectors[y_vectors.size() - 1]);
						x_vectors.push_back(m_undefined);
						y_vector->columnToVector(i, x_vectors[x_vectors.size() - 1]);
						count++;
					}
				}
			} else if(y_vector->isVector()) {
				y_vectors.push_back(*y_vector);
				x_vectors.push_back(m_undefined);
			} else {
				y_vectors.push_back(*y_vector);
				y_vectors[y_vectors.size() - 1].transform(STRUCT_VECTOR);
				x_vectors.push_back(m_undefined);
			}
		} else {
			y_vectors.push_back(*y_vector);
			x_vectors.push_back(*x_vector);
		}
		for(int i = 0; i < count; i++) {
			PlotDataParameters *pdp = new PlotDataParameters();
			pdp->title = gstr1;
			remove_blank_ends(pdp->title);
			if(pdp->title.empty()) {
				pdp->title = gstr2;
			}
			if(count > 1) {
				pdp->title += " :";
				pdp->title += i2s(i + 1);
			}
			pdp->test_continuous = type != 1 && type != 2;
			switch(smoothing) {
				case SMOOTHING_MENU_NONE: {pdp->smoothing = PLOT_SMOOTHING_NONE; break;}
				case SMOOTHING_MENU_UNIQUE: {pdp->smoothing = PLOT_SMOOTHING_UNIQUE; break;}
				case SMOOTHING_MENU_CSPLINES: {pdp->smoothing = PLOT_SMOOTHING_CSPLINES; break;}
				case SMOOTHING_MENU_BEZIER: {pdp->smoothing = PLOT_SMOOTHING_BEZIER; break;}
				case SMOOTHING_MENU_SBEZIER: {pdp->smoothing = PLOT_SMOOTHING_SBEZIER; break;}
			}
			switch(style) {
				case PLOTSTYLE_MENU_LINES: {pdp->style = PLOT_STYLE_LINES; break;}
				case PLOTSTYLE_MENU_POINTS: {pdp->style = PLOT_STYLE_POINTS; break;}
				case PLOTSTYLE_MENU_LINESPOINTS: {pdp->style = PLOT_STYLE_POINTS_LINES; break;}
				case PLOTSTYLE_MENU_DOTS: {pdp->style = PLOT_STYLE_DOTS; break;}
				case PLOTSTYLE_MENU_BOXES: {pdp->style = PLOT_STYLE_BOXES; break;}
				case PLOTSTYLE_MENU_HISTEPS: {pdp->style = PLOT_STYLE_HISTOGRAM; break;}
				case PLOTSTYLE_MENU_STEPS: {pdp->style = PLOT_STYLE_STEPS; break;}
				case PLOTSTYLE_MENU_CANDLESTICKS: {pdp->style = PLOT_STYLE_CANDLESTICKS; break;}
				case PLOTSTYLE_MENU_POLAR: {pdp->style = PLOT_STYLE_POLAR; break;}
			}
			pdp->yaxis2 = (axis == 2);
			pdps.push_back(pdp);
		}
		g_free(gstr1);
		g_free(gstr2);
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	}
	switch(gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")))) {
		case PLOTLEGEND_MENU_NONE: {pp.legend_placement = PLOT_LEGEND_NONE; break;}
		case PLOTLEGEND_MENU_TOP_LEFT: {pp.legend_placement = PLOT_LEGEND_TOP_LEFT; break;}
		case PLOTLEGEND_MENU_TOP_RIGHT: {pp.legend_placement = PLOT_LEGEND_TOP_RIGHT; break;}
		case PLOTLEGEND_MENU_BOTTOM_LEFT: {pp.legend_placement = PLOT_LEGEND_BOTTOM_LEFT; break;}
		case PLOTLEGEND_MENU_BOTTOM_RIGHT: {pp.legend_placement = PLOT_LEGEND_BOTTOM_RIGHT; break;}
		case PLOTLEGEND_MENU_BELOW: {pp.legend_placement = PLOT_LEGEND_BELOW; break;}
		case PLOTLEGEND_MENU_OUTSIDE: {pp.legend_placement = PLOT_LEGEND_OUTSIDE; break;}
	}
	pp.title = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_plottitle")));
	pp.x_label = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_xlabel")));
	pp.y_label = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_ylabel")));
	pp.grid = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_grid")));
	pp.x_log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_xlog")));
	pp.y_log = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_ylog")));
	pp.x_log_base = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_xlog_base")));
	pp.y_log_base = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_ylog_base")));
	pp.auto_y_min = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_ymin")));
	pp.auto_y_max = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_ymax")));
	pp.y_min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_ymin")));
	pp.y_max = gtk_spin_button_get_value(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_ymax")));
	pp.color = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_color")));
	pp.show_all_borders = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_full_border")));
	pp.linewidth = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_linewidth")));
	return true;
}
void on_plot_button_help_clicked(GtkButton, gpointer) {
	show_help("qalculate-plotting.html", GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
}
void on_plot_button_save_clicked(GtkButton*, gpointer) {
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	GtkFileChooserNative *d = gtk_file_chooser_native_new(_("Select file to export"), GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Save"), _("_Cancel"));
#else
	GtkWidget *d = gtk_file_chooser_dialog_new(_("Select file to export"), GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
	if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
#endif
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(d), TRUE);
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Allowed File Types"));
	gtk_file_filter_add_mime_type(filter, "image/x-xfig");
	gtk_file_filter_add_mime_type(filter, "image/svg");
	gtk_file_filter_add_mime_type(filter, "text/x-tex");
	gtk_file_filter_add_mime_type(filter, "application/pdf");
	gtk_file_filter_add_mime_type(filter, "application/postscript");
	gtk_file_filter_add_mime_type(filter, "image/x-eps");
	gtk_file_filter_add_mime_type(filter, "image/png");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), filter);
	GtkFileFilter *filter_all = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter_all, "*");
	gtk_file_filter_set_name(filter_all, _("All Files"));
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), filter_all);
	string title = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_plottitle")));
	if(title.empty()) {
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(d), "plot.png");
	} else {
		title += ".png";
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(d), title.c_str());
	}
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#else
	if(gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
#endif
		vector<MathStructure> y_vectors;
		vector<MathStructure> x_vectors;
		vector<PlotDataParameters*> pdps;
		PlotParameters pp;
		if(generate_plot(pp, y_vectors, x_vectors, pdps)) {
			pp.filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
			pp.filetype = PLOT_FILETYPE_AUTO;
			block_error();
			CALCULATOR->plotVectors(&pp, y_vectors, x_vectors, pdps, false, max_plot_time * 1000);
			display_errors(GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
			unblock_error();
			for(size_t i = 0; i < pdps.size(); i++) {
				if(pdps[i]) delete pdps[i];
			}
		}
	}
#if !defined(_WIN32) && (GTK_MAJOR_VERSION > 3 || GTK_MINOR_VERSION >= 20)
	g_object_unref(d);
#else
	gtk_widget_destroy(d);
#endif
}
void update_plot() {
	vector<MathStructure> y_vectors;
	vector<MathStructure> x_vectors;
	vector<PlotDataParameters*> pdps;
	PlotParameters pp;
	if(!generate_plot(pp, y_vectors, x_vectors, pdps)) {
		CALCULATOR->closeGnuplot();
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_save")), false);
		return;
	}
	block_error();
	CALCULATOR->plotVectors(&pp, y_vectors, x_vectors, pdps, false, max_plot_time * 1000);
	display_errors(GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
	unblock_error();
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_save")), true);
	for(size_t i = 0; i < pdps.size(); i++) {
		if(pdps[i]) delete pdps[i];
	}
}

void generate_plot_series(MathStructure **x_vector, MathStructure **y_vector, int type, string str, string str_x) {
	CALCULATOR->beginTemporaryStopIntervalArithmetic();
	EvaluationOptions eo;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	eo.parse_options = evalops.parse_options;
	if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
	eo.parse_options.base = 10;
	eo.parse_options.read_precision = DONT_READ_PRECISION;
	block_error();
	if(type == 1 || type == 2) {
		*y_vector = new MathStructure();
		if(!CALCULATOR->calculate(*y_vector, CALCULATOR->unlocalizeExpression(str, eo.parse_options), max_plot_time * 1000, eo)) {
			GtkWidget *d = gtk_message_dialog_new (GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("It took too long to generate the plot data."));
			if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
		}
		*x_vector = NULL;
	} else {
		*x_vector = new MathStructure();
		(*x_vector)->clearVector();
		MathStructure min;
		if(!CALCULATOR->calculate(&min, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_min"))), eo.parse_options), 1000, eo)) {
			GtkWidget *d = gtk_message_dialog_new (GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("It took too long to generate the plot data."));
			if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
			display_errors(GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
			unblock_error();
			CALCULATOR->endTemporaryStopIntervalArithmetic();
			return;
		}
		MathStructure max;
		if(!CALCULATOR->calculate(&max, CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_max"))), eo.parse_options), 1000, eo)) {
			GtkWidget *d = gtk_message_dialog_new (GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")), (GtkDialogFlags) 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("It took too long to generate the plot data."));
			if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(d), always_on_top);
			gtk_dialog_run(GTK_DIALOG(d));
			gtk_widget_destroy(d);
			display_errors(GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
			unblock_error();
			CALCULATOR->endTemporaryStopIntervalArithmetic();
			return;
		}
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_step")))) {
			*y_vector = new MathStructure(CALCULATOR->expressionToPlotVector(CALCULATOR->unlocalizeExpression(str, eo.parse_options), min, max, CALCULATOR->calculate(CALCULATOR->unlocalizeExpression(gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_step"))), eo.parse_options), eo), default_plot_complex >= 0 ? default_plot_complex : evalops.allow_complex, *x_vector, str_x, eo.parse_options, max_plot_time * 1000));
		} else {
			*y_vector = new MathStructure(CALCULATOR->expressionToPlotVector(CALCULATOR->unlocalizeExpression(str, eo.parse_options), min, max, gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps"))), default_plot_complex >= 0 ? default_plot_complex : evalops.allow_complex, *x_vector, str_x, eo.parse_options, max_plot_time * 1000));
		}
	}
	CALCULATOR->endTemporaryStopIntervalArithmetic();
	display_errors(GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
	unblock_error();
}
void on_plot_button_add_clicked(GtkButton*, gpointer) {
	string expression = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
	if(expression.find_first_not_of(SPACES) == string::npos) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
		show_message(_("Empty expression."), GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
		return;
	}
	gint type = 0, axis = 1, rows = 0;
	string title = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_title")));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")))) {
		type = 1;
	} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")))) {
		type = 2;
	}
	string str_x = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
	remove_blank_ends(str_x);
	if(str_x.empty() && type == 0) {
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
		show_message(_("Empty x variable."), GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
		return;
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_yaxis2")))) {
		axis = 2;
	}
	if((type == 1 || type == 2) && title.empty()) {
		Variable *v = CALCULATOR->getActiveVariable(expression);
		if(v) {
			title = v->title(false);
		}
	}
	rows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")));
	MathStructure *x_vector, *y_vector;
	generate_plot_series(&x_vector, &y_vector, type, expression, str_x);
	GtkTreeIter iter;
	if(type != 1 && type != 2 && y_vector->isMatrix()) {
		EvaluationOptions eo;
		eo.approximation = APPROXIMATION_APPROXIMATE;
		eo.parse_options = evalops.parse_options;
		if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
		eo.parse_options.base = 10;
		eo.parse_options.read_precision = DONT_READ_PRECISION;
		eo.interval_calculation = INTERVAL_CALCULATION_NONE;
		MathStructure mfunc = CALCULATOR->parse(expression, eo.parse_options);
		if(!mfunc.isVector()) {
			CALCULATOR->beginTemporaryStopIntervalArithmetic();
			CALCULATOR->beginTemporaryStopMessages();
			mfunc.eval(eo);
			CALCULATOR->endTemporaryStopMessages();
			CALCULATOR->endTemporaryStopIntervalArithmetic();
		}
		string str;
		PrintOptions po = printops;
		po.can_display_unicode_string_arg = (void*) gtk_builder_get_object(plot_builder, "plot_entry_expression");
		po.base = 10;
		po.is_approximate = NULL;
		po.allow_non_usable = false;
		for(size_t i = 0; i < y_vector->columns(); i++) {
			MathStructure *m = new MathStructure();
			y_vector->columnToVector(i + 1, *m);
			gtk_list_store_append(tPlotFunctions_store, &iter);
			if(mfunc.isVector() && i < mfunc.size()) str = mfunc[i].print(po);
			else str = expression;
			gtk_list_store_set(tPlotFunctions_store, &iter, 0, title.c_str(), 1, str.c_str(), 2, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style"))), 3, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing"))), 4, type, 5, axis, 6, rows, 7, new MathStructure(*x_vector), 8, m, 9, str_x.c_str(), -1);
			if(i == 0) {
				gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")), str.c_str());
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions)), &iter);
			}
		}
		delete y_vector;
		delete x_vector;
	} else {
		gtk_list_store_append(tPlotFunctions_store, &iter);
		gtk_list_store_set(tPlotFunctions_store, &iter, 0, title.c_str(), 1, expression.c_str(), 2, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style"))), 3, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing"))), 4, type, 5, axis, 6, rows, 7, x_vector, 8, y_vector, 9, str_x.c_str(), -1);
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions)), &iter);
	}
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
	update_plot();
}
void on_plot_button_modify_clicked(GtkButton*, gpointer) {

	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		string expression = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
		if(expression.find_first_not_of(SPACES) == string::npos) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
			show_message(_("Empty expression."), GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
			return;
		}
		gint type = 0, axis = 1, rows = 0;
		string title = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_title")));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")))) {
			type = 1;
		} else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")))) {
			type = 2;
		}
		string str_x = gtk_entry_get_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
		remove_blank_ends(str_x);
		if(str_x.empty() && type == 0) {
			gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_variable")));
			show_message(_("Empty x variable."), GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog")));
			return;
		}
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_yaxis2")))) {
			axis = 2;
		}
		if((type == 1 || type == 2) && title.empty()) {
			Variable *v = CALCULATOR->getActiveVariable(expression);
			if(v) {
				title = v->title(false);
			}
		}
		MathStructure *x_vector, *y_vector;
		gchar *old_expression;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 1, &old_expression, 7, &x_vector, 8, &y_vector, -1);
		if(x_vector) delete x_vector;
		if(y_vector) delete y_vector;
		x_vector = NULL;
		y_vector = NULL;
		generate_plot_series(&x_vector, &y_vector, type, expression, str_x);
		rows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")));
		if(type != 1 && type != 2 && y_vector->isMatrix() && expression != old_expression) {
			EvaluationOptions eo;
			eo.approximation = APPROXIMATION_APPROXIMATE;
			eo.parse_options = evalops.parse_options;
			if(!simplified_percentage) eo.parse_options.parsing_mode = (ParsingMode) (eo.parse_options.parsing_mode | PARSE_PERCENT_AS_ORDINARY_CONSTANT);
			eo.parse_options.base = 10;
			eo.parse_options.read_precision = DONT_READ_PRECISION;
			eo.interval_calculation = INTERVAL_CALCULATION_NONE;
			MathStructure mfunc = CALCULATOR->parse(expression, eo.parse_options);
			if(!mfunc.isVector()) {
				CALCULATOR->beginTemporaryStopIntervalArithmetic();
				CALCULATOR->beginTemporaryStopMessages();
				mfunc.eval(eo);
				CALCULATOR->endTemporaryStopMessages();
				CALCULATOR->endTemporaryStopIntervalArithmetic();
			}
			string str;
			PrintOptions po = printops;
			po.can_display_unicode_string_arg = (void*) gtk_builder_get_object(plot_builder, "plot_entry_expression");
			po.base = 10;
			po.is_approximate = NULL;
			po.allow_non_usable = false;
			for(size_t i = 0; i < y_vector->columns(); i++) {
				MathStructure *m = new MathStructure();
				y_vector->columnToVector(i + 1, *m);
				if(i != 0) gtk_list_store_append(tPlotFunctions_store, &iter);
				if(mfunc.isVector() && i < mfunc.size()) str = mfunc[i].print(po);
				else str = expression;
				if(i == 0) gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")), str.c_str());
				gtk_list_store_set(tPlotFunctions_store, &iter, 0, title.c_str(), 1, str.c_str(), 2, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style"))), 3, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing"))), 4, type, 5, axis, 6, rows, 7, new MathStructure(*x_vector), 8, m, 9, str_x.c_str(), -1);
			}
			delete x_vector;
			delete y_vector;
		} else {
			gtk_list_store_set(tPlotFunctions_store, &iter, 0, title.c_str(), 1, expression.c_str(), 2, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style"))), 3, gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing"))), 4, type, 5, axis, 6, rows, 7, x_vector, 8, y_vector, 9, str_x.c_str(), -1);
		}
		g_free(old_expression);
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
		update_plot();
	}
}
void on_plot_button_remove_clicked(GtkButton*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		MathStructure *x_vector, *y_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 7, &x_vector, 8, &y_vector, -1);
		if(x_vector) delete x_vector;
		if(y_vector) delete y_vector;
		gtk_list_store_remove(tPlotFunctions_store, &iter);
		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
		update_plot();
	}
}

void on_plot_checkbutton_xlog_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_xlog_base")), gtk_toggle_button_get_active(w));
}
void on_plot_checkbutton_ylog_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_ylog_base")), gtk_toggle_button_get_active(w));
}
void on_plot_checkbutton_ymin_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_ymin")), gtk_toggle_button_get_active(w));
}
void on_plot_checkbutton_ymax_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_ymax")), gtk_toggle_button_get_active(w));
}
void on_plot_radiobutton_step_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_step")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps")), !gtk_toggle_button_get_active(w));
}
void on_plot_radiobutton_steps_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_step")), !gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps")), gtk_toggle_button_get_active(w));
}
void on_plot_entry_expression_activate(GtkEntry*, gpointer) {
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions));
	if(gtk_tree_selection_get_selected(select, &model, &iter)) {
		on_plot_button_modify_clicked(GTK_BUTTON(gtk_builder_get_object(plot_builder, "plot_button_modify")), NULL);
	} else {
		on_plot_button_add_clicked(GTK_BUTTON(gtk_builder_get_object(plot_builder, "plot_button_add")), NULL);
	}
}
gboolean on_plot_entry_expression_key_press_event(GtkWidget *o, GdkEventKey *event, gpointer) {
	if(entry_in_quotes(GTK_ENTRY(o))) return FALSE;
	const gchar *key = key_press_get_symbol(event, false);
	if(!key) return FALSE;
	if(strlen(key) > 0) entry_insert_text(o, key);
	return TRUE;
}

void on_plot_radiobutton_function_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_box_variable")), gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), !gtk_toggle_button_get_active(w));
}
void on_plot_radiobutton_vector_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_box_variable")), !gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), gtk_toggle_button_get_active(w));
}
void on_plot_radiobutton_paired_toggled(GtkToggleButton *w, gpointer) {
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_box_variable")), !gtk_toggle_button_get_active(w));
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), gtk_toggle_button_get_active(w));
}
void on_plot_button_range_apply_clicked(GtkButton*, gpointer) {
	GtkTreeIter iter;
	bool b = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	while(b) {
		gchar *gstr2, *gstr3;
		gint type = 0;
		MathStructure *y_vector, *x_vector;
		gtk_tree_model_get(GTK_TREE_MODEL(tPlotFunctions_store), &iter, 1, &gstr2, 4, &type, 7, &x_vector, 8, &y_vector, 9, &gstr3, -1);
		if(y_vector) delete y_vector;
		if(x_vector) delete x_vector;
		x_vector = NULL;
		y_vector = NULL;
		generate_plot_series(&x_vector, &y_vector, type, gstr2, gstr3);
		g_free(gstr2);
		g_free(gstr3);
		gtk_list_store_set(tPlotFunctions_store, &iter, 7, x_vector, 8, y_vector, -1);
		b = gtk_tree_model_iter_next(GTK_TREE_MODEL(tPlotFunctions_store), &iter);
	}
	update_plot();
}
void on_plot_button_appearance_apply_clicked(GtkButton*, gpointer) {
	update_plot();
}

GtkWidget* get_plot_dialog(void) {
	if(!plot_builder) {

		if(!CALCULATOR->canPlot()) return NULL;

		plot_builder = getBuilder("plot.ui");
		g_assert(plot_builder != NULL);

		g_assert(gtk_builder_get_object(plot_builder, "plot_dialog") != NULL);

		tPlotFunctions = GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_treeview_data"));
		tPlotFunctions_store = gtk_list_store_new(10, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_STRING);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tPlotFunctions), GTK_TREE_MODEL(tPlotFunctions_store));
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tPlotFunctions));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
		GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(_("Title"), renderer, "text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tPlotFunctions), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Expression"), renderer, "text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tPlotFunctions), column);
		g_signal_connect((gpointer) selection, "changed", G_CALLBACK(on_tPlotFunctions_selection_changed), NULL);

		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_save")), false);

		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), 0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), 0);

		gtk_builder_add_callback_symbols(plot_builder, "on_plot_dialog_hide", G_CALLBACK(on_plot_dialog_hide), "on_plot_button_help_clicked", G_CALLBACK(on_plot_button_help_clicked), "on_plot_button_save_clicked", G_CALLBACK(on_plot_button_save_clicked), "on_plot_entry_expression_activate", G_CALLBACK(on_plot_entry_expression_activate), "on_plot_entry_expression_key_press_event", G_CALLBACK(on_plot_entry_expression_key_press_event), "on_plot_radiobutton_function_toggled", G_CALLBACK(on_plot_radiobutton_function_toggled), "on_plot_radiobutton_vector_toggled", G_CALLBACK(on_plot_radiobutton_vector_toggled), "on_plot_radiobutton_paired_toggled", G_CALLBACK(on_plot_radiobutton_paired_toggled), "on_plot_button_add_clicked", G_CALLBACK(on_plot_button_add_clicked), "on_plot_button_modify_clicked", G_CALLBACK(on_plot_button_modify_clicked), "on_plot_button_remove_clicked", G_CALLBACK(on_plot_button_remove_clicked), "on_plot_radiobutton_steps_toggled", G_CALLBACK(on_plot_radiobutton_steps_toggled), "on_plot_radiobutton_step_toggled", G_CALLBACK(on_plot_radiobutton_step_toggled), "on_plot_button_range_apply_clicked", G_CALLBACK(on_plot_button_range_apply_clicked), "on_plot_checkbutton_ymin_toggled", G_CALLBACK(on_plot_checkbutton_ymin_toggled), "on_plot_checkbutton_ymax_toggled", G_CALLBACK(on_plot_checkbutton_ymax_toggled), "on_plot_checkbutton_xlog_toggled", G_CALLBACK(on_plot_checkbutton_xlog_toggled), "on_plot_checkbutton_ylog_toggled", G_CALLBACK(on_plot_checkbutton_ylog_toggled), "on_plot_button_appearance_apply_clicked", G_CALLBACK(on_plot_button_appearance_apply_clicked), NULL);
		gtk_builder_connect_signals(plot_builder, NULL);

	}

	update_window_properties(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));

	return GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog"));
}

bool is_plot_dialog(GtkWindow *w) {
	return plot_builder && w == GTK_WINDOW(gtk_builder_get_object(plot_builder, "plot_dialog"));
}
void hide_plot_dialog() {
	if(plot_builder && gtk_widget_get_visible(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")))) {
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_dialog")));
	}
}
void show_plot_dialog(GtkWindow *parent, const gchar *text) {
	GtkWidget *dialog = get_plot_dialog();
	if(!dialog) {
		GtkWidget *edialog = gtk_message_dialog_new(parent, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("Gnuplot was not found."));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
		gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(edialog), _("%s (%s) needs to be installed separately, and found in the executable search path, for plotting to work."), "Gnuplot", "<a href=\"http://www.gnuplot.info/\">http://www.gnuplot.info/</a>");
		GList *childlist = gtk_container_get_children(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(edialog))));
		for(guint i = 0; ; i++) {
			GtkWidget *w = (GtkWidget*) g_list_nth_data(childlist, i);
			if(!w) break;
			g_signal_connect(G_OBJECT(w), "activate-link", G_CALLBACK(on_activate_link), NULL);
		}
		g_list_free(childlist);
		gtk_dialog_run(GTK_DIALOG(edialog));
		gtk_widget_destroy(edialog);
		return;
	}
	gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_expression")), text);
	if(!gtk_widget_get_visible(dialog)) {
		gtk_list_store_clear(tPlotFunctions_store);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_modify")), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_button_remove")), FALSE);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_grid")), default_plot_display_grid);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_full_border")), default_plot_full_border);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), default_plot_rows);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_color")), default_plot_color);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_mono")), !default_plot_color);
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_min")), default_plot_min.c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_max")), default_plot_max.c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_step")), default_plot_step.c_str());
		gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(plot_builder, "plot_entry_variable")), default_plot_variable.c_str());
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_steps")), default_plot_use_sampling_rate);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_step")), !default_plot_use_sampling_rate);
		switch(default_plot_type) {
			case 1: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_vector")), TRUE); break;}
			case 2: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_paired")), TRUE); break;}
			default: {gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(plot_builder, "plot_radiobutton_function")), TRUE); break;}
		}
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_checkbutton_rows")), default_plot_type == 1 || default_plot_type == 2);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_box_variable")), default_plot_type != 1 && default_plot_type != 2);
		switch(default_plot_legend_placement) {
			case PLOT_LEGEND_NONE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_NONE); break;}
			case PLOT_LEGEND_TOP_LEFT: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_TOP_LEFT); break;}
			case PLOT_LEGEND_TOP_RIGHT: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_TOP_RIGHT); break;}
			case PLOT_LEGEND_BOTTOM_LEFT: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_BOTTOM_LEFT); break;}
			case PLOT_LEGEND_BOTTOM_RIGHT: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_BOTTOM_RIGHT); break;}
			case PLOT_LEGEND_BELOW: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_BELOW); break;}
			case PLOT_LEGEND_OUTSIDE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_legend_place")), PLOTLEGEND_MENU_OUTSIDE); break;}
		}
		switch(default_plot_smoothing) {
			case PLOT_SMOOTHING_NONE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_NONE); break;}
			case PLOT_SMOOTHING_UNIQUE: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_UNIQUE); break;}
			case PLOT_SMOOTHING_CSPLINES: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_CSPLINES); break;}
			case PLOT_SMOOTHING_BEZIER: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_BEZIER); break;}
			case PLOT_SMOOTHING_SBEZIER: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_smoothing")), SMOOTHING_MENU_SBEZIER); break;}
		}
		switch(default_plot_style) {
			case PLOT_STYLE_LINES: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_LINES); break;}
			case PLOT_STYLE_POINTS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_LINES); break;}
			case PLOT_STYLE_POINTS_LINES: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_LINESPOINTS); break;}
			case PLOT_STYLE_BOXES: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_BOXES); break;}
			case PLOT_STYLE_HISTOGRAM: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_HISTEPS); break;}
			case PLOT_STYLE_STEPS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_STEPS); break;}
			case PLOT_STYLE_CANDLESTICKS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_CANDLESTICKS); break;}
			case PLOT_STYLE_DOTS: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_DOTS); break;}
			case PLOT_STYLE_POLAR: {gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(plot_builder, "plot_combobox_style")), PLOTSTYLE_MENU_POLAR); break;}
		}
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_steps")), default_plot_sampling_rate);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(plot_builder, "plot_spinbutton_linewidth")), default_plot_linewidth);

		gtk_widget_show(dialog);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(plot_builder, "plot_notebook")), 2);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(plot_builder, "plot_notebook")), 1);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gtk_builder_get_object(plot_builder, "plot_notebook")), 0);

		gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(plot_builder, "plot_entry_expression")));
	} else {
		gtk_window_present_with_time(GTK_WINDOW(dialog), GDK_CURRENT_TIME);
	}
}
