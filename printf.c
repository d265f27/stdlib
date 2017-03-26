// C/Posix standard library function printf and its derivatives.
// Includes printf, fprintf, sprintf, snprintf, asprintf (for allocated string),
// dprintf (for file descriptors), and vprintf forms of all. Allows for posix
// positional arguments.
// 
// printf.c - provides printf family functions, generic input/output.
// printf_arguments.c/h - va_arg / posix positional parsing.
// printf_basic_output.c/h - everything but floating point output.
// printf_format.c/h - printf format string parsing helpers.
// printf_definitons.h - general data structures and functions.
//
// TODO: printf_s bounds checked versions.
//       wide character support, both input and output.
//       floating point support.
//
// Copyright 2017 - Elliot Dawber. MIT licensed.

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>
#include <unistd.h>

#include "printf_definitions.h"
#include "printf_basic_output.h"
#include "printf_arguments.h"
#include "printf_format.h"



#define BASE_ALLOCATED_STRING_SIZE 16



static bool generic_printf(output_specifier *output, const char *format, 
						   va_list_s *valist);
static bool printf_output_asprintf(output_specifier *output, char c);
static bool printf_output_dprintf(output_specifier *output, char c);
static bool printf_output_fprintf(output_specifier *output, char c);
static bool printf_output_sprintf(output_specifier *output, char c);



