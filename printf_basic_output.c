// Part of printf function suite. Handles the output for most printf format
// specifiers, namely "%d/i", "%x/X", "%o", "%p", "%s", "%c", "%n". Does not
// include floating point output. Functions print according to the 
// format_specifier structs given to them.
//
// Copyright 2017 - Elliot Dawber. MIT licensed.


#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <assert.h>

#include "printf_definitions.h"
#include "printf_basic_output.h"

char* base_conversion_small = "0123456789abcdef";
char* base_conversion_capital = "0123456789ABCDEF";
char* null_pointer_string = "(nil)";
char* null_string_string = "(null)";

#define BUFFER_SIZE 128



static int write_decimal_negative_backwards(char *buffer, intmax_t value);

static int write_integer_backwards(char *buffer, uintmax_t value, 
								   const format_specifier *fs, int base);

static int strnlen_safe(const char* str, size_t max);


static bool pad_output(output_specifier *output, int length, 
					   char pad_character);
static bool write_backwards_buffer(output_specifier *output, const char *buffer, 
								   int length);
static bool write_forwards_buffer(output_specifier *output, const char *buffer, 
								  int length);

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
static bool write_backwards_buffer_with_padding(output_specifier *output, 
	const char *buffer, unsigned int length, const format_specifier *fs, 
	char prefix, char prefix2, unsigned int padding, 
	unsigned int precision_padding);



// Writes to output what is backwards in the buffer.
// Parameters:
//     output - Where we should output to.
//     buffer - The buffer to write out, content is stored backwards, is not a 
//         string so not 0 terminated.
//     length - Characters in the buffer to be written.
// Returns:
//     true on success, false on error.
static bool write_backwards_buffer(output_specifier *output, const char *buffer, 
								   int length) 
{
	for (int i = 0; i < length; i++) {
		if (!printf_output(output, buffer[length - 1 - i])) {
			return false;
		}	
	}
	return true;
}



// Writes to output what is in the buffer.
// Parameters:
//     output - Where we should output to.
//     buffer - The buffer to write out, content is stored forwards, is not a 
//         string so not 0 terminated.
//     length - Characters in the buffer to be written.
// Returns:
//     true on success, false on error.
static bool write_forwards_buffer(output_specifier *output, const char *buffer, 
								  int length) 
{
	for (int i = 0; i < length; i++) {
		if (!printf_output(output, buffer[i])) {
			return false;
		}
	}
	return true;
}



// Writes the contents of the buffer, appropriately padded and prefixed.
// Parameters:
//     output - Where we should output to.
//     buffer - The buffer to write out, content is stored backwards, is not a 
//         string so not 0 terminated.
//     length - Characters in the buffer to be written.
//     fs - The format specifier that describes how we should write it.
//     prefix - A char to prefix the output with. '\0' if to be ignored.
//     prefix2 - A char to prefix the output with. '\0' if to be ignored. 
//         prefix is written before prefix2.
//     padding - The amount of padding with ' '/'0' we need. Related to
//         width - length.
//     precision_padding - The amount we should pad buffer with. Related to
//         padding until we reach that precision value.
// Returns:
//     true on success, false on error.
static bool write_backwards_buffer_with_padding(output_specifier *output, 
	const char *buffer, unsigned int length, const format_specifier *fs, 
	char prefix, char prefix2, unsigned int padding, 
	unsigned int precision_padding)
{
	if (fs->zero_padded) {
		// Right justified, zero padded.
		if (prefix != 0) {
			if (!printf_output(output, prefix)) {
				return false;
			}								
		}
		if (prefix2 != 0) {
			if (!printf_output(output, prefix2)) {
				return false;
			}		
		}
		// Zero padding.
		if (!pad_output(output, padding, '0')) {
			return false;
		}
		// Precision padding.
		if (!pad_output(output, precision_padding, '0')) {
			return false;
		}
		// Our number.
		if (!write_backwards_buffer(output, buffer, length)) {
			return false;
		}
	} else if (!fs->left_justify && !fs->zero_padded) {
		// Right justified, padded with spaces.
		// Space padded.
		if (!pad_output(output, padding, ' ')) {
			return false;
		}
		if (prefix != 0) {
			if (!printf_output(output, prefix)) {
				return false;
			}					
		}
		if (prefix2 != 0) {
			if (!printf_output(output, prefix2)) {
				return false;
			}		
		}
		// Precision padding.
		if (!pad_output(output, precision_padding, '0')) {
			return false;
		}
		// Number.
		if (!write_backwards_buffer(output, buffer, length)) {
			return false;
		}
	} else if (fs->left_justify && !fs->zero_padded) {
		// Left-justified, padded with ' '
		if (prefix != 0) {
			if (!printf_output(output, prefix)) {
				return false;
			}
		}
		if (prefix2 != 0) {
			if (!printf_output(output, prefix2)) {
				return false;
			}		
		}
		// Precision padding.
		if (!pad_output(output, precision_padding, '0')) {
			return false;
		}
		// Our number.
		if (!write_backwards_buffer(output, buffer, length)) {
			return false;
		}
		// Space padding.
		if (!pad_output(output, padding, ' ')) {
			return false;
		}
	}
	
	return true;
}



