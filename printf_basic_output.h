// Part of printf function suite. See other files for usage instructions.
//
// Copyright 2017 - Elliot Dawber. MIT licensed.

#ifndef PRINTF_BASIC_OUTPUT_H
#define PRINTF_BASIC_OUTPUT_H

#include "printf_definitions.h"


bool write_characters_written(output_specifier *output, void *pointer, 
							  const format_specifier *fs);
bool write_pointer(output_specifier *output, const void *pointer, 
				   const format_specifier *fs);
bool write_decimal_positive(output_specifier *output, uintmax_t value, 
							const format_specifier *fs);
bool write_decimal_negative(output_specifier *output, uintmax_t value, 
							const format_specifier *fs);
bool write_integer_positive(output_specifier *output, uintmax_t value, 
							format_specifier *fs);
bool write_hexadecimal(output_specifier *output, uintmax_t value, 
					   const format_specifier *fs);
bool write_octal(output_specifier *output, uintmax_t value, 
				 format_specifier *fs);
bool write_string(output_specifier *output, const char *input, 
				  const format_specifier *fs);
bool write_character(output_specifier *output, uintmax_t value, 
					 const format_specifier *fs);


#endif // PRINTF_BASIC_OUTPUT_H
