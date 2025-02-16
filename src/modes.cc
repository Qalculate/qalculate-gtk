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
#include "modes.h"

using std::string;
using std::cout;
using std::vector;
using std::endl;

vector<mode_struct> modes;
string current_mode;

extern int visible_keypad;

bool load_mode(string name) {
	for(size_t i = 0; i < modes.size(); i++) {
		if(modes[i].name == name) {
			if(modes[i].name == _("Preset") || modes[i].name == _("Default")) current_mode = "";
			else current_mode = modes[i].name;
			load_mode(&modes[i]);
			return true;
		}
	}
	return false;
}

void save_initial_modes() {
	save_mode_as(_("Preset"));
	save_mode_as(_("Default"));
}

void save_default_mode(const char *custom_angle_unit) {
	modes[1].precision = CALCULATOR->getPrecision();
	modes[1].interval = CALCULATOR->usesIntervalArithmetic();
	modes[1].concise_uncertainty_input = CALCULATOR->conciseUncertaintyInputEnabled();
	modes[1].fixed_denominator = CALCULATOR->fixedDenominator();
	modes[1].adaptive_interval_display = adaptive_interval_display;
	modes[1].variable_units_enabled = CALCULATOR->variableUnitsEnabled();
	modes[1].po = printops;
	modes[1].po.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	modes[1].eo = evalops;
	modes[1].at = CALCULATOR->defaultAssumptions()->type();
	modes[1].as = CALCULATOR->defaultAssumptions()->sign();
	modes[1].rpn_mode = rpn_mode;
	modes[1].autocalc = auto_calculate;
	modes[1].chain_mode = chain_mode;
	modes[1].keypad = visible_keypad;
	modes[1].custom_output_base = CALCULATOR->customOutputBase();
	modes[1].custom_input_base = CALCULATOR->customInputBase();
	modes[1].complex_angle_form = complex_angle_form;
	modes[1].implicit_question_asked = implicit_question_asked;
	modes[1].simplified_percentage = simplified_percentage;
	if(custom_angle_unit) modes[1].custom_angle_unit = custom_angle_unit;
	else if(CALCULATOR->customAngleUnit()) modes[1].custom_angle_unit = CALCULATOR->customAngleUnit()->referenceName();
	else modes[1].custom_angle_unit = "";
}

size_t initialize_mode_as(string name) {
	remove_blank_ends(name);
	modes.push_back(modes[1]);
	size_t index = modes.size() - 1;
	modes[index].name = name;
	modes[index].implicit_question_asked = false;
	modes[index].description = "";
	return index;
}

size_t save_mode_as(string name, bool *new_mode, bool set_as_current) {
	remove_blank_ends(name);
	size_t index = 0;
	for(; index < modes.size(); index++) {
		if(modes[index].name == name) {
			if(new_mode) *new_mode = false;
			break;
		}
	}
	if(index >= modes.size()) {
		modes.resize(modes.size() + 1);
		index = modes.size() - 1;
		if(new_mode) *new_mode = true;
	}
	modes[index].po = printops;
	modes[index].po.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
	modes[index].eo = evalops;
	modes[index].precision = CALCULATOR->getPrecision();
	modes[index].interval = CALCULATOR->usesIntervalArithmetic();
	modes[index].adaptive_interval_display = adaptive_interval_display;
	modes[index].variable_units_enabled = CALCULATOR->variableUnitsEnabled();
	modes[index].at = CALCULATOR->defaultAssumptions()->type();
	modes[index].as = CALCULATOR->defaultAssumptions()->sign();
	modes[index].concise_uncertainty_input = CALCULATOR->conciseUncertaintyInputEnabled();
	modes[index].fixed_denominator = CALCULATOR->fixedDenominator();
	modes[index].name = name;
	modes[index].rpn_mode = rpn_mode;
	modes[index].autocalc = auto_calculate;
	modes[index].chain_mode = chain_mode;
	modes[index].keypad = visible_keypad;
	modes[index].custom_output_base = CALCULATOR->customOutputBase();
	modes[index].custom_input_base = CALCULATOR->customInputBase();
	modes[index].complex_angle_form = complex_angle_form;
	modes[index].implicit_question_asked = implicit_question_asked;
	modes[index].simplified_percentage = simplified_percentage;
	modes[index].custom_angle_unit = "";
	if(index == 0) modes[index].description = _("Default mode loaded at first startup for new users");
	else if(index == 1) modes[index].description = _("Mode loaded at each startup and, by default, saved at exit");
	else modes[index].description = "";
	if(CALCULATOR->customAngleUnit()) modes[index].custom_angle_unit = CALCULATOR->customAngleUnit()->referenceName();
	if(set_as_current) current_mode = modes[index].name;
	return index;
}
size_t remove_mode(string name) {
	for(size_t index = 2; index < modes.size(); index++) {
		if(modes[index].name == name) {
			modes.erase(modes.begin() + index);
			return index;
		}
	}
	return (size_t) -1;
}
size_t mode_count(bool include_default) {
	if(include_default) return modes.size();
	else return modes.size() - 2;
}
size_t mode_index(string name, bool case_sensitive) {
	for(size_t index = 0; index < modes.size(); index++) {
		if(modes[index].name == name || (!case_sensitive && equalsIgnoreCase(modes[index].name, name))) {
			return index;
		}
	}
	return (size_t) -1;
}
mode_struct *get_mode(size_t index) {
	if(index >= modes.size()) return NULL;
	return &modes[index];
}
string current_mode_name() {
	return current_mode;
}