int new_printf(const char *format, ...)
{	
	output_specifier output;
	output.type = OUTPUT_stream;
	output.stream = stdout;
	output.string = NULL;
	output.fd = 0;
	output.allocated_size = 0;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;
	
	assert(format != NULL);
	if (format == NULL) {
		return -1;
	}
	
	va_list_s valist;
	va_start(valist.valist, format);
	
	bool result = generic_printf(&output, format, &valist);
	
	va_end(valist.valist);
	
	if (result) {
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_vprintf(const char *format, va_list args)
{
	output_specifier output;
	output.type = OUTPUT_stream;
	output.stream = stdout;
	output.fd = 0;
	output.string = NULL;
	output.allocated_size = 0;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;
	
	assert(format != NULL);
	if (format == NULL) {
		return -1;
	}
	
	va_list_s valist;
	va_copy(valist.valist, args);
	
	bool result = generic_printf(&output, format, &valist);
	
	// We have to end our copy of args, their copy is ended by client.
	va_end(valist.valist);
	
	if (result) {
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_fprintf(FILE *stream, const char *format, ...)
{
	output_specifier output;
	output.type = OUTPUT_stream;
	output.stream = stream;
	output.string = NULL;
	output.fd = 0;
	output.allocated_size = 0;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;

	assert(format != NULL);
	if (format == NULL) {
		return -1;
	}
	
	va_list_s valist;
	va_start(valist.valist, format);
	
	bool result = generic_printf(&output, format, &valist);
	
	va_end(valist.valist);
	
	if (result) {
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_vfprintf(FILE *stream, const char *format, va_list args)
{
	output_specifier output;
	output.type = OUTPUT_stream;
	output.stream = stream;
	output.string = NULL;
	output.fd = 0;
	output.allocated_size = 0;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;

	assert(format != NULL);
	if (format == NULL) {
		return -1;
	}
	
	va_list_s valist;
	va_copy(valist.valist, args);
	
	bool result = generic_printf(&output, format, &valist);
	
	// We have to end our copy of args, their copy is ended by client.
	va_end(valist.valist);
	
	if (result) {
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_sprintf(char *str, const char *format, ...)
{
	output_specifier output;
	output.type = OUTPUT_string;
	output.stream = NULL;
	output.string = str;
	output.fd = 0;
	output.allocated_size = 0;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;
	
	assert(format != NULL);
	assert(str != NULL);
	if (format == NULL || str == NULL) {
		return -1;
	}

	va_list_s valist;
	va_start(valist.valist, format);
	
	bool result = generic_printf(&output, format, &valist);
	
	va_end(valist.valist);
	
	if (result) {
		*(output.string) = 0;
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_snprintf(char *str, size_t size, const char *format, ...)
{
	output_specifier output;
	output.type = OUTPUT_string;
	output.stream = NULL;
	output.string = str;
	output.fd = 0;
	output.allocated_size = 0;
	output.character_limit = size;
	output.characters_written = 0;

	assert(format != NULL);
	if (format == NULL) {
		return -1;
	}
	if (size != 0) {
		assert(str != NULL);
		if (str == NULL) {
			return -1;
		}
	}
	
	va_list_s valist;
	va_start(valist.valist, format);
	
	bool result = generic_printf(&output, format, &valist);
	
	va_end(valist.valist);

	if (result) {
		if (size != 0) {
			*(output.string) = 0;
		}
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_vsprintf(char *str, const char *format, va_list args)
{
	output_specifier output;
	output.type = OUTPUT_string;
	output.stream = NULL;
	output.string = str;
	output.fd = 0;
	output.allocated_size = 0;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;
		
	assert(format != NULL);
	assert(str != NULL);
	if (format == NULL || str == NULL) {
		return -1;
	}
	
	va_list_s valist;
	va_copy(valist.valist, args);
	
	bool result = generic_printf(&output, format, &valist);
	
	// We have to end our copy of args, their copy is ended by client.
	va_end(valist.valist);
	
	if (result) {
		*(output.string) = 0;
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	output_specifier output;
	output.type = OUTPUT_string;
	output.stream = NULL;
	output.string = str;
	output.fd = 0;
	output.allocated_size = 0;
	output.character_limit = size;
	output.characters_written = 0;
	
	assert(format != NULL);
	if (format == NULL) {
		return -1;
	}
	if (size != 0) {
		assert(str != NULL);
		if (str == NULL) {
			return -1;
		}
	}
	
	va_list_s valist;
	va_copy(valist.valist, args);
	
	bool result = generic_printf(&output, format, &valist);
	
	// We have to end our copy of args, their copy is ended by client.
	va_end(valist.valist);
	
	if (result) {
		if (size != 0) {
			*(output.string) = 0;
		}
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_asprintf(char **strp, const char *format, ...)
{	
	output_specifier output;
	output.type = OUTPUT_allocated_string;
	output.stream = NULL;
	output.fd = 0;
	output.string = malloc(BASE_ALLOCATED_STRING_SIZE);
	output.allocated_size = BASE_ALLOCATED_STRING_SIZE;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;
	
	assert(format != NULL);
	assert(strp != NULL);
	if (format == NULL || strp == NULL) {
		free(output.string);
		return -1;
	}
	
	// If we couldn't allocate space.
	if (output.string == NULL) {
		*strp = NULL;
		return -1;
	}
	
	va_list_s valist;
	va_start(valist.valist, format);
	
	bool result = generic_printf(&output, format, &valist);
	
	va_end(valist.valist);
	
	// FIXME What is strp when response is error/-1?
	if (result) {
		// We need to '\0' the string.
		if (output.characters_written >= output.allocated_size) {
			// We need to allocate more space.
			char *string = realloc(output.string, output.allocated_size + 1);
			if (string == NULL) {
				// We couldn't realloc more space.
				free(output.string);
				*strp = NULL;
				return -1;
			}
			output.string = string;
			output.allocated_size *= 2;
		}
		*(output.string + output.characters_written) = '\0';
		*strp = output.string;

		return output.characters_written;
	} else {
		*strp = NULL;
		return -1;
	}
}



int new_vasprintf(char **strp, const char *format, va_list args)
{
	output_specifier output;
	output.type = OUTPUT_allocated_string;
	output.stream = NULL;
	output.fd = 0;
	output.string = malloc(BASE_ALLOCATED_STRING_SIZE);
	output.allocated_size = BASE_ALLOCATED_STRING_SIZE;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;
	
	assert(format != NULL);
	assert(strp != NULL);
	if (format == NULL || strp == NULL) {
		free(output.string);
		return -1;
	}
	
	// If we couldn't allocate space.
	if (output.string == NULL) {
		*strp = NULL;
		return -1;
	}
	
	va_list_s valist;
	va_copy(valist.valist, args);
	
	bool result = generic_printf(&output, format, &valist);
	
	// We have to end our copy of args, their copy is ended by client.
	va_end(valist.valist);
	
	// FIXME What is strp when response is error/-1?
	if (result) {
		// We need to '\0' the string.
		if (output.characters_written >= output.allocated_size) {
			// We need to allocate more space.
			char *string = realloc(output.string, output.allocated_size + 1);
			if (string == NULL) {
				// We couldn't realloc more space.
				free(output.string);
				*strp = NULL;
				return -1;
			}
			output.string = string;
			output.allocated_size *= 2;
		}
		*(output.string + output.characters_written) = '\0';
		*strp = output.string;

		return output.characters_written;
	} else {
		*strp = NULL;
		return -1;
	}
}



int new_dprintf(int fd, const char *format, ...)
{	
	output_specifier output;
	output.type = OUTPUT_stream;
	output.stream = NULL;
	output.string = NULL;
	output.fd = fd;
	output.allocated_size = 0;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;
	
	assert(format != NULL);
	if (format == NULL) {
		return -1;
	}
	
	va_list_s valist;
	va_start(valist.valist, format);
	
	bool result = generic_printf(&output, format, &valist);
	
	va_end(valist.valist);
	
	if (result) {
		return output.characters_written;
	} else {
		return -1;
	}
}



int new_vdprintf(int fd, const char *format, va_list args)
{
	output_specifier output;
	output.type = OUTPUT_file_descriptor;
	output.stream = NULL;
	output.fd = fd;
	output.string = NULL;
	output.allocated_size = 0;
	output.character_limit = SIZE_MAX;
	output.characters_written = 0;
	
	assert(format != NULL);
	if (format == NULL) {
		return -1;
	}
	
	va_list_s valist;
	va_copy(valist.valist, args);
	
	bool result = generic_printf(&output, format, &valist);
	
	// We have to end our copy of args, their copy is ended by client.
	va_end(valist.valist);
	
	if (result) {
		return output.characters_written;
	} else {
		return -1;
	}
}



// Outputs the char to the correct output. May not output anything if we would 
// be past our character limit.
// Parameters:
//     output - where we should output to.
//     c - the character to output.
// Returns:
//     true on success, false on error.
bool printf_output(output_specifier *output, char c)
{
	if (output->type == OUTPUT_string) {
		return printf_output_sprintf(output, c);
	} else if (output->type == OUTPUT_stream) {
		return printf_output_fprintf(output, c);
	} else if (output->type == OUTPUT_file_descriptor) {
		return printf_output_dprintf(output, c);
	} else if (output->type == OUTPUT_allocated_string) {
		return printf_output_asprintf(output, c);
	}
	// Should never be reached.
	return false;
}



// Outputs the char for sprintf/snprintf. May not output anything if we would 
// be past our character limit.
// Parameters:
//     output - where we should output to.
//     c - the character to output.
// Returns:
//     true on success, false on error.
static bool printf_output_sprintf(output_specifier *output, char c)
{
	if (output->character_limit == 0) {
		// We should never write anything.
		output->characters_written++;
		return true;
	}
	
	if (output->characters_written >= output->character_limit - 1) {
		// We've reached out max characters already. Leave room for \0.
		output->characters_written++;
		return true;
	}
	
	*(output->string)++ = c;
	output->characters_written++;
	return true;
}



// Outputs the char for printf/fprintf.
// Parameters:
//     output - where we should output to.
//     c - the character to output.
// Returns:
//     true on success, false on error.
static bool printf_output_fprintf(output_specifier *output, char c)
{
	if (putc(c, output->stream) == EOF) {
		return false;
	}
	
	output->characters_written++;
	return true;
}



// Outputs the char for dprintf.
// Parameters:
//     output - where we should output to.
//     c - the character to output.
// Returns:
//     true on success, false on error.
static bool printf_output_dprintf(output_specifier *output, char c)
{
	// This checks for both whether an error occured or if
	// nothing was written.
	// FIXME If we are interrupted by an interrupt this may return 0, 
	// should we try again?
	if (write(output->fd, &c, 1) != 1) {
		return false;
	}
	
	output->characters_written++;
	return true;
}



// Outputs the char for asprintf.
// Parameters:
//     output - where we should output to.
//     c - the character to output.
// Returns:
//     true on success, false on error.
static bool printf_output_asprintf(output_specifier *output, char c)
{
	// Check if we have enough space to write it.
	if (output->characters_written >= output->allocated_size) {
		// We need to allocate more space.
		// FIXME Can this size determination overflow?
		char *string = realloc(output->string, output->allocated_size * 2);
		if (string == NULL) {
			// We couldn't realloc more space.
			free(output->string);
			return false;
		}
		output->string = string;
		output->allocated_size *= 2;
	}
	*(output->string + output->characters_written) = c;

	output->characters_written++;
	return true;
}



// Generic printf. Serves for all the commands in the printf family. Main 
// function that reads the format string and produces output according to it.
// Parameters:
//     output - where we should output to.
//     format - the printf format string we should parse. May not be NULL.
//     valist - the struct holding the relevant va_list.
// Returns:
//     true on success, false on any error.
static bool generic_printf(output_specifier *output, const char *format, 
						   va_list_s *valist) 
{
	// For holding the values given in the va_list.
    void *pointer_value = NULL;
    char *string_value = NULL;
    intmax_t int_value = 0;
    uintmax_t uint_value = 0;
   
    int width = 0;
    int precision = 0;
    
    format_error error = FORMAT_okay;
    
    // A copy of the start of 'format', as we modify the 'format' pointer.
    const char *format_start = format;
    
    // Whether we are using positional parameters.
    bool using_positions = false;
    // Whether this is the first format specifier we have processed, used to 
    // determine if we are using positional parameters.
    bool first_element = true;
    
    // Holds the parsed positions for when we need it.
    positional_info_array pia;
    pia.size = 0;
    pia.array = NULL;
    
    // The highest position we have been given.
    int position_count = 0;
    
    bool result = false;
    
    // While format string is not end of line character.
    while (*format != '\0') {
		if (*format == '%' && *(format + 1) == '%') {
			// Starting an escaped % - "%%"
			result = printf_output(output, '%');
			if (!result) {
				return false;
			}
			format += 2;
		} else if (*format == '%') {
            // Starting a format specifier.
            format++;
            
            format_specifier fs;
            error = read_format_string(format, &fs);
            if (format_error_is_error(error)) {
				if (using_positions) {
					pop_and_store_cleanup(&pia, position_count);
					free(pia.array);
				}
				return false;
			}
			
			// Determine whether we are using positions, if we are parse the 
			// format string for positions and continue.
            if (first_element && fs.position != 0) {
				using_positions = true;
				// Parse the whole list.
				if (!parse_format_string_for_positions(format_start, &pia, 
													   &position_count))
				{
					return false;
				}
				// Populate items
				if (!pop_and_store_argument_list(&pia, position_count, valist)) 
				{
					//print_positional_info_stuff(pia.array, position_count);
					free(pia.array);
					return false;
				}				
			}
			first_element = false;

			// Check for consistency between this format string and 
			// using_positions.
            if (fs.position == 0 && using_positions) {
				// Using positions but weren't given one.
				if (using_positions) {
					pop_and_store_cleanup(&pia, position_count);
					free(pia.array);
				}
				return false;
			} else if (fs.position != 0 && !using_positions) {
				// Not using positions but were given one.
				if (using_positions) {
					pop_and_store_cleanup(&pia, position_count);
					free(pia.array);
				}
				return false;
			}
            
            // Preceding width and positional preceding width.
            if (fs.preceding_width != 0) {
				width = pop_or_load_width_precision(valist, using_positions, 
												pia.array, fs.preceding_width);
				// If width is negative then that is taken as a positive 
				// value and a '-' flag.
				if (width >= 0) {
					fs.width = width;
				} else {
					fs.left_justify = true;
					if (width == INT_MIN) {
						// Negating INT_MIN is unsafe
						// FIXME We can't actually return such a big
						// number from printf anyway.
						fs.width = INT_MAX;
					} else {
						fs.width = -width;
					}
				}
            }
            
            // Preceding precision or positional preceding precision.
			if (fs.preceding_precision != 0) {
				precision = pop_or_load_width_precision(valist, using_positions, 
											pia.array, fs.preceding_precision);
				// Explicit precision values of < 0 are ignored.
				if (precision >= 0) {
					fs.precision = precision;
				}
			}
			
			// Before we pass it to a printing function, make sure we cancel out
			// any incompatible but recoverable errors in the format string.
			format_string_check_unused_values(&fs);
			
            switch (fs.type) {
                case TYPE_d:
                    // PASS-THROUGH.
                case TYPE_i:
                    // Signed decimal integer.
                    int_value = pop_or_load_integer(&fs, valist, 
													using_positions, pia.array);
                    if (int_value >= 0) {
                        result = write_integer_positive(output, int_value, &fs);
                    } else {
						result = write_decimal_negative(output, int_value, &fs);
                    }
                    break; 
                case TYPE_o:
                    // PASS-THROUGH
                case TYPE_x:
                    // PASS-THROUGH
                case TYPE_X:
                    // PASS-THROUGH
                case TYPE_u:
                    // Unsigned decimal integer.
                    uint_value = pop_or_load_unsigned_integer(&fs, valist, 
													using_positions, pia.array);
                    result = write_integer_positive(output, uint_value, &fs);
                    break;
                case TYPE_f:
                    // PASS-THROUGH
                case TYPE_F:
					// PASS-THROUGH
                case TYPE_e:
                    // PASS-THROUGH
                case TYPE_E:
                    // PASS-THROUGH
                case TYPE_g:
                    // PASS-THROUGH
                case TYPE_G:
                    // PASS-THROUGH
                case TYPE_a:
                    // PASS-THROUGH
                case TYPE_A:
					// FIXME Implement.
                    return false;
                    break;
                case TYPE_c:
					// unsigned char
					uint_value = pop_or_load_character(&fs, valist, 
													using_positions, pia.array);
					result = write_character(output, uint_value, &fs);
                    break;
                case TYPE_s:
					// char*
					string_value = pop_or_load_string(&fs, valist, 
													using_positions, pia.array);                    
                    result = write_string(output, string_value, &fs);
                    break;
                case TYPE_p:
                    // pointer.
                    pointer_value = pop_or_load_pointer(&fs, valist, 
													using_positions, pia.array);
                    result = write_pointer(output, pointer_value, &fs);
                    break;
                case TYPE_n:
					// Important: we pop off a void*, but the real type
					// is a pointer to something else, determined by
					// the length in the fs. Convert before use.
                    pointer_value = pop_or_load_n_pointer(&fs, valist, 
													using_positions, pia.array);
                    result = write_characters_written(output, pointer_value, 
													  &fs);                    
                    break;
                default: 
					// Set it so we clean up.
					result = false;
                    break;
            } // end of switch (fs.type) 
            if (!result) {
				if (using_positions) {
					pop_and_store_cleanup(&pia, position_count);
					free(pia.array);
				}
				return false;
			}
			// Tidy up positions.
			format += fs.input_length;
		} else {
			// Just a normal letter.
			result = printf_output(output, *format);
			if (!result) {
				if (using_positions) {
					pop_and_store_cleanup(&pia, position_count);
					free(pia.array);
				}	
				return false;
			}
			format++;
        }
    }
    if (using_positions) {
		pop_and_store_cleanup(&pia, position_count);
		free(pia.array);
	}
    return true;
}

