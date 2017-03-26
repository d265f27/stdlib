// Part of printf function suite. See other files for usage instructions.
//
// Copyright 2017 - Elliot Dawber. MIT licensed.

#ifndef PRINTF_ARGUMENTS_H
#define PRINTF_ARGUMENTS_H

#include "printf_definitions.h"

intmax_t pop_or_load_integer(const format_specifier *fs, va_list_s *valist, 
					bool using_positions, positional_info *positional_items);
uintmax_t pop_or_load_unsigned_integer(const format_specifier *fs, 
	va_list_s *valist, bool using_positions, positional_info *positional_items);
uintmax_t pop_or_load_character(const format_specifier *fs, va_list_s *valist, 
					bool using_positions, positional_info *positional_items);
char* pop_or_load_string(const format_specifier *fs, va_list_s *valist, 
					bool using_positions, positional_info *positional_items);
void* pop_or_load_pointer(const format_specifier *fs, va_list_s *valist, 
					bool using_positions, positional_info *positional_items);
long double pop_or_load_floating_point(const format_specifier *fs, 
	va_list_s *valist, bool using_positions, positional_info *positional_items);
int pop_or_load_width_precision(va_list_s *valist, bool using_positions, 
							positional_info *positional_items, int position);
void* pop_or_load_n_pointer(const format_specifier *fs, va_list_s *valist, 
					bool using_positions, positional_info *positional_items);

void pop_and_store_cleanup(positional_info_array *pia, int count);

void print_positional_info_stuff(const positional_info *items, int count);

bool parse_format_string_for_positions(const char* format, 
									   positional_info_array *pia, int *max);
bool pop_and_store_argument_list(positional_info_array *pia, int count, 
								 va_list_s *valist);

#endif // PRINTF_ARGUMENTS_H
