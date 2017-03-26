// Part of printf function suite. See other files for usage instructions.
//
// Copyright 2017 - Elliot Dawber. MIT licensed.

#ifndef PRINTF_FORMAT_H
#define PRINTF_FORMAT_H

#include "printf_definitions.h"

// Format string functions.
void print_format_specifier(const format_specifier *fs);
format_error read_format_string(const char *format_string, 
								format_specifier *fs);
bool format_error_is_error(format_error error);
bool format_error_is_warning(format_error error);
format_error format_string_check_unused_values(format_specifier *fs);




#endif // PRINTF_FORMAT_H
