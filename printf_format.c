// Part of printf function suite. Parses format specifiers in the printf format
// string e.g. "%20d". format: %[flags][width][.precision][length]specifier
// Parsed results are stored in the format_specifier struct.
// Incompatible types and lengths, e.g. "%lp", result in errors of type 
// format_error. Inconsistent features, e.g. "% +d" are fixed and warnings 
// returned.
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
#include "printf_arguments.h"
#include "printf_format.h"




static format_error read_format_string_position(const char *format, 
												format_specifier *fs);
static format_error read_format_string_flags(const char *format_string, 
											 format_specifier *fs);
static format_error read_format_string_width(const char *format_string, 
											 format_specifier *fs);
static format_error read_format_string_precision(const char *format_string, 
												 format_specifier *fs);
static format_error read_format_string_length(const char* format_string, 
											  format_specifier *fs);
static format_error read_format_string_type(const char* format_string, 
											format_specifier *fs);
static int format_string_atoi(const char *string, int *value);
static format_error format_string_check_length_type(const format_specifier *fs);



// Parses the format string for a format specifier. Assumes '%' has already
// been read, so send "5d" instead of "%5d.
// Parameters:
//     format - A printf format string past the '%'.
//     fs - The format specifier to write into.
// Returns:
//     fs - Populates the format specifier pointed to with the result.
//     return - A format_error error on error, otherwise a warning or okay.
format_error read_format_string(const char *format, format_specifier *fs)
{
	format_error error = FORMAT_okay;
	format_error error2 = FORMAT_okay;
	
    fs->input_length = 0;
    fs->left_justify = false;
    fs->always_sign = false;
    fs->empty_sign = false;
    fs->alternate_form = false;
    fs->zero_padded = false;
    fs->preceding_width = 0;
    fs->width = 0;
    fs->preceding_precision = 0;
    fs->precision = -1;
    fs->position = 0;
    error = read_format_string_position(format, fs);
    if (format_error_is_error(error)) {
		return error;
	}
	error2 = format_string_check_length_type(fs);
	if (format_error_is_error(error2)) {
		return error2;
	}
	return error;
}



// Reads the format position. If it reads a width and not a position it acts
// like read_format_string_width.
// Parameters:
//     format - What is left of a printf format string.
//     fs - The format specifier to write into.
// Returns:
//     fs - Populates the format specifier pointed to with the result.
//     return - A format_error error on error, otherwise a warning or okay.
static format_error read_format_string_position(const char *format, 
												format_specifier *fs) 
{
    int position = 0;
    int characters_read;
    
    // We should only read a value if it is possibly a valid position, and not a 
    // '0' for zero padding.
    if (*format > '0' && *format <= '9') {
		characters_read = format_string_atoi(format, &position);

		fs->input_length += characters_read;
		format += characters_read;
		
		// Check if it was actually a position.
		if (*format == '$') {
			// It was a position.
			fs->input_length++;
			format++;
			fs->position = position;
			return read_format_string_flags(format, fs);
		} else {
			// It was a width;
			fs->width = position;
			return read_format_string_precision(format, fs);
		}
	} else {
		return read_format_string_flags(format, fs);
	}
}



// Parses the format string for format flags, reads them and continues. 
// Parameters:
//     format - What is left of a printf format string.
//     fs - The format specifier to write into.
// Returns:
//     fs - Populates the format specifier pointed to with the result.
//     return - A format_error error on error, otherwise a warning or okay.
static format_error read_format_string_flags(const char *format, 
											 format_specifier *fs) 
{
	format_error error = FORMAT_okay;
	format_error rest = FORMAT_okay;

    bool finished_finding_flags = false;
    while (!finished_finding_flags) {
		if (*format == '-') {
			// Check for repeated flag.
			if (fs->left_justify) {
				error = FORMAT_WARNING_repeat_flag;
			}
			fs->left_justify = true;
			fs->input_length++;
			format++;
		} else if (*format == '+') {
			// Check for repeated flag.
			if (fs->always_sign) {
				error = FORMAT_WARNING_repeat_flag;
			}
			fs->always_sign = true;
			fs->input_length++;
			format++;
		} else if (*format == ' ') {
			// Check for repeated flag.
			if (fs->empty_sign) {
				error = FORMAT_WARNING_repeat_flag;
			}
			fs->empty_sign = true;
			fs->input_length++;
			format++;
		} else if (*format == '#') {
			// Check for repeated flag.
			if (fs->alternate_form) {
				error = FORMAT_WARNING_repeat_flag;
			}
			fs->alternate_form = true;
			fs->input_length++;
			format++;
		} else if (*format == '0') {
			// Check for repeated flag.
			if (fs->zero_padded) {
				error = FORMAT_WARNING_repeat_flag;
			}
			fs->zero_padded = true;
			fs->input_length++;
			format++;
		} else {
			// No more flags, move to width.
			finished_finding_flags = true;
		}
	}
	// Give priority to other errors.
	rest = read_format_string_width(format, fs);
	if (rest == FORMAT_okay) {
		return error;
	} else {
		return rest;
	}
}