// Finds the length of the string, or max, whichever is smaller. If max is 0 
// then it is safe to call when str is NULL, which is not guaranteed by other 
// strnlen definitions.
// Parameters:
//     str - The string to find the length of.
//     max - The maximum number of characters in str to read.
// Returns:
//     The length of the string, or max, whichever is smaller.
static int strnlen_safe(const char* str, size_t max) {
    size_t count = 0;
    while (count < max && *str++) {
        count++;
    }
    return count;
}



// Writes out a string to our output. This is for '%s'.
// Parameters:
//     output - Where we should output to.
//     input - The string to write. May be NULL if fs's precision is 0.
//     fs - The format specifier for how we should write it.
// Returns:
//     true on success, false on error.
bool write_string(output_specifier *output, const char *input, 
				  const format_specifier *fs) 
{
	// Holds the length of the string.
    unsigned int length = 0;
    // How much we need to pad.
    int padding_amount = 0;
   
	if (fs->precision != 0) {
		if (input == NULL) {
			input = null_string_string;
		}
	}
   
    // Find out the length of the other string. If we were given a precision we 
    // only print out up to X characters.
    if (fs->precision != -1) {
		length = strnlen_safe(input, fs->precision);
	} else {
		length = strlen(input);
	}
    
    // Determine how much to pad.
	if (fs->width > length) {
		padding_amount = fs->width - length;
	}
    
    // NOTE: We aren't using write_backwards_buffer_with_padding because this
    // input string is forwards, not backwards.
	
	if (!fs->left_justify) {
		// Right justified, padded with spaces.
		if (!pad_output(output, padding_amount, ' ')) {
			return false;
		}
		if (!write_forwards_buffer(output, input, length)) {
			return false;
		}
	} else if (fs->left_justify) {
		// Left-justified, padded with ' '
		if (!write_forwards_buffer(output, input, length)) {
			return false;
		}
		if (!pad_output(output, padding_amount, ' ')) {
			return false;
		}
	}
	return true;
}



// Writes out a character to our output. This is for '%c'.
// Parameters:
//     output - Where we should output to.
//     value - The char we need to write, as a uintmax_t.
//     fs - The format specifier for how we should write it.
// Returns:
//     true on success, false on error.
bool write_character(output_specifier *output, uintmax_t value, 
					 const format_specifier *fs) 
{
	// Buffer for holding it backwards.
    char buffer = value;
	// How much we need to pad.
    int padding_amount = 0;
   
    if (fs->width > 1) {
		padding_amount = fs->width - 1;
	}

    return write_backwards_buffer_with_padding(output, &buffer, 1, fs, 0, 0, 
											   padding_amount, 0);
}



