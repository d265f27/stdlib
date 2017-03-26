// Part of printf function suite. See other files for usage instructions.
//
// Copyright 2017 - Elliot Dawber. MIT licensed.

#ifndef PRINTF_DEFINITIONS_H
#define PRINTF_DEFINITIONS_H

#include <stdint.h>
#include <stdarg.h>

typedef enum {
	FORMAT_okay,
	FORMAT_ERROR_no_positional_width,
	FORMAT_ERROR_no_positional_precision,
	FORMAT_ERROR_unknown_type,
	FORMAT_ERROR_incompatible_length_type,
	FORMAT_WARNING_flag_does_nothing,
	FORMAT_WARNING_repeat_flag,
	FORMAT_WARNING_width_does_nothing,
	FORMAT_WARNING_precision_does_nothing,
	FORMAT_WARNING_does_not_print
} format_error;

// Literally an enum of the type letters of printf format strings.
typedef enum {
	TYPE_d, TYPE_i, TYPE_u, TYPE_o, TYPE_x, TYPE_X, TYPE_f, TYPE_F, 
	TYPE_e, TYPE_E, TYPE_g, TYPE_G, TYPE_a, TYPE_A, TYPE_c, TYPE_s, 
	TYPE_p, TYPE_n, TYPE_ERROR
} format_string_types;

// Literally an enum of the length specifiers of printf format strings.
typedef enum {
	LENGTH_none, LENGTH_hh, LENGTH_h, LENGTH_l, LENGTH_ll, LENGTH_j, 
	LENGTH_z, LENGTH_t, LENGTH_L
} format_string_lengths;


typedef enum {
	OUTPUT_file_descriptor, OUTPUT_stream, OUTPUT_string, 
	OUTPUT_allocated_string
} printf_output_type;

// Holds information about how we output our characters.
typedef struct output_specifier_struct {
	printf_output_type type;
	// For use with OUTPUT_stream
	FILE *stream;
	// For use with OUTPUT_file_descriptor
	int fd;
	// For use with OUTPUT_string and OUTPUT_allocated_string
	// Note: for OUTPUT_string this refers to the next location to write to, for
	// OUTPUT_allocated_string it refers to the start of the string.
	char *string;
	// For use with OUTPUT_allocated_string;
	size_t allocated_size;
	// For use with any.
	size_t character_limit;                     // FIXME should these be
	size_t characters_written;					// size_ts or ints?
} output_specifier;

typedef struct format_specifier_struct {
	//%[flags][width][.precision][length]specifier 
	// The number of characters read in the format specifier.
    int input_length;
    // left_justify (-): whether the result in our width should be left
    bool left_justify;
    // always_sign (+): Shows a sign for positive numbers.
    bool always_sign;
    // empty_sign (space): If it is positive then a space is inserted for +.
    bool empty_sign;
    // print_preface (#) Adds prefixes - 0, 0x 0X. Some float stuff too.
    bool alternate_form;
    // zero_padded (0) Left-pads the numbers with "0".
    bool zero_padded;
    // Whether the width is instead given as a preceding va_args value. 
    // 0 is no preceding width. If just preceding width with no position then 
    // non-zero, otherwise the position.
    int preceding_width;
    // The width we should be printing into.
    unsigned int width;
    // Whether the precision is instead given as a preceding va_args value. 
    // 0 is no preceding precision. If just preceding width with no position 
    // then non-zero, otherwise the position.
    int preceding_precision;
    // The precision we should be using. -1 if not given, 0 if given 0, above 
    // zero is given that number.
    int precision;
    // The length of the value/type of input, eg. short, long.
    format_string_lengths length;
    // Type of printing output, eg. floating point, floating point as hex, etc.
    format_string_types type;
    // The position in list of parameters this is related to. 0 is no position 
    // given, otherwise the position.
    int position;
} format_specifier;

// Holds information for when we are using posix positional arguments and need
// to store the arguments for later.
typedef struct struct_positional_info {
	format_string_lengths length;
	format_string_types type;
	void *item;
} positional_info;

// Holds information for storage and retrieval of all the posix positional
// arguments passed to our function. Using .array[i] will give i+1th positional
// argument.
typedef struct struct_positional_info_array {
	int size;
	positional_info* array;
} positional_info_array;

// To share the va_list between functions and to avoid type issues like
// "expected ‘__va_list_tag (*)[1]’ but argument is of type ‘__va_list_tag **’"
// from gcc when using va_list pointers.
typedef struct struct_va_list {
	va_list valist;
} va_list_s;



format_error read_format_string(const char *format_string, 
								format_specifier *fs);
void print_format_specifier(const format_specifier *fs);
format_error format_string_check_unused_values(format_specifier *fs);


// stdout printing.
int new_printf(const char *format, ...);
int new_vprintf(const char *format, va_list args);

// File stream output.
int new_fprintf(FILE *stream, const char *format, ...);
int new_vfprintf(FILE *stream, const char *format, va_list args);

// String output.
int new_sprintf(char *str, const char *format, ...);
int new_snprintf(char *str, size_t size, const char *format, ...);
int new_vsprintf(char *str, const char *format, va_list args);
int new_vsnprintf(char *str, size_t size, const char *format, va_list args);

// Allocated string output.
int new_asprintf(char **strp, const char *format, ...);
int new_vasprintf(char **strp, const char *format, va_list args);

// File descriptor output.
int new_dprintf(int fd, const char *format, ...);
int new_vdprintf(int fd, const char *format, va_list args);

bool parse_format_string_for_positions(const char* format, 
									   positional_info_array *pia, int *max);
void print_positional_info_stuff(const positional_info *items, int count);

bool format_error_is_error(format_error error);
bool format_error_is_warning(format_error error);
bool printf_output(output_specifier *output, char c);

void test_rda(void);
	

#endif // PRINTF_DEFINITIONS_H 