// Parses the format string for a width field.
// Parameters:
//     format - What is left of a printf format string.
//     fs - The format specifier to write into.
// Returns:
//     fs - Populates the format specifier pointed to with the result.
//     return - A format_error error on error, otherwise a warning or okay.
static format_error read_format_string_width(const char *format, 
									         format_specifier *fs) 
{
	int characters_read = 0;
	int width = 0;
    
    if (*format == '*') {
		fs->input_length++;
		format++;
		if (fs->position != 0) {
			// fs has one positional argument, so all arguments need to be.
			characters_read = format_string_atoi(format, &width);
			fs->preceding_width = width;
			fs->input_length += characters_read;
			format += characters_read;
			// If we weren't given a number, or it didn't end with '$'.
			if (width == 0 || *format != '$') {
				// Error.
				return FORMAT_ERROR_no_positional_width;
			}
			// for '$'
			format++;
			fs->input_length++;
			
		} else {
			// Not position, just preceding width.
			fs->preceding_width = 1;
		}	
    } else {
        characters_read = format_string_atoi(format, &width);
        fs->width = width;
        fs->input_length += characters_read;
        format += characters_read;
    }
    return read_format_string_precision(format, fs);
}



// Parses the format string for a precision field.
// Parameters:
//     format - What is left of a printf format string.
//     fs - The format specifier to write into.
// Returns:
//     fs - Populates the format specifier pointed to with the result.
//     return - A format_error error on error, otherwise a warning or okay.
static format_error read_format_string_precision(const char *format, 
												 format_specifier *fs)
{
	int precision = 0;
	int characters_read = 0;
    
    // if there is a precision string.
    if (*format == '.') {
		format++;
		fs->input_length++;
        if (*format == '*') {
			// It is a preceding positional argument.
			format++;
			fs->input_length++;
			if (fs->position != 0) {
				// fs has one positional argument, so all arguments need to be.
				characters_read = format_string_atoi(format, &precision);
				fs->preceding_precision = precision;
				fs->input_length += characters_read;
				format += characters_read;
				// If we weren't given a number, or it didn't end with '$'.
				if (precision == 0 || *format != '$') {
					// Error.
					return FORMAT_ERROR_no_positional_precision;
				}
				// for '$'
				format++;
				fs->input_length++;
			} else {
				// Not position, just preceding precision.
				fs->preceding_precision = 1;
			}	
		} else {
			// Not a preceding precision, just a precision.
            characters_read = format_string_atoi(format, &precision);
            fs->precision = precision;
            fs->input_length += characters_read;
            format += characters_read;
            // If we weren't given a number we assume 0, which is the
            // default return of format_string_atoi anyway.
        }
    }
    return read_format_string_length(format, fs);
}



// Parses the format string for a length field.
// Parameters:
//     format - What is left of a printf format string.
//     fs - The format specifier to write into.
// Returns:
//     fs - Populates the format specifier pointed to with the result.
//     return - A format_error error on error, otherwise a warning or okay.
static format_error read_format_string_length(const char* format, 
											  format_specifier *fs)
{
    if (strncmp(format, "hh", 2) == 0) {
        fs->length = LENGTH_hh;
        fs->input_length += 2;
        format += 2;
    } else if (*format == 'h') {
        fs->length = LENGTH_h;
        fs->input_length++;
        format++;
    } else if (strncmp(format, "ll", 2) == 0) {
        fs->length = LENGTH_ll;
        fs->input_length += 2;
        format += 2;
    } else if (*format == 'l') {
        fs->length = LENGTH_l;
        fs->input_length++;
        format++;
    } else if (*format == 'j') {
        fs->length = LENGTH_j;
        fs->input_length++;
        format++;
    } else if (*format == 'z') {
        fs->length = LENGTH_z;
        fs->input_length++;
        format++;
    } else if (*format == 't') {
        fs->length = LENGTH_t;
        fs->input_length++;
        format++;
    } else if (*format == 'L') {
        fs->length = LENGTH_L;
        fs->input_length++;
        format++;
    } else {
        fs->length = LENGTH_none;
    }
    return read_format_string_type(format, fs);
}