// Writes out the number of characters written up to this point into the 
// pointer. This is for '%n'.
// Parameters:
//     output - information about our output.
//     pointer - where we should write the characters written.
//     fs - The format specifier for how we should write it.
// Returns:
//     pointer - characters written is stored here.
//     return - true on success, false on error.
bool write_characters_written(output_specifier *output, void *pointer, 
							  const format_specifier *fs)
{
	// Appropriate pointers for different sized outputs.
	signed char *p_char;
	short *p_short;
	int *p_int;
    long int *p_l_int;
    long long int *p_ll_int;
    intmax_t *p_intmax;
    size_t *p_size;
    ptrdiff_t *p_ptrdiff;
	
	if (pointer == NULL) {
		return false;
	}
	
	switch (fs->length) {
		case LENGTH_none:
			p_int = (int*) pointer;
			*p_int = output->characters_written;
			break;
		case LENGTH_hh:
			p_char = (signed char*) pointer;
			*p_char = output->characters_written;
			break;
		case LENGTH_h:
			p_short = (short*) pointer;
			*p_short = output->characters_written;
			break;
		case LENGTH_l:
			p_l_int = (long*) pointer;
			*p_l_int = output->characters_written;
			break;
		case LENGTH_ll:
			p_ll_int = (long long int*) pointer;
			*p_ll_int = output->characters_written;
			break;
		case LENGTH_j:
			p_intmax = (intmax_t*) pointer;
			*p_intmax = output->characters_written;
			break;
		case LENGTH_z:
			p_size = (size_t*) pointer;
			*p_size = output->characters_written;
			break;
		case LENGTH_t:
			p_ptrdiff = (ptrdiff_t*) pointer;
			*p_ptrdiff = output->characters_written;
			break;
		default:
			// Note: this should never be reached, any errors should have been 
			// caught earlier.
			return false;
			break;
	}
	return true;
}



// Writes out a pointer to our output. This is for '%p'. Writes out as '%#x' 
// with appropriate length. Writes out differently for NULL.
// Parameters:
//     output - Where we should output to.
//     pointer - the pointer to print out, may be NULL.
//     fs - The format specifier for how we should write it.
// Returns:
//     true on success, false on error.
bool write_pointer(output_specifier *output, const void *pointer, 
				   const format_specifier *fs) 
{
	// We print out something special for null.
	char *null_value = null_pointer_string;
	
	// Write out as '%#x'.
	format_specifier pointer_fs;
	pointer_fs.input_length = 0;
    pointer_fs.left_justify = fs->left_justify;
    pointer_fs.always_sign = false;
    pointer_fs.empty_sign = false;
    pointer_fs.alternate_form = false;
    pointer_fs.zero_padded = false;
    pointer_fs.preceding_width = 0;
    pointer_fs.width = fs->width;
    pointer_fs.preceding_precision = 0;
    pointer_fs.precision = -1;
    pointer_fs.position = 0;
    pointer_fs.length = LENGTH_none;
    pointer_fs.type = TYPE_x;
	
	if (pointer == NULL) {
		return write_string(output, null_value, &pointer_fs);
	} else {
		pointer_fs.alternate_form = true;
		return write_hexadecimal(output, (uintmax_t) pointer, &pointer_fs);
	}
}



// Writes a negative decimal number into buffer backwards. This is for "%d" with
// negative numbers. Does not terminate the string with '\0'.
// Parameters:
//     buffer - the buffer to write into.
//     value - the value to write, must be negative.
// Returns:
//     the number of characters written.
static int write_decimal_negative_backwards(char *buffer, intmax_t value)
{
    int length = 0;
   
    // While the number is still greater than 0.
    while (value / 10 != 0) {
        *buffer++ = '0' - (value % 10);
        value /= 10;
        length += 1;
    }
    // We need to do that again, for the ones column.
    *buffer++ = '0' - (value % 10);
    length += 1;
    
    return length;
}



// Writes out the value as a hexadecimal number to our output according to the 
// options in the format specifier. This is for "%x".
// Parameters:
//     output - Where we should output to.
//     value - The value to write out.
//     fs - The format specifier for how we should write it.
// Returns:
//     true on success, false on error.
bool write_hexadecimal(output_specifier *output, uintmax_t value, 
					   const format_specifier *fs) 
{
    // Buffer for holding it backwards.
    char buffer[BUFFER_SIZE];
    // The length of our buffer.
    unsigned int length = 0;
    // The length of our buffer, or it padded, which ever is longer.
    unsigned int precision_length = 0;
    // Amount to pad with ' '/'0'.
    unsigned int padding_amount = 0;
    // Amount to pad our number to match precision.
    unsigned int precision_padding = 0;
    // Prefix characters.
    char zero_char = 0;
    char x_char = 0;
   
    // Determine prefix characters.
	if (fs->alternate_form) {
		if (fs->type == TYPE_X) {
			x_char = 'X';
		} else {
			x_char = 'x';
		}
		zero_char = '0';
	}
	
    // Write it backwards into our buffer.
    // For precision and value of 0 we print nothing, not '0'.
    if (fs->precision == 0 && value == 0) {
		length = 0;
	} else {
		length = write_integer_backwards(buffer, value, fs, 16);
	}
    
    // Determine if we need to pad our number up to precision.
    if (fs->precision == -1) {
		precision_length = length;
	} else {
		if ((unsigned int) fs->precision > length) {
			precision_length = fs->precision;
			precision_padding = fs->precision - length;
		} else {
			precision_length = length;
		}
	}

	// Determine if we need to pad our number up to width.
    if (fs->alternate_form) {
		if (fs->width > (unsigned int) precision_length + 2) {
			padding_amount = fs->width - precision_length - 2;
		}
	} else {
		if (fs->width > (unsigned int) precision_length) {
			padding_amount = fs->width - precision_length;
		}
	}

    return write_backwards_buffer_with_padding(output, buffer, length, fs, 
						zero_char, x_char, padding_amount, precision_padding);
}



