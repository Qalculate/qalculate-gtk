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
#include "preferencesdialog.h"
#include "mainwindow.h"
#include "exchangerates.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

int auto_update_exchange_rates = -1;

bool read_exchange_rates_settings_line(string &svar, string&, int &v) {
	if(svar == "auto_update_exchange_rates") {
		auto_update_exchange_rates = v;
	} else {
		return false;
	}
	return true;
}
void write_exchange_rates_settings(FILE *file) {
	fprintf(file, "auto_update_exchange_rates=%i\n", auto_update_exchange_rates);
}

int exchange_rates_frequency() {return auto_update_exchange_rates;}
void set_exchange_rates_frequency(int v) {
	auto_update_exchange_rates = v;
	preferences_update_exchange_rates();
}

class FetchExchangeRatesThread : public Thread {
protected:
	virtual void run();
};

void fetch_exchange_rates(int timeout, int n) {
	bool b_busy_bak = calculator_busy();
	block_error();
	set_busy();
	FetchExchangeRatesThread fetch_thread;
	if(fetch_thread.start() && fetch_thread.write(timeout) && fetch_thread.write(n)) {
		int i = 0;
		while(fetch_thread.running && i < 50) {
			while(gtk_events_pending()) gtk_main_iteration();
			sleep_ms(10);
			i++;
		}
		if(fetch_thread.running) {
			GtkWidget *dialog = gtk_message_dialog_new(main_window(), (GtkDialogFlags) (GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL), GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, _("Fetching exchange rates."));
			if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(dialog), always_on_top);
			gtk_widget_show(dialog);
			while(fetch_thread.running) {
				while(gtk_events_pending()) gtk_main_iteration();
				sleep_ms(10);
			}
			gtk_widget_destroy(dialog);
		}
	}
	if(b_busy_bak) set_busy();
	unblock_error();
}

void FetchExchangeRatesThread::run() {
	int timeout = 15;
	int n = -1;
	if(!read(&timeout)) return;
	if(!read(&n)) return;
	CALCULATOR->fetchExchangeRates(timeout, n);
}

bool check_exchange_rates(GtkWindow *win, bool set_result) {
	int i = CALCULATOR->exchangeRatesUsed();
	if(i == 0) return false;
	if(auto_update_exchange_rates == 0 && win != NULL) return false;
	if(CALCULATOR->checkExchangeRatesDate(auto_update_exchange_rates > 0 ? auto_update_exchange_rates : 7, false, auto_update_exchange_rates == 0, i)) return false;
	if(auto_update_exchange_rates == 0) return false;
	bool b = false;
	if(auto_update_exchange_rates < 0) {
		int days = (int) floor(difftime(time(NULL), CALCULATOR->getExchangeRatesTime(i)) / 86400);
		GtkWidget *edialog = gtk_message_dialog_new(win == NULL ? main_window() : win, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Do you wish to update the exchange rates now?"));
		if(always_on_top) gtk_window_set_keep_above(GTK_WINDOW(edialog), always_on_top);
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(edialog), _n("It has been %s day since the exchange rates last were updated.", "It has been %s days since the exchange rates last were updated.", days), i2s(days).c_str());
		GtkWidget *w = gtk_check_button_new_with_label(_("Do not ask again"));
		gtk_container_add(GTK_CONTAINER(gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(edialog))), w);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FALSE);
		gtk_widget_show(w);
		switch(gtk_dialog_run(GTK_DIALOG(edialog))) {
			case GTK_RESPONSE_YES: {
				b = true;
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
					auto_update_exchange_rates = 7;
				}
				break;
			}
			case GTK_RESPONSE_NO: {
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))) {
					auto_update_exchange_rates = 0;
				}
				break;
			}
			default: {}
		}
		gtk_widget_destroy(edialog);
	}
	if(b || auto_update_exchange_rates > 0) {
		if(auto_update_exchange_rates <= 0) i = -1;
		if(!b && set_result) setResult(NULL, false, false, false, "", 0, false);
		fetch_exchange_rates(b ? 15 : 8, i);
		CALCULATOR->loadExchangeRates();
		return true;
	}
	return false;
}