// Parses the format string for a type field.
// Parameters:
//     format - What is left of a printf format string.
//     fs - The format specifier to write into.
// Returns:
//     fs - Populates the format specifier pointed to with the result.
//     return - A format_error error on error, otherwise a warning or okay.
static format_error read_format_string_type(const char* format, 
											format_specifier *fs) 
{
    if (*format == 'd') {
        fs->type = TYPE_d;
    } else if (*format == 'i') {
        fs->type = TYPE_i;
    } else if (*format == 'u') {
        fs->type = TYPE_u;
    } else if (*format == 'o') {
        fs->type = TYPE_o;
    } else if (*format == 'x') {
        fs->type = TYPE_x;
    } else if (*format == 'X') {
        fs->type = TYPE_X;
    } else if (*format == 'f') {
        fs->type = TYPE_f;
    } else if (*format == 'F') {
        fs->type = TYPE_F;
    } else if (*format == 'e') {
        fs->type = TYPE_e;
    } else if (*format == 'E') {
        fs->type = TYPE_E;
    } else if (*format == 'g') {
        fs->type = TYPE_g;
    } else if (*format == 'G') {
        fs->type = TYPE_G;
    } else if (*format == 'a') {
        fs->type = TYPE_a;
    } else if (*format == 'A') {
        fs->type = TYPE_A;
    } else if (*format == 'c') {
        fs->type = TYPE_c;
    } else if (*format == 's') {
        fs->type = TYPE_s;
    } else if (*format == 'p') {
        fs->type = TYPE_p;
    } else if (*format == 'n') {
        fs->type = TYPE_n;
    } else {
        fs->type = TYPE_ERROR;
        return FORMAT_ERROR_unknown_type;
    }
    // Increment length counter by 1.
    fs->input_length++;
    return FORMAT_okay;
}



// Checks fs for invalid length and type combos, e.g. "%llp". When otherwise
// undefined behaviour would be invoked, just return a failure so printf returns
// nicely.
// Parameters:
//     fs - The format specifier to write into.
// Returns:
//     A format_error error on error, otherwise an okay.
static format_error format_string_check_length_type(const format_specifier *fs)
{
	switch (fs->type) {
        case TYPE_d:
            // PASS-THROUGH
        case TYPE_i:
            if (fs->length == LENGTH_L) {
				return FORMAT_ERROR_incompatible_length_type;
			}
            break;        
        case TYPE_u:
			// PASS-THROUGH
        case TYPE_o:
            // PASS-THROUGH
        case TYPE_x:
            // PASS-THROUGH
        case TYPE_X:
			if (fs->length == LENGTH_L) {
				return FORMAT_ERROR_incompatible_length_type;
			}
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
			// Everything but L and none is invalid.
            if (fs->length == LENGTH_h || fs->length == LENGTH_hh || 
				fs->length == LENGTH_l || fs->length == LENGTH_ll || 
				fs->length == LENGTH_j || fs->length == LENGTH_z || 
				fs->length == LENGTH_t) 
			{
				return FORMAT_ERROR_incompatible_length_type;
			}
            break;  
        case TYPE_c:
			// Everything but none and l is invalid.
            if (fs->length == LENGTH_h || fs->length == LENGTH_hh || 
				fs->length == LENGTH_ll || fs->length == LENGTH_j || 
				fs->length == LENGTH_z || fs->length == LENGTH_t || 
				fs->length == LENGTH_L) 
			{
				return FORMAT_ERROR_incompatible_length_type;
			}
            break;   
        case TYPE_s:
			// Everything but none and l is invalid.
            if (fs->length == LENGTH_h || fs->length == LENGTH_hh || 
				fs->length == LENGTH_ll || fs->length == LENGTH_j || 
				fs->length == LENGTH_z || fs->length == LENGTH_t || 
				fs->length == LENGTH_L) 
			{
				return FORMAT_ERROR_incompatible_length_type;
			}
            break;  
        case TYPE_p:
			// Everything but none is invalid.
            if (fs->length != LENGTH_none) {
				return FORMAT_ERROR_incompatible_length_type;
			}
            break;   
        case TYPE_n:
            if (fs->length == LENGTH_L) {
				return FORMAT_ERROR_incompatible_length_type;
			}
            break;
        default: 
            return FORMAT_ERROR_unknown_type;
            break;
    }
	
	return FORMAT_okay;
}