// Writes out a negative decimal number to our output. This is for '%d' with
// negative numbers.
// Parameters:
//     output - Where we should output to.
//     value - The value to write out. Must be negative.
//     fs - The format specifier for how we should write it.
// Returns:
//     true on success, false on error.
bool write_decimal_negative(output_specifier *output, uintmax_t value, 
							const format_specifier *fs)
{
	// Buffer for holding it backwards.
    char buffer[BUFFER_SIZE];
    // The length of our buffer.
    unsigned int length = 0;
    // The length of our buffer, or it padded, which ever is longer.
    unsigned int precision_length = 0;
    // Amount to pad with ' '/'0'.
    unsigned int padding_amount = 0;
    // Amount to pad our number to match precision.
    unsigned int precision_padding = 0;   
   
    // Write it backwards into our buffer.
    length = write_decimal_negative_backwards(buffer, value);
    
    // Determine whether we need to pad our number up to precision.
    if (fs->precision == -1) {
		precision_length = length;
	} else {
		if ((unsigned int) fs->precision > length) {
			precision_length = fs->precision;
			precision_padding = fs->precision - length;
		} else {
			precision_length = length;
		}
	}
	
	// Determine whether we need to pad our output up to width.
    if (fs->width > precision_length + 1) {
		padding_amount = fs->width - precision_length - 1;
	}
    
    return write_backwards_buffer_with_padding(output, buffer, length, fs, '-', 
										0, padding_amount, precision_padding);
}



// Writes out a positive decimal number to our output. This is for '%d'.
// Parameters:
//     output - Where we should output to.
//     value - The value to write out.
//     fs - The format specifier for how we should write it.
// Returns:
//     true on success, false on error.
bool write_decimal_positive(output_specifier *output, uintmax_t value, 
							const format_specifier *fs)
{
    // Buffer for holding it backwards.
    char buffer[BUFFER_SIZE];
    // The length of our buffer.
    unsigned int length = 0;
    // The length of our buffer, or it padded, which ever is longer.
    unsigned int precision_length = 0;
    // Amount to pad with ' '/'0'.
    unsigned int padding_amount = 0;
    // Amount to pad our number to match precision.
    unsigned int precision_padding = 0;
    // Prefix characters.
    char plus_char = 0;
	
    // Write it backwards into our buffer.
    // For precision and value of 0 we print nothing, not a '0'.
    if (fs->precision == 0 && value == 0) {
		length = 0;
	} else {
		length = write_integer_backwards(buffer, value, fs, 10);
	}
	
	// Whether we should pad our number up to precision.
	if (fs->precision == -1) {
		precision_length = length;
	} else {
		if ((unsigned int) fs->precision > length) {
			precision_length = fs->precision;
			precision_padding = fs->precision - length;
		} else {
			precision_length = length;
		}
	}
	
	// Whether we should pad our number up to width.
    if (fs->always_sign || fs->empty_sign) {
		// Writing sign so we pad one less.
		if (fs->width > precision_length + 1) {
			padding_amount = fs->width - precision_length - 1;
		}
	} else {
		if (fs->width > precision_length) {
			padding_amount = fs->width - precision_length;
		}
	}
	
	// Determine prefix.
	if (fs->always_sign) {
		plus_char = '+';
	} else if (fs->empty_sign) {
		plus_char = ' ';
	}    
	
	return write_backwards_buffer_with_padding(output, buffer, length, fs, 
							plus_char, 0, padding_amount, precision_padding);
}