extern string custom_angle_unit;
bool read_mode_line(size_t mode_index, string &svar, string &svalue, int &v) {
	if(svar == "min_deci") {
		if(mode_index == 1) printops.min_decimals = v;
		else modes[mode_index].po.min_decimals = v;
	} else if(svar == "use_min_deci") {
		if(mode_index == 1) printops.use_min_decimals = v;
		else modes[mode_index].po.use_min_decimals = v;
	} else if(svar == "max_deci") {
		if(mode_index == 1) printops.max_decimals = v;
		else modes[mode_index].po.max_decimals = v;
	} else if(svar == "use_max_deci") {
		if(mode_index == 1) printops.use_max_decimals = v;
		else modes[mode_index].po.use_max_decimals = v;
	} else if(svar == "precision") {
		if(v == 8 && (version_numbers[0] < 3 || (version_numbers[0] == 3 && version_numbers[1] <= 12))) v = 10;
		if(mode_index == 1) CALCULATOR->setPrecision(v);
		else modes[mode_index].precision = v;
	} else if(svar == "min_exp") {
		if(mode_index == 1) printops.min_exp = v;
		else modes[mode_index].po.min_exp = v;
	} else if(svar == "interval_arithmetic") {
		if(version_numbers[0] >= 3) {
			if(mode_index == 1) CALCULATOR->useIntervalArithmetic(v);
			else modes[mode_index].interval = v;
		} else {
			modes[mode_index].interval = true;
		}
	} else if(svar == "interval_display") {
		if(v == 0) {
			if(mode_index == 1) {printops.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS; adaptive_interval_display = true;}
			else {modes[mode_index].po.interval_display = INTERVAL_DISPLAY_SIGNIFICANT_DIGITS; modes[mode_index].adaptive_interval_display = true;}
		} else {
			v--;
			if(v >= INTERVAL_DISPLAY_SIGNIFICANT_DIGITS && v <= INTERVAL_DISPLAY_RELATIVE) {
				if(mode_index == 1) {printops.interval_display = (IntervalDisplay) v; adaptive_interval_display = false;}
				else {modes[mode_index].po.interval_display = (IntervalDisplay) v; modes[mode_index].adaptive_interval_display = false;}
			}
		}
	} else if(svar == "concise_uncertainty_input") {
		if(mode_index == 1) CALCULATOR->setConciseUncertaintyInputEnabled(v);
		else modes[mode_index].concise_uncertainty_input = v;
	} else if(svar == "negative_exponents") {
		if(mode_index == 1) printops.negative_exponents = v;
		else modes[mode_index].po.negative_exponents = v;
	} else if(svar == "sort_minus_last") {
		if(mode_index == 1) printops.sort_options.minus_last = v;
		else modes[mode_index].po.sort_options.minus_last = v;
	} else if(svar == "place_units_separately") {
		if(mode_index == 1) printops.place_units_separately = v;
		else modes[mode_index].po.place_units_separately = v;
	} else if(svar == "display_mode") {	//obsolete
		switch(v) {
			case 1: {
				if(mode_index == 1) {
					printops.min_exp = EXP_PRECISION;
					printops.negative_exponents = false;
					printops.sort_options.minus_last = true;
				} else {
					modes[mode_index].po.min_exp = EXP_PRECISION;
					modes[mode_index].po.negative_exponents = false;
					modes[mode_index].po.sort_options.minus_last = true;
				}
				break;
			}
			case 2: {
				if(mode_index == 1) {
					printops.min_exp = EXP_SCIENTIFIC;
					printops.negative_exponents = true;
					printops.sort_options.minus_last = false;
				} else {
					modes[mode_index].po.min_exp = EXP_SCIENTIFIC;
					modes[mode_index].po.negative_exponents = true;
					modes[mode_index].po.sort_options.minus_last = false;
				}
				break;
			}
			case 3: {
				if(mode_index == 1) {
					printops.min_exp = EXP_PURE;
					printops.negative_exponents = true;
					printops.sort_options.minus_last = false;
				} else {
					modes[mode_index].po.min_exp = EXP_PURE;
					modes[mode_index].po.negative_exponents = true;
					modes[mode_index].po.sort_options.minus_last = false;
				}
				break;
			}
			case 4: {
				if(mode_index == 1) {
					printops.min_exp = EXP_NONE;
					printops.negative_exponents = false;
					printops.sort_options.minus_last = true;
				} else {
					modes[mode_index].po.min_exp = EXP_NONE;
					modes[mode_index].po.negative_exponents = false;
					modes[mode_index].po.sort_options.minus_last = true;
				}
				break;
			}
		}
	} else if(svar == "use_prefixes") {
		if(mode_index == 1) printops.use_unit_prefixes = v;
		else modes[mode_index].po.use_unit_prefixes = v;
	} else if(svar == "use_prefixes_for_all_units") {
		if(mode_index == 1) printops.use_prefixes_for_all_units = v;
		else modes[mode_index].po.use_prefixes_for_all_units = v;
	} else if(svar == "use_prefixes_for_currencies") {
		if(mode_index == 1) printops.use_prefixes_for_currencies = v;
		else modes[mode_index].po.use_prefixes_for_currencies = v;
	} else if(svar == "fractional_mode") {	//obsolete
		switch(v) {
			case 1: {
				if(mode_index == 1) printops.number_fraction_format = FRACTION_DECIMAL;
				else modes[mode_index].po.number_fraction_format = FRACTION_DECIMAL;
				break;
			}
			case 2: {
				if(mode_index == 1) printops.number_fraction_format = FRACTION_COMBINED;
				else modes[mode_index].po.number_fraction_format = FRACTION_COMBINED;
				break;
			}
			case 3: {
				if(mode_index == 1) printops.number_fraction_format = FRACTION_FRACTIONAL;
				else modes[mode_index].po.number_fraction_format = FRACTION_FRACTIONAL;
				break;
			}
		}
		if(mode_index == 1) printops.restrict_fraction_length = (printops.number_fraction_format >= FRACTION_FRACTIONAL);
		else modes[mode_index].po.restrict_fraction_length = (modes[mode_index].po.number_fraction_format >= FRACTION_FRACTIONAL);
	} else if(svar == "number_fraction_format") {
		if(v >= FRACTION_DECIMAL && v <= FRACTION_PERMYRIAD) {
			if(mode_index == 1) printops.number_fraction_format = (NumberFractionFormat) v;
			else modes[mode_index].po.number_fraction_format = (NumberFractionFormat) v;
		}
		if(mode_index == 1) printops.restrict_fraction_length = (printops.number_fraction_format == FRACTION_FRACTIONAL || printops.number_fraction_format == FRACTION_COMBINED);
		else modes[mode_index].po.restrict_fraction_length = (modes[mode_index].po.number_fraction_format == FRACTION_FRACTIONAL || modes[mode_index].po.number_fraction_format == FRACTION_COMBINED);
	} else if(svar == "number_fraction_denominator") {
		if(mode_index == 1) CALCULATOR->setFixedDenominator(v);
		else modes[mode_index].fixed_denominator = v;
	} else if(svar == "complex_number_form") {
		if(v == COMPLEX_NUMBER_FORM_CIS + 1) {
			if(mode_index == 1) {
				evalops.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
				complex_angle_form = true;
			} else {
				modes[mode_index].eo.complex_number_form = COMPLEX_NUMBER_FORM_CIS;
				modes[mode_index].complex_angle_form = true;
			}
		} else if(v >= COMPLEX_NUMBER_FORM_RECTANGULAR && v <= COMPLEX_NUMBER_FORM_CIS) {
			if(mode_index == 1) {
				evalops.complex_number_form = (ComplexNumberForm) v;
				complex_angle_form = false;
			} else {
				modes[mode_index].eo.complex_number_form = (ComplexNumberForm) v;
				modes[mode_index].complex_angle_form = false;
			}
		}
	} else if(svar == "number_base") {
		if(mode_index == 1) printops.base = v;
		else modes[mode_index].po.base = v;
	} else if(svar == "custom_number_base") {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure m;
		CALCULATOR->calculate(&m, svalue, 500);
		CALCULATOR->endTemporaryStopMessages();
		if(mode_index == 1) CALCULATOR->setCustomOutputBase(m.number());
		else modes[mode_index].custom_output_base = m.number();
	} else if(svar == "number_base_expression") {
		if(mode_index == 1) evalops.parse_options.base = v;
		else modes[mode_index].eo.parse_options.base = v;
	} else if(svar == "custom_number_base_expression") {
		CALCULATOR->beginTemporaryStopMessages();
		MathStructure m;
		CALCULATOR->calculate(&m, svalue, 500);
		CALCULATOR->endTemporaryStopMessages();
		if(mode_index == 1) CALCULATOR->setCustomInputBase(m.number());
		else modes[mode_index].custom_input_base = m.number();
	} else if(svar == "read_precision") {
		if(v >= DONT_READ_PRECISION && v <= READ_PRECISION_WHEN_DECIMALS) {
			if(mode_index == 1) evalops.parse_options.read_precision = (ReadPrecisionMode) v;
			else modes[mode_index].eo.parse_options.read_precision = (ReadPrecisionMode) v;
		}
	} else if(svar == "assume_denominators_nonzero") {
		if(version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
			v = true;
		}
		if(mode_index == 1) evalops.assume_denominators_nonzero = v;
		else modes[mode_index].eo.assume_denominators_nonzero = v;
	} else if(svar == "warn_about_denominators_assumed_nonzero") {
		if(mode_index == 1) evalops.warn_about_denominators_assumed_nonzero = v;
		else modes[mode_index].eo.warn_about_denominators_assumed_nonzero = v;
	} else if(svar == "structuring") {
		if(v >= STRUCTURING_NONE && v <= STRUCTURING_FACTORIZE) {
			if((v == STRUCTURING_NONE) && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 12))) {
				v = STRUCTURING_SIMPLIFY;
			}
			if(mode_index == 1) {
				evalops.structuring = (StructuringMode) v;
				printops.allow_factorization = (evalops.structuring == STRUCTURING_FACTORIZE);
			} else {
				modes[mode_index].eo.structuring = (StructuringMode) v;
				modes[mode_index].po.allow_factorization = (modes[mode_index].eo.structuring == STRUCTURING_FACTORIZE);
			}
		}
	} else if(svar == "angle_unit") {
		if(version_numbers[0] == 0 && (version_numbers[1] < 7 || (version_numbers[1] == 7 && version_numbers[2] == 0))) {
			v++;
		}
		if(v >= ANGLE_UNIT_NONE && v <= ANGLE_UNIT_CUSTOM) {
			if(mode_index == 1) evalops.parse_options.angle_unit = (AngleUnit) v;
			else modes[mode_index].eo.parse_options.angle_unit = (AngleUnit) v;
		}
	} else if(svar == "custom_angle_unit") {
		if(mode_index == 1) custom_angle_unit = svalue;
		else modes[mode_index].custom_angle_unit = svalue;
	} else if(svar == "functions_enabled") {
		if(mode_index == 1) evalops.parse_options.functions_enabled = v;
		else modes[mode_index].eo.parse_options.functions_enabled = v;
	} else if(svar == "variables_enabled") {
		if(mode_index == 1) evalops.parse_options.variables_enabled = v;
		else modes[mode_index].eo.parse_options.variables_enabled = v;
	} else if(svar == "donot_calculate_variables") {
		if(mode_index == 1) evalops.calculate_variables = !v;
		else modes[mode_index].eo.calculate_variables = !v;
	} else if(svar == "calculate_variables") {
		if(mode_index == 1) evalops.calculate_variables = v;
		else modes[mode_index].eo.calculate_variables = v;
	} else if(svar == "variable_units_enabled") {
		if(mode_index == 1) CALCULATOR->setVariableUnitsEnabled(v);
		else modes[mode_index].variable_units_enabled = v;
	} else if(svar == "calculate_functions") {
		if(mode_index == 1) evalops.calculate_functions = v;
		else modes[mode_index].eo.calculate_functions = v;
	} else if(svar == "sync_units") {
		if(mode_index == 1) evalops.sync_units = v;
		else modes[mode_index].eo.sync_units = v;
	} else if(svar == "unknownvariables_enabled") {
		if(mode_index == 1) evalops.parse_options.unknowns_enabled = v;
		else modes[mode_index].eo.parse_options.unknowns_enabled = v;
	} else if(svar == "units_enabled") {
		if(mode_index == 1) evalops.parse_options.units_enabled = v;
		else modes[mode_index].eo.parse_options.units_enabled = v;
	} else if(svar == "allow_complex") {
		if(mode_index == 1) evalops.allow_complex = v;
		else modes[mode_index].eo.allow_complex = v;
	} else if(svar == "allow_infinite") {
		if(mode_index == 1) evalops.allow_infinite = v;
		else modes[mode_index].eo.allow_infinite = v;
	} else if(svar == "use_short_units") {
		if(mode_index == 1) printops.abbreviate_names = v;
		else modes[mode_index].po.abbreviate_names = v;
	} else if(svar == "abbreviate_names") {
		if(mode_index == 1) printops.abbreviate_names = v;
		else modes[mode_index].po.abbreviate_names = v;
	} else if(svar == "all_prefixes_enabled") {
		if(mode_index == 1) printops.use_all_prefixes = v;
		else modes[mode_index].po.use_all_prefixes = v;
	} else if(svar == "denominator_prefix_enabled") {
		if(mode_index == 1) printops.use_denominator_prefix = v;
		else modes[mode_index].po.use_denominator_prefix = v;
	} else if(svar == "auto_post_conversion") {
		if(v >= POST_CONVERSION_NONE && v <= POST_CONVERSION_OPTIMAL) {
			if(v == POST_CONVERSION_NONE && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] <= 12))) {
				v = POST_CONVERSION_OPTIMAL;
			}
			if(mode_index == 1) evalops.auto_post_conversion = (AutoPostConversion) v;
			else modes[mode_index].eo.auto_post_conversion = (AutoPostConversion) v;
		}
	} else if(svar == "mixed_units_conversion") {
		if(v >= MIXED_UNITS_CONVERSION_NONE || v <= MIXED_UNITS_CONVERSION_FORCE_ALL) {
			if(mode_index == 1) evalops.mixed_units_conversion = (MixedUnitsConversion) v;
			else modes[mode_index].eo.mixed_units_conversion = (MixedUnitsConversion) v;
		}
	} else if(svar == "indicate_infinite_series") {
		if(mode_index == 1) printops.indicate_infinite_series = v;
		else modes[mode_index].po.indicate_infinite_series = v;
	} else if(svar == "show_ending_zeroes") {
		if(version_numbers[0] > 2 || (version_numbers[0] == 2 && version_numbers[1] >= 9)) {
			if(mode_index == 1) printops.show_ending_zeroes = v;
			else modes[mode_index].po.show_ending_zeroes = v;
		}
	} else if(svar == "round_halfway_to_even") {//obsolete
		if(v) {
			if(mode_index == 1) printops.rounding = ROUNDING_HALF_TO_EVEN;
			else modes[mode_index].po.rounding = ROUNDING_HALF_TO_EVEN;
		}
	} else if(svar == "rounding_mode") {
		if(v >= ROUNDING_HALF_AWAY_FROM_ZERO && v <= ROUNDING_DOWN) {
			if(!VERSION_AFTER(4, 9, 0) && v == 2) v = ROUNDING_TOWARD_ZERO;
			if(mode_index == 1) printops.rounding = (RoundingMode) v;
			else modes[mode_index].po.rounding = (RoundingMode) v;
		}
	} else if(svar == "always_exact") {//obsolete
		if(mode_index == 1) {
			evalops.approximation = APPROXIMATION_EXACT;
		} else {
			modes[mode_index].eo.approximation = APPROXIMATION_EXACT;
			modes[mode_index].interval = false;
		}
	} else if(svar == "approximation") {
		if(v >= APPROXIMATION_EXACT && v <= APPROXIMATION_APPROXIMATE) {
			if(mode_index == 1) {
				evalops.approximation = (ApproximationMode) v;
			} else {
				modes[mode_index].eo.approximation = (ApproximationMode) v;
			}
		}
	} else if(svar == "interval_calculation") {
		if(v >= INTERVAL_CALCULATION_NONE && v <= INTERVAL_CALCULATION_SIMPLE_INTERVAL_ARITHMETIC) {
			if(mode_index == 1) evalops.interval_calculation = (IntervalCalculation) v;
			else modes[mode_index].eo.interval_calculation = (IntervalCalculation) v;
		}
	} else if(svar == "calculate_as_you_type") {
		if(mode_index == 1) auto_calculate = v;
		else modes[mode_index].autocalc = v;
	} else if(svar == "chain_mode") {
		if(mode_index == 1) chain_mode = v;
		else modes[mode_index].chain_mode = v;
	} else if(svar == "in_rpn_mode") {
		if(mode_index == 1) rpn_mode = v;
		else modes[mode_index].rpn_mode = v;
	} else if(svar == "rpn_syntax") {
		if(v) {
			if(mode_index == 1) evalops.parse_options.parsing_mode = PARSING_MODE_RPN;
			else modes[mode_index].eo.parse_options.parsing_mode = PARSING_MODE_RPN;
		}
	} else if(svar == "limit_implicit_multiplication") {
		if(mode_index == 1) {
			evalops.parse_options.limit_implicit_multiplication = v;
			printops.limit_implicit_multiplication = v;
		} else {
			modes[mode_index].eo.parse_options.limit_implicit_multiplication = v;
			modes[mode_index].po.limit_implicit_multiplication = v;
		}
	} else if(svar == "parsing_mode") {
		if((evalops.parse_options.parsing_mode != PARSING_MODE_RPN || version_numbers[0] > 3 || (version_numbers[0] == 3 && version_numbers[1] > 15)) && v >= PARSING_MODE_ADAPTIVE && v <= PARSING_MODE_RPN) {
			if(mode_index == 1) {
				evalops.parse_options.parsing_mode = (ParsingMode) v;
				if(evalops.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL || evalops.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) implicit_question_asked = true;
			} else {
				modes[mode_index].eo.parse_options.parsing_mode = (ParsingMode) v;
				if(modes[mode_index].eo.parse_options.parsing_mode == PARSING_MODE_CONVENTIONAL || modes[mode_index].eo.parse_options.parsing_mode == PARSING_MODE_IMPLICIT_MULTIPLICATION_FIRST) implicit_question_asked = true;
			}
		}
	} else if(svar == "simplified_percentage") {
		if(v > 0 && !VERSION_AFTER(5, 0, 0)) v = -1;
		if(mode_index == 1) simplified_percentage = v;
		else modes[mode_index].simplified_percentage = v;
	} else if(svar == "implicit_question_asked") {
		if(mode_index == 1) implicit_question_asked = v;
		else modes[mode_index].implicit_question_asked = v;
	} else if(svar == "default_assumption_type") {
		if(v >= ASSUMPTION_TYPE_NONE && v <= ASSUMPTION_TYPE_BOOLEAN) {
			if(v < ASSUMPTION_TYPE_NUMBER && version_numbers[0] < 1) v = ASSUMPTION_TYPE_NUMBER;
			if(v == ASSUMPTION_TYPE_COMPLEX && version_numbers[0] < 2) v = ASSUMPTION_TYPE_NUMBER;
			if(mode_index == 1) CALCULATOR->defaultAssumptions()->setType((AssumptionType) v);
			else modes[mode_index].at = (AssumptionType) v;
		}
	} else if(svar == "default_assumption_sign") {
		if(v >= ASSUMPTION_SIGN_UNKNOWN && v <= ASSUMPTION_SIGN_NONZERO) {
			if(v == ASSUMPTION_SIGN_NONZERO && version_numbers[0] == 0 && (version_numbers[1] < 9 || (version_numbers[1] == 9 && version_numbers[2] == 0))) {
				v = ASSUMPTION_SIGN_UNKNOWN;
			}
			if(mode_index == 1) CALCULATOR->defaultAssumptions()->setSign((AssumptionSign) v);
			else modes[mode_index].as = (AssumptionSign) v;
		}
	} else if(svar == "spacious") {
		if(mode_index == 1) printops.spacious = v;
		else modes[mode_index].po.spacious = v;
	} else if(svar == "excessive_parenthesis") {
		if(mode_index == 1) printops.excessive_parenthesis = v;
		else modes[mode_index].po.excessive_parenthesis = v;
	} else if(svar == "short_multiplication") {
		if(mode_index == 1) printops.short_multiplication = v;
		else modes[mode_index].po.short_multiplication = v;
	} else if(svar == "visible_keypad") {
		if(mode_index == 1) visible_keypad = v;
		else modes[mode_index].keypad = v;
	} else {
		return false;
	}
	return true;
}