// Checks fs for unused values, and clears them from fs. Unused values are for 
// example when '+' is used with ' ', the space is ingored. Unused values that 
// result in undefined behaviour are also altered.
// Parameters:
//     fs - The format specifier to write into.
// Returns:
//     fs - Removes inconsistent values from fs.
//     return - A format_error warning if inconsistencies found, otherwise okay.
format_error format_string_check_unused_values(format_specifier *fs)
{
	format_error result = FORMAT_okay;
	
	// Fixing always_space and always_sign at same time, "+ d"
	if (fs->always_sign && fs->empty_sign) {
		result = FORMAT_WARNING_flag_does_nothing;
		fs->empty_sign = false;
	}

	// Fixing decimal with alternate form, "#i" "#d" "#u"
	if (fs->type == TYPE_d || fs->type == TYPE_i || fs->type == TYPE_u) {
		if (fs->alternate_form) {
			result = FORMAT_WARNING_flag_does_nothing;
			fs->alternate_form = false;
		}
	}
	
	// Fixing hex with sign. "+x" "+X" " x" " X"
	if (fs->type == TYPE_x || fs->type == TYPE_X) {
		if (fs->always_sign) {
			result = FORMAT_WARNING_flag_does_nothing;
			fs->always_sign = false;
		}
		if (fs->empty_sign) {
			result = FORMAT_WARNING_flag_does_nothing;
			fs->empty_sign = false;
		}
	}
	
	// Fixing flags (other than '-' with c, s, or p. 
	// " c" "0c" "+c" "#c" " s" "0s" "+s" "#c" " p" "0p" "+p" "#p". 
	if (fs->type == TYPE_c || fs->type == TYPE_s || 
		fs->type == TYPE_p) 
	{
		if (fs->always_sign) {
			result = FORMAT_WARNING_flag_does_nothing;
			fs->always_sign = false;
		}
		if (fs->empty_sign) {
			result = FORMAT_WARNING_flag_does_nothing;
			fs->empty_sign = false;
		}
		if (fs->alternate_form) {
			result = FORMAT_WARNING_flag_does_nothing;
			fs->alternate_form = false;
		}
		if (fs->zero_padded) {
			result = FORMAT_WARNING_flag_does_nothing;
			fs->zero_padded = false;
		}
	}
	
	// Fixing flags with n. " n" "0n" "+n" "#n" "-n" "8n" ".8n"
	// Note: we don't overwrite preceding precision values due to not wanting to
	// break argument parsing, even though the passed arguments will do nothing.
	if (fs->type == TYPE_n) {
		if (fs->always_sign) {
			result = FORMAT_WARNING_does_not_print;
			fs->always_sign = false;
		}
		if (fs->empty_sign) {
			result = FORMAT_WARNING_does_not_print;
			fs->empty_sign = false;
		}
		if (fs->alternate_form) {
			result = FORMAT_WARNING_does_not_print;
			fs->alternate_form = false;
		}
		if (fs->zero_padded) {
			result = FORMAT_WARNING_does_not_print;
			fs->zero_padded = false;
		}
		if (fs->left_justify) {
			result = FORMAT_WARNING_does_not_print;
			fs->left_justify = false;
		}
		if (fs->width != 0) {
			result = FORMAT_WARNING_does_not_print;
			fs->width = 0;
		}
		if (fs->precision != -1) {
			result = FORMAT_WARNING_does_not_print;
			fs->precision = -1;
		}
	}
	
	// Fixing precision where it doesn't make sense.
	if (fs->type == TYPE_c || fs->type == TYPE_p) {
		if (fs->precision != -1) {
			result = FORMAT_WARNING_precision_does_nothing;
			fs->precision = -1;
		}
	}
	
	// Fixing being zero padded (which is removed) and left justified.
	if (fs->zero_padded && fs->left_justify) {
		result = FORMAT_WARNING_flag_does_nothing;
		fs->zero_padded = false;
	}
	
	// If a precision is specified, the 0 flag is ignored.
	if (fs->precision != -1) {
		if (fs->zero_padded) {
			result = FORMAT_WARNING_flag_does_nothing;
			fs->zero_padded = false;
		}
	}
	
	return result;
}