// Writes out a number to our output as octal. This is for '%o'.
// Parameters:
//     output - Where we should output to.
//     value - The value to write out. Must be negative.
//     fs - The format specifier for how we should write it.
// Returns:
//     fs - Modifies if we are removing '#' flag as unnecessary.
//     true on success, false on error.
bool write_octal(output_specifier *output, uintmax_t value, 
				 format_specifier *fs) 
{
    // Buffer for holding it backwards.
    char buffer[BUFFER_SIZE];
    // The length of our buffer.
    unsigned int length = 0;
    // The length of our buffer, or it padded, which ever is longer.
    unsigned int precision_length = 0;
    // Amount to pad with ' '/'0'.
    unsigned int padding_amount = 0;
    // Amount to pad our number to match precision.
    unsigned int precision_padding = 0;
    // Prefix characters.
	char zero_char = 0;
	
    // Write it backwards into our buffer.
    // For precision and value of zero we print nothing, not a '0'.
    if (fs->precision == 0 && value == 0) {
		length = 0;
	} else {
		length = write_integer_backwards(buffer, value, fs, 8);
	}
       
    // Whether we need to pad our number up to precision.
    if (fs->precision == -1) {
		precision_length = length;
	} else {
		if ((unsigned int) fs->precision > length) {
			precision_length = fs->precision;
			precision_padding = fs->precision - length;
		} else {
			precision_length = length;
		}
	}
    
    // If we are printing 0s in front because of the precision we ignore
    // alternate_form.
    if (precision_length > length) {
		fs->alternate_form = false;
	}
    
    // Whether we need to pad our number up to precision.
    if (fs->alternate_form) {
		if (fs->width > precision_length + 1) {
			padding_amount = fs->width - precision_length - 1;
		}
	} else {
		if (fs->width > precision_length) {
			padding_amount = fs->width - precision_length;
		}
	}
	
	// Set prefix.
	if (fs->alternate_form) {
		zero_char = '0';
	}
	
	return write_backwards_buffer_with_padding(output, buffer, length, fs, 
							zero_char, 0, padding_amount, precision_padding);
}



// Writes out a positive number backwards into our buffer. The base of our
// resulting number is given as input. Does not terminate the buffer with '\0'.
// Parameters:
//     buffer - Buffer to write backwards into.
//     value - The value to write out.
//     fs - The format specifier for how we should write it.
//     base - the base to write out in. E.g. 8 = octal, 10 = decimal.
// Returns:
//     the number of characters written.
static int write_integer_backwards(char *buffer, uintmax_t value, 
								   const format_specifier *fs, int base) 
{
    int length = 0;
    int temp = 0;
    
    // Determine character set to use, "%X" gives uppercase letters.
    char *char_values = base_conversion_small;
    if (fs->type == TYPE_X) {
		char_values = base_conversion_capital;
	}
    
    // While the number is still greater than 0.
    while (value / base != 0) {
        temp = value % base;
        *buffer++ = char_values[temp];
        value /= base;
        length += 1;
    }
    // We need to do that again, for the ones column.
    temp = value % base;
    *buffer++ = char_values[temp];
    length += 1;

    return length;
}



// Writes out a positive integer to our output according to the format 
// specifier. This is for '%d'/'%x'/'%o'/'%u'.
// Parameters:
//     output - Where we should output to.
//     value - The value to write out. Must be negative.
//     fs - The format specifier for how we should write it.
// Returns:
//     true on success, false on error.
bool write_integer_positive(output_specifier *output, uintmax_t value, 
							format_specifier *fs) 
{
    switch(fs->type) {
		case TYPE_u:
			// PASS-THROUGH
        case TYPE_d:
            // PASS-THROUGH
        case TYPE_i:
            return write_decimal_positive(output, value, fs);
            break;
        case TYPE_o:
            return write_octal(output, value, fs);
            break;
        case TYPE_x:
            // PASS-THROUGH
        case TYPE_X:
            return write_hexadecimal(output, value, fs);
            break;
        default:
			// Note: Should never be called, this would be an error elsewhere in
			// the code.
            return false;
            break;
    }
    return false;
}



// Writes out the pad_character to output length times.
// Parameters:
//     output - Where we should output to.
//     length - Number of times to write out the character. May be 0.
//     pad_character - Character to write out.
// Returns:
//     true on success, false on error.
static bool pad_output(output_specifier *output, int length, char pad_character)
{
	for (int i =  0; i < length; i++) {
		if(!printf_output(output, pad_character)) {
			return false;
		}
	}
	return true;
}