void write_mode(FILE *file, size_t i) {
	fprintf(file, "min_deci=%i\n", modes[i].po.min_decimals);
	fprintf(file, "use_min_deci=%i\n", modes[i].po.use_min_decimals);
	fprintf(file, "max_deci=%i\n", modes[i].po.max_decimals);
	fprintf(file, "use_max_deci=%i\n", modes[i].po.use_max_decimals);
	fprintf(file, "precision=%i\n", modes[i].precision);
	fprintf(file, "interval_arithmetic=%i\n", modes[i].interval);
	fprintf(file, "interval_display=%i\n", modes[i].adaptive_interval_display ? 0 : modes[i].po.interval_display + 1);
	fprintf(file, "min_exp=%i\n", modes[i].po.min_exp);
	fprintf(file, "negative_exponents=%i\n", modes[i].po.negative_exponents);
	fprintf(file, "sort_minus_last=%i\n", modes[i].po.sort_options.minus_last);
	fprintf(file, "number_fraction_format=%i\n", modes[i].po.number_fraction_format);
	if(modes[i].po.number_fraction_format == FRACTION_COMBINED_FIXED_DENOMINATOR || modes[i].po.number_fraction_format == FRACTION_FRACTIONAL_FIXED_DENOMINATOR) fprintf(file, "number_fraction_denominator=%li\n", modes[i].fixed_denominator);
	fprintf(file, "complex_number_form=%i\n", (modes[i].complex_angle_form && modes[i].eo.complex_number_form == COMPLEX_NUMBER_FORM_CIS) ? modes[i].eo.complex_number_form + 1 : modes[i].eo.complex_number_form);
	fprintf(file, "use_prefixes=%i\n", modes[i].po.use_unit_prefixes);
	fprintf(file, "use_prefixes_for_all_units=%i\n", modes[i].po.use_prefixes_for_all_units);
	fprintf(file, "use_prefixes_for_currencies=%i\n", modes[i].po.use_prefixes_for_currencies);
	fprintf(file, "abbreviate_names=%i\n", modes[i].po.abbreviate_names);
	fprintf(file, "all_prefixes_enabled=%i\n", modes[i].po.use_all_prefixes);
	fprintf(file, "denominator_prefix_enabled=%i\n", modes[i].po.use_denominator_prefix);
	fprintf(file, "place_units_separately=%i\n", modes[i].po.place_units_separately);
	fprintf(file, "auto_post_conversion=%i\n", modes[i].eo.auto_post_conversion);
	fprintf(file, "mixed_units_conversion=%i\n", modes[i].eo.mixed_units_conversion);
	fprintf(file, "number_base=%i\n", modes[i].po.base);
	if(!modes[i].custom_output_base.isZero()) fprintf(file, "custom_number_base=%s\n", modes[i].custom_output_base.print(CALCULATOR->save_printoptions).c_str());
	fprintf(file, "number_base_expression=%i\n", modes[i].eo.parse_options.base);
	if(!modes[i].custom_input_base.isZero()) fprintf(file, "custom_number_base_expression=%s\n", modes[i].custom_input_base.print(CALCULATOR->save_printoptions).c_str());
	fprintf(file, "read_precision=%i\n", modes[i].eo.parse_options.read_precision);
	fprintf(file, "assume_denominators_nonzero=%i\n", modes[i].eo.assume_denominators_nonzero);
	fprintf(file, "warn_about_denominators_assumed_nonzero=%i\n", modes[i].eo.warn_about_denominators_assumed_nonzero);
	fprintf(file, "structuring=%i\n", modes[i].eo.structuring);
	fprintf(file, "angle_unit=%i\n", modes[i].eo.parse_options.angle_unit);
	if(modes[i].eo.parse_options.angle_unit == ANGLE_UNIT_CUSTOM) fprintf(file, "custom_angle_unit=%s\n", modes[i].custom_angle_unit.c_str());
	fprintf(file, "functions_enabled=%i\n", modes[i].eo.parse_options.functions_enabled);
	fprintf(file, "variables_enabled=%i\n", modes[i].eo.parse_options.variables_enabled);
	fprintf(file, "calculate_functions=%i\n", modes[i].eo.calculate_functions);
	fprintf(file, "calculate_variables=%i\n", modes[i].eo.calculate_variables);
	fprintf(file, "variable_units_enabled=%i\n", modes[i].variable_units_enabled);
	fprintf(file, "sync_units=%i\n", modes[i].eo.sync_units);
	fprintf(file, "unknownvariables_enabled=%i\n", modes[i].eo.parse_options.unknowns_enabled);
	fprintf(file, "units_enabled=%i\n", modes[i].eo.parse_options.units_enabled);
	fprintf(file, "allow_complex=%i\n", modes[i].eo.allow_complex);
	fprintf(file, "allow_infinite=%i\n", modes[i].eo.allow_infinite);
	fprintf(file, "indicate_infinite_series=%i\n", modes[i].po.indicate_infinite_series);
	fprintf(file, "show_ending_zeroes=%i\n", modes[i].po.show_ending_zeroes);
	fprintf(file, "rounding_mode=%i\n", modes[i].po.rounding);
	fprintf(file, "approximation=%i\n", modes[i].eo.approximation);
	fprintf(file, "interval_calculation=%i\n", modes[i].eo.interval_calculation);
	fprintf(file, "concise_uncertainty_input=%i\n", modes[i].concise_uncertainty_input);
	fprintf(file, "calculate_as_you_type=%i\n", modes[i].autocalc);
	fprintf(file, "in_rpn_mode=%i\n", modes[i].rpn_mode);
	fprintf(file, "chain_mode=%i\n", modes[i].chain_mode);
	fprintf(file, "limit_implicit_multiplication=%i\n", modes[i].eo.parse_options.limit_implicit_multiplication);
	fprintf(file, "parsing_mode=%i\n", modes[i].eo.parse_options.parsing_mode);
	fprintf(file, "simplified_percentage=%i\n", modes[i].simplified_percentage);
	if(modes[i].implicit_question_asked) fprintf(file, "implicit_question_asked=%i\n", modes[i].implicit_question_asked);
	fprintf(file, "spacious=%i\n", modes[i].po.spacious);
	fprintf(file, "excessive_parenthesis=%i\n", modes[i].po.excessive_parenthesis);
	fprintf(file, "visible_keypad=%i\n", modes[i].keypad);
	fprintf(file, "short_multiplication=%i\n", modes[i].po.short_multiplication);
	fprintf(file, "default_assumption_type=%i\n", modes[i].at);
	if(modes[i].at != ASSUMPTION_TYPE_BOOLEAN) fprintf(file, "default_assumption_sign=%i\n", modes[i].as);
}