// Reads an numberic value from the format string.
// Parameters:
//     format - What is left of a printf format string.
//     value - Pointer to the resulting number.
// Returns:
//     value - Populates with the read number. Defaults to 0.
//     return - Characters read.
static int format_string_atoi(const char *format, int *value) 
{
    int characters_read = 0;
    int current = 0;
    while ((*format >= '0') && (*format <= '9')) {
		// FIXME Bounds check this arithmetic.
        current *= 10;
        current += *format - '0';
        format++;
        characters_read++;
    }
    *value = current;
    return characters_read;
}



// Determines whether it is an actual error.
// Parameters:
//     error - The format_error to check.
// Returns:
//     true if the format_error is an error, false if okay/warning.
bool format_error_is_error(format_error error) 
{
	return (error == FORMAT_ERROR_no_positional_width || 
		    error == FORMAT_ERROR_no_positional_precision ||
		    error == FORMAT_ERROR_unknown_type ||
		    error == FORMAT_ERROR_incompatible_length_type
			);
}



// Determines whether it is a warning.
// Parameters:
//     error - The format_error to check.
// Returns:
//     true if the format_error is a warning, false if okay/error.
bool format_error_is_warning(format_error error)
{
	return (error == FORMAT_WARNING_flag_does_nothing ||
			error == FORMAT_WARNING_repeat_flag ||
			error == FORMAT_WARNING_width_does_nothing ||
			error == FORMAT_WARNING_precision_does_nothing ||
			error == FORMAT_WARNING_does_not_print
			);
}



// Debug function - prints out a format_specifier.
// Parameters:
//     fs - the format specifier to print.
void print_format_specifier(const format_specifier *fs)
{
    printf("Format string length: %d\n", fs->input_length);
    printf("Left justify: ");
    if (fs->left_justify) {
        printf("true\n");
    } else {
        printf("false\n");
    }
    printf("Always Sign: ");
    if (fs->always_sign) {
        printf("true\n");
    } else {
        printf("false\n");
    }
    
    printf("Empty Sign: ");
    if (fs->empty_sign) {
        printf("true\n");
    } else {
        printf("false\n");
    }
    
    printf("Print Preface: ");
    if (fs->alternate_form) {
        printf("true\n");
    } else {
        printf("false\n");
    }
    
    
    printf("Zero Padded: ");
    if (fs->zero_padded) {
        printf("true\n");
    } else {
        printf("false\n");
    }
    printf("Preceding Width: %d\n", fs->preceding_width);
    printf("Width: %d\n", fs->width);
    printf("Preceding Precision: %d\n", fs->preceding_precision);
    printf("Precision: %d\n", fs->precision);
    
    printf("Length: ");
    switch (fs->length) {
        case LENGTH_none:
            printf("None\n");
            break;
        case LENGTH_hh:
            printf("hh\n");
            break;
        case LENGTH_h:
            printf("h\n");
            break;
        case LENGTH_l:
            printf("l\n");
            break;
        case LENGTH_ll:
            printf("ll\n");
            break;
        case LENGTH_j:
            printf("j\n");
            break;
        case LENGTH_z:
            printf("z\n");
            break;
        case LENGTH_t:
            printf("t\n");
            break;
        case LENGTH_L:
            printf("L\n");
            break;
        default:
            printf("Error\n");
            break;
    }
    printf("Type: ");
    switch (fs->type) {
        case TYPE_d:
            printf("d\n");
            break;
        case TYPE_i:
            printf("i\n");
            break;        
        case TYPE_u:
            printf("u\n");
            break;
        case TYPE_o:
            printf("o\n");
            break;        
        case TYPE_x:
            printf("x\n");
            break;        
        case TYPE_X:
            printf("X\n");
            break;
        case TYPE_f:
            printf("f\n");
            break;
        case TYPE_F:
            printf("F\n");
            break;   
        case TYPE_e:
            printf("e\n");
            break;     
        case TYPE_E:
            printf("E\n");
            break;      
        case TYPE_g:
            printf("g\n");
            break;  
        case TYPE_G:
            printf("G\n");
            break;      
        case TYPE_a:
            printf("a\n");
            break; 
        case TYPE_A:
            printf("A\n");
            break;  
        case TYPE_c:
            printf("c\n");
            break;   
        case TYPE_s:
            printf("s\n");
            break;  
        case TYPE_p:
            printf("p\n");
            break;   
        case TYPE_n:
            printf("n\n");
            break;
        case TYPE_ERROR:
            printf("TYPE_ERROR\n");
            break;
        default: 
            printf("Error\n");
            break;
    }
    printf("Position: %d\n", fs->position);
}
