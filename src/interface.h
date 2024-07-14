/*
    Qalculate (GTK UI)

    Copyright (C) 2003-2007, 2008, 2016  Hanna Knutsson (hanna.knutsson@protonmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef INTERFACE_H
#define INTERFACE_H

#define MENU_ITEM_WITH_INT(x,y,z)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), GINT_TO_POINTER(z)); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_WITH_STRING(x,y,z)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) z); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_WITH_POINTER(x,y,z)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) z); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_WITH_OBJECT(x,y)		item = gtk_menu_item_new_with_label(x->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str()); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) x); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_WITH_POINTER_PREPEND(x,y,z)	item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) z); gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_WITH_OBJECT_PREPEND(x,y)	item = gtk_menu_item_new_with_label(x->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str()); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) x); gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_WITH_OBJECT_AND_FLAG(x,y)	{GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6); unordered_map<string, cairo_surface_t*>::const_iterator it_flag = flag_surfaces.find(x->referenceName()); GtkWidget *image_w; if(it_flag != flag_surfaces.end()) {image_w = gtk_image_new_from_surface(it_flag->second);} else {image_w = gtk_image_new();} gtk_widget_set_size_request(image_w, flagheight * 2, flagheight); gtk_container_add(GTK_CONTAINER(box), image_w); gtk_container_add(GTK_CONTAINER(box), gtk_label_new(x->title(true, printops.use_unicode_signs, &can_display_unicode_string_function, (void*) sub).c_str())); item = gtk_menu_item_new(); gtk_container_add(GTK_CONTAINER(item), box); gtk_widget_show_all(item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) x); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);}
#define MENU_ITEM_MARKUP_WITH_INT_AND_FLAGIMAGE(x,y,z,i)	{GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6); GtkWidget *image_w = gtk_image_new_from_surface(y); gtk_widget_set_size_request(image_w, flagheight * 2, flagheight); gtk_container_add(GTK_CONTAINER(box), image_w); GtkWidget *label = gtk_label_new(x); gtk_container_add(GTK_CONTAINER(box), label); item = gtk_menu_item_new(); gtk_container_add(GTK_CONTAINER(item), box); gtk_widget_show_all(item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(z), GINT_TO_POINTER(i)); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);}
#define MENU_ITEM_MARKUP_WITH_POINTER(x,y,z)	item = gtk_menu_item_new_with_label(""); gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), x); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), (gpointer) z); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_MARKUP_WITH_INT(x,y,z)	item = gtk_menu_item_new_with_label(""); gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), x); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), GINT_TO_POINTER(z)); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM(x,y)				item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_MARKUP(x,y)			item = gtk_menu_item_new_with_label(""); gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), x); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_NO_ITEMS(x)			item = gtk_menu_item_new_with_label(x); gtk_widget_set_sensitive(item, FALSE); gtk_widget_show (item); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define CHECK_MENU_ITEM(x,y,b)			item = gtk_check_menu_item_new_with_label(x); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), b); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define RADIO_MENU_ITEM(x,y,b)			item = gtk_radio_menu_item_new_with_label(group, x); group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item)); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), b); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define RADIO_MENU_ITEM_WITH_INT(x,y,b,z)	item = gtk_radio_menu_item_new_with_label(group, x); group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item)); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), b); gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), GINT_TO_POINTER(z)); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define POPUP_CHECK_MENU_ITEM_WITH_LABEL(y,w,l)	item = gtk_check_menu_item_new_with_label(l); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))); gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(w)); if(gstr) {gtk_widget_set_tooltip_text(item, gstr); g_free(gstr);} gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define POPUP_CHECK_MENU_ITEM(y,w)		item = gtk_check_menu_item_new_with_label(gtk_menu_item_get_label(GTK_MENU_ITEM(w))); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))); gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(w)); if(gstr) {gtk_widget_set_tooltip_text(item, gstr); g_free(gstr);} gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define POPUP_RADIO_MENU_ITEM(y,w)		item = gtk_radio_menu_item_new_with_label(group, gtk_menu_item_get_label(GTK_MENU_ITEM(w))); group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item)); gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))); gstr = gtk_widget_get_tooltip_text(GTK_WIDGET(w)); if(gstr) {gtk_widget_set_tooltip_text(item, gstr); g_free(gstr);} gtk_widget_show (item); g_signal_connect(G_OBJECT (item), "activate", G_CALLBACK(y), NULL); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_ITEM_SET_ACCEL(a)			gtk_widget_add_accelerator(item, "activate", accel_group, a, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
#define MENU_TEAROFF				item = gtk_tearoff_menu_item_new(); gtk_widget_show (item); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_SEPARATOR				item = gtk_separator_menu_item_new(); gtk_widget_show (item); gtk_menu_shell_append(GTK_MENU_SHELL(sub), item);
#define MENU_SEPARATOR_PREPEND			item = gtk_separator_menu_item_new(); gtk_widget_show (item); gtk_menu_shell_prepend(GTK_MENU_SHELL(sub), item);
#define SUBMENU_ITEM(x,y)			item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); gtk_menu_shell_append(GTK_MENU_SHELL(y), item); sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
#define SUBMENU_ITEM_PREPEND(x,y)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); gtk_menu_shell_prepend(GTK_MENU_SHELL(y), item); sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
#define SUBMENU_ITEM_INSERT(x,y,i)		item = gtk_menu_item_new_with_label(x); gtk_widget_show (item); gtk_menu_shell_insert(GTK_MENU_SHELL(y), item, i); sub = gtk_menu_new(); gtk_widget_show (sub); gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);

#define EXPRESSION_YPAD 3

#ifdef _WIN32
void create_systray_icon();
void destroy_systray_icon();
#endif
bool has_systray_icon();
void set_tooltips_enabled(GtkWidget *w, bool b);
void test_border(void);
void update_colors(bool initial = false);
void set_custom_buttons(void);
void create_button_menus(void);
void update_button_padding(bool initial = false);
void create_main_window(void);
GtkWidget* get_preferences_dialog(void);
GtkWidget* get_matrix_dialog(void);

#endif /* INTERFACE_H */
