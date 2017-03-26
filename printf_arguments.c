// Part of printf function suite. Handles dealing with varible length argument
// lists (va_args) and posix positional parameters.
// pop_and_store functions are used to store positional arguments for later use.
// pop_and_load functions are used to load from either previously stored 
// arguments if using positional parameters, or directly from the variable 
// argument list if not.
// positional_info_array is a dynamically resizing data structure that holds
// the stored positional arguments and information to retrieve them.
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

#define DEFAULT_PIA_SIZE 8



static bool pop_and_store_integer(positional_info *current_item, 
								  va_list_s *valist);
static bool pop_and_store_unsigned_integer(positional_info *current_item, 
										   va_list_s *valist);
static bool pop_and_store_floating_point(positional_info *current_item, 
										 va_list_s *valist);
static bool pop_and_store_string(positional_info *current_item, 
								 va_list_s *valist);
static bool pop_and_store_character(positional_info *current_item, 
									va_list_s *valist);
static bool pop_and_store_pointer(positional_info *current_item, 
								  va_list_s *valist);
static bool pop_and_store_n_pointer(positional_info *current_item, 
									va_list_s *valist);
static bool pia_check_size_and_update(positional_info_array *pia, 
									  int required_size);
static bool pia_initialise(positional_info_array *pia);



// Pops arguments off the va_list according to information and order in items.
// Cleans up the positional_info_array of malloced items on failure.
// Parameters:
//     pia - A positional_info_array filled with information from the format 
//         string.
//     count - Number of arguments to pop, positional_info must be valid for all
//         items up to count.
//     valist - Struct holding a va_list to pop off of.
// Returns:
//     pia - Places pointers into pia.
//     true on success, false on error.
bool pop_and_store_argument_list(positional_info_array *pia, int count, 
								 va_list_s *valist)
{	
	positional_info *current_item;
	
	for (int i = 0; i < count; i++) {
		current_item = pia->array + i;
		switch (current_item->type) {
			case TYPE_d:
				// PASS-THROUGH.
			case TYPE_i:
				// Signed decimal integer.
				if (!pop_and_store_integer(current_item, valist)) {
					pop_and_store_cleanup(pia, count);
					return false;
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
				if (!pop_and_store_unsigned_integer(current_item, valist)) {
					pop_and_store_cleanup(pia, count);
					return false;
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
				// Floating point number.
				if (!pop_and_store_floating_point(current_item, valist)) {
					pop_and_store_cleanup(pia, count);
					return false;
				}
				break;
			case TYPE_c:
				// Character.
				if (!pop_and_store_character(current_item, valist)) {
					pop_and_store_cleanup(pia, count);
					return false;
				}
				break;
			case TYPE_s:
			    // String.
				if (!pop_and_store_string(current_item, valist)) {
					pop_and_store_cleanup(pia, count);
					return false;
				}
				break;
			case TYPE_p:
				// Pointer.
				if (!pop_and_store_pointer(current_item, valist)) {
					pop_and_store_cleanup(pia, count);
					return false;	
				}
				break;
			case TYPE_n:
				if (!pop_and_store_n_pointer(current_item, valist)) {
					pop_and_store_cleanup(pia, count);
					return false;
				}
				break;
			default:
				// Should never reach. An error in code logic elsewhere.
				pop_and_store_cleanup(pia, count);
				return false;
				break;
		}           
	}
	return true;
}



// Either pops an integer off the valist and returns it, or if we are using
// positional arguments loads one from memory. va_list may be NULL if
// using_positions, positional_items may be NULL if not using_positions.
// Parameters:
//     fs - The format specifier for what to pop.
//     valist - Struct holding a va_list for us to pop off of.
//     using_positions - Whether we pop or load from memory.
//     positional_items - Array holding information about previously popped 
//         items stored in memory, when using_positions = true.
// Returns:
//     The value popped or loaded.
intmax_t pop_or_load_integer(const format_specifier *fs, va_list_s *valist, 
						bool using_positions, positional_info *positional_items)
{
	// So, the C standard says we must convert the popped off element of the 
	// va_list into the shorter length type before use even though they are 
	// automatically promoted to ints for the va_list. This is redundant, but 
	// we shall obey.
	intmax_t return_value = 0;
	positional_info current_item;
	void **pointer = NULL;

	if (using_positions) {
		current_item = positional_items[fs->position - 1];
		pointer = current_item.item;
	}

	switch (fs->length) {
		case LENGTH_none:
			// signed int.
			if (using_positions) {
				return_value = *((int*) pointer);
			} else {
				return_value = va_arg(valist->valist, int);
			}
			break;
		case LENGTH_hh:
		    // signed char.
			if (using_positions) {
				return_value = (signed char) *((int*) pointer);
			} else {
				return_value = (signed char) va_arg(valist->valist, int);
			}
			break;
		case LENGTH_h:
			// signed short.
			if (using_positions) {
				return_value = (short) *((int*) pointer);
			} else {
				return_value = (short) va_arg(valist->valist, int);
			}
			break;
		case LENGTH_l:
			// signed long.
			if (using_positions) {
				return_value = *((long int*) pointer);
			} else {
				return_value = va_arg(valist->valist, long int);
			}
			break;
		case LENGTH_ll:
		    // signed long long.
			if (using_positions) {
				return_value = *((long long int*) pointer);
			} else {
				return_value = va_arg(valist->valist, long long int);
			}
			break;
		case LENGTH_j:
		    // intmax_t.
			if (using_positions) {
				return_value = *((intmax_t*) pointer);
			} else {
				return_value = va_arg(valist->valist, intmax_t);
			}
			break;
		case LENGTH_z:
			// size_t.
			if (using_positions) {
				return_value = *((size_t*) pointer);
			} else {
				return_value = va_arg(valist->valist, size_t);
			}
			break;
		case LENGTH_t:
			// ptrdiff_t.
			if (using_positions) {
				return_value = *((ptrdiff_t*) pointer);
			} else {
				return_value = va_arg(valist->valist, ptrdiff_t);
			}
			break;
		default:
			// Should never be called, means an error elsewhere in the code.
			break;
	}

	return return_value;
}



// Either pops an unsigned integer off the valist and returns it, or if we are
// using positional arguments loads one from memory. va_list may be NULL if
// using_positions, positional_items may be NULL if not using_positions.
// Parameters:
//     fs - The format specifier for what to pop.
//     valist - Struct holding a va_list for us to pop off of.
//     using_positions - Whether we pop or load from memory.
//     positional_items - Array holding information about previously popped
//         items stored in memory, when using_positions = true.
// Returns:
//     The value popped or loaded.
uintmax_t pop_or_load_unsigned_integer(const format_specifier *fs, 
	va_list_s *valist, bool using_positions, positional_info *positional_items)
{
	// So, the C standard says we must convert the popped off element of the 
	// va_list into the shorter length type before use even though they are 
	// automatically promoted to ints for the va_list. This is redundant, but 
	// we shall obey.
	uintmax_t return_value = 0;
	positional_info current_item;
    void **pointer = NULL;

	if (using_positions) {
		current_item = positional_items[fs->position - 1];
		pointer = current_item.item;
	}

	switch (fs->length) {
		case LENGTH_none:
			// unsigned int.
			if (using_positions) {
				return_value = *((unsigned int*) pointer);
			} else {
				return_value = va_arg(valist->valist, unsigned int);
			}
			break;
		case LENGTH_hh:
		    // unsigned char.
			if (using_positions) {
				return_value = (unsigned char) *((unsigned int*) pointer);
			} else {
				return_value = (unsigned char) va_arg(valist->valist, 
													  unsigned int);
			}
			break;
		case LENGTH_h:
			// unsigned short.
			if (using_positions) {
				return_value = (unsigned short) *((unsigned int*) pointer);
			} else {
				return_value = (unsigned short) va_arg(valist->valist, 
													   unsigned int);
			}
			break;
		case LENGTH_l:
			// unsigned long.
			if (using_positions) {
				return_value = *((unsigned long int*) pointer);
			} else {
				return_value = va_arg(valist->valist, unsigned long int);
			};
			break;
		case LENGTH_ll:
			// unsigned long long.
			if (using_positions) {
				return_value = *((unsigned long long int*) pointer);
			} else {
				return_value = va_arg(valist->valist, unsigned long long int);
			}
			break;
		case LENGTH_j:
			if (using_positions) {
				return_value = *((uintmax_t*) pointer);
			} else {
				return_value = va_arg(valist->valist, uintmax_t);
			}
			break;
		case LENGTH_z:
			if (using_positions) {
				return_value = *((size_t*) pointer);
			} else {
				return_value = va_arg(valist->valist, size_t);
			}
			break;
		case LENGTH_t:
			if (using_positions) {
				return_value = *((ptrdiff_t*) pointer);
			} else {
				return_value = va_arg(valist->valist, ptrdiff_t);
			}
			break;
		default:
			// Should never be called, means an error elsewhere in the code.
			break;
	}
	
	return return_value;
}              



// Either pops a floating point number off the valist and returns it, or if we
// are using positional arguments loads one from memory. va_list may be NULL
// if using_positions, positional_items may be NULL if not using_positions.
// Parameters:
//     fs - The format specifier for what to pop.
//     valist - Struct holding a va_list for us to pop off of.
//     using_positions - Whether we pop or load from memory.
//     positional_items - Array holding information about previously popped
//         items stored in memory, when using_positions = true.
// Returns:
//     The value popped or loaded.
long double pop_or_load_floating_point(const format_specifier *fs, 
	va_list_s *valist, bool using_positions, positional_info *positional_items)
{
	long double return_value = 0;
	positional_info current_item;
    void **pointer = NULL;

	if (using_positions) {
		current_item = positional_items[fs->position - 1];
		pointer = current_item.item;
	}

	switch (fs->length) {
		case LENGTH_none:
			if (using_positions) {
				return_value = *((double*) pointer);
			} else {
				return_value = va_arg(valist->valist, double);
			}
			break;
		case LENGTH_L:
			if (using_positions) {
				return_value = *((long double*) pointer);
			} else {
				return_value = va_arg(valist->valist, long double);
			}
			break;
		default:
			// Should never be called, means an error elsewhere in the code.
			break;
	}
	
	return return_value;
}



// Either pops a char off the valist and returns it, or if we are using
// positional arguments loads one from memory. va_list may be NULL if
// using_positions, positional_items may be NULL if not using_positions.
// Parameters:
//     fs - The format specifier for what to pop.
//     valist - Struct holding a va_list for us to pop off of.
//     using_positions - Whether we pop or load from memory.
//     positional_items - Array holding information about previously popped
//         items stored in memory, when using_positions = true.
// Returns:
//     The value popped or loaded.
uintmax_t pop_or_load_character(const format_specifier *fs, va_list_s *valist, 
						bool using_positions, positional_info *positional_items)
{	
	// So, the C standard says we must convert the popped off element of the 
	// va_list into the shorter length type before use even though they are 
	// automatically promoted to ints for the va_list. This is redundant, but 
	// we shall obey.
	
	positional_info current_item;
    void **pointer = NULL;
	
	if (using_positions) {
		current_item = positional_items[fs->position - 1];
		pointer = current_item.item;
		return (unsigned char) *((int*) pointer);
	} else {
		return (unsigned char) va_arg(valist->valist, int);
	}
}



// Either pops a char pointer off the valist and returns it, or if we are
// using positional arguments loads one from memory. va_list may be NULL if
// using_positions, positional_items may be NULL if not using_positions.
// Parameters:
//     fs - The format specifier for what to pop.
//     valist - Struct holding a va_list for us to pop off of.
//     using_positions - Whether we pop or load from memory.
//     positional_items - Array holding information about previously popped
//         items stored in memory, when using_positions = true.
// Returns:
//     The value popped or loaded.
char* pop_or_load_string(const format_specifier *fs, va_list_s *valist, 
						bool using_positions, positional_info *positional_items)
{
	positional_info current_item;
    void **pointer = NULL;
	
	if (using_positions) {
		current_item = positional_items[fs->position - 1];
		pointer = current_item.item;
		return *((char**) pointer);
	} else {
		return va_arg(valist->valist, char*);
	}
}



// Either pops a void pointer off the valist and returns it, or if we are using
// positional arguments loads one from memory. va_list may be NULL if 
// using_positions, positional_items may be NULL if not using_positions.
// Parameters:
//     fs - The format specifier for what to pop.
//     valist - Struct holding a va_list for us to pop off of.
//     using_positions - Whether we pop or load from memory.
//     positional_items - Array holding information about previously popped
//         items stored in memory, when using_positions = true.
// Returns:
//     The value popped or loaded.
void* pop_or_load_pointer(const format_specifier *fs, va_list_s *valist, 
						bool using_positions, positional_info *positional_items)
{
	positional_info current_item;
    void **pointer = NULL;
	
	if (using_positions) {
		current_item = positional_items[fs->position - 1];
		pointer = current_item.item;
		return *((void**) pointer);
	} else {
		return va_arg(valist->valist, void*);
	}
}



// Either pops an int off the valist and returns it, or if we are using
// positional arguments loads one from memory. va_list may be NULL if 
// using_positions, positional_items may be NULL if not using_positions.
// Parameters:
//     fs - The format specifier for what to pop.
//     valist - Struct holding a va_list for us to pop off of.
//     using_positions - Whether we pop or load from memory.
//     positional_items - Array holding information about previously popped
//         items stored in memory, when using_positions = true.
// Returns:
//     The value popped or loaded.
int pop_or_load_width_precision(va_list_s *valist, bool using_positions, 
								positional_info *positional_items, int position)
{
	positional_info current_item;
    void **pointer = NULL;
    
	if (!using_positions) {
		return va_arg(valist->valist, int);
	} else {
		current_item = positional_items[position - 1];
		pointer = current_item.item;
		return *((int*) pointer);
	}
}



// Either pops a pointer off the valist and returns it, or if we are using
// positional arguments loads one from memory. va_list may be NULL if
// using_positions, positional_items may be NULL if not using_positions.
// Parameters:
//     fs - The format specifier for what to pop.
//     valist - Struct holding a va_list for us to pop off of.
//     using_positions - Whether we pop or load from memory.
//     positional_items - Array holding information about previously popped
//         items stored in memory, when using_positions = true.
// Returns:
//     The value popped or loaded as void*. Must be converted to the correct 
//         type before use.
void* pop_or_load_n_pointer(const format_specifier *fs, va_list_s *valist, 
						bool using_positions, positional_info *positional_items)
{
	positional_info current_item;
	void **pointer = NULL;
	
	if (using_positions) {
		current_item = positional_items[fs->position - 1];
		pointer = current_item.item;
		return *((void**) pointer);
	}

	switch (fs->length) {
		case LENGTH_none:
			return va_arg(valist->valist, int*);
			break;
		case LENGTH_hh:
			return va_arg(valist->valist, signed char*);
			break;
		case LENGTH_h:
			return va_arg(valist->valist, short*);
			break;
		case LENGTH_l:
			return va_arg(valist->valist, long int*);
			break;
		case LENGTH_ll:
			return va_arg(valist->valist, long long int*);
			break;
		case LENGTH_j:
			return va_arg(valist->valist, intmax_t*);
			break;
		case LENGTH_z:
			return va_arg(valist->valist, size_t*);
			break;
		case LENGTH_t:
			return va_arg(valist->valist, ptrdiff_t*);
			break;
		default:
		    // Should never be called, means an error elsewhere in the code.
			break;
	}
	return NULL;
}



// Pops an integer off the valist and stores it in the positional_info.
// Parameters:
//     current_item - A positional_info item in the relevant place in the
//         positional_info_array.
//     valist - Struct holding a va_list for us to pop off of.
// Returns:
//     true on success, false on failure.
static bool pop_and_store_integer(positional_info *current_item, 
								  va_list_s *valist)
{
	int *p_int;
    long int *p_l_int;
    long long int *p_ll_int;
    intmax_t *p_intmax;
    size_t *p_size;
    ptrdiff_t *p_ptrdiff;
	
	switch (current_item->length) {
		case LENGTH_none:
			// PASS-THROUGH
		case LENGTH_hh:
			// PASS-THROUGH
		case LENGTH_h:
			p_int = malloc(sizeof(int));
			if (p_int == NULL) {
				return false;
			}
			*p_int = va_arg(valist->valist, int);
			current_item->item = (void*) p_int;
			break;
		case LENGTH_l:
			p_l_int = malloc(sizeof(long int));
			if (p_l_int == NULL) {
				return false;
			}
			*p_l_int = va_arg(valist->valist, long int);
			current_item->item = (void*) p_l_int;
			break;
		case LENGTH_ll:
			p_ll_int = malloc(sizeof(long long int));
			if (p_ll_int == NULL) {
				return false;
			}
			*p_ll_int = va_arg(valist->valist, long long int);
			current_item->item = (void*) p_ll_int;
			break;
		case LENGTH_j:
			p_intmax = malloc(sizeof(intmax_t));
			if (p_intmax == NULL) {
				return false;
			}
			*p_intmax = va_arg(valist->valist, intmax_t);
			current_item->item = (void*) p_intmax;
			break;
		case LENGTH_z:
			p_size = malloc(sizeof(size_t));
			if (p_size == NULL) {
				return false;
			}
			*p_size = va_arg(valist->valist, size_t);
			current_item->item = (void*) p_size;
			break;
		case LENGTH_t:
			p_ptrdiff = malloc(sizeof(ptrdiff_t));
			if (p_ptrdiff == NULL) {
				return false;
			}
			*p_ptrdiff = va_arg(valist->valist, ptrdiff_t);
			current_item->item = (void*) p_ptrdiff;
			break;
		default:
			// This should never be called.
			return false;
			break;
	}
	return true;
}



// Pops an unsigned integer off the valist and stores it in the positional_info.
// Parameters:
//     current_item - A positional_info item in the relevant place in the 
//         positional_info_array.
//     valist - Struct holding a va_list for us to pop off of.
// Returns:
//     true on success, false on failure.
static bool pop_and_store_unsigned_integer(positional_info *current_item, 
										   va_list_s *valist)
{
	size_t *p_size;
    ptrdiff_t *p_ptrdiff;
    unsigned int *p_uint;
    unsigned long int *p_l_uint;
    unsigned long long int *p_ll_uint; 
    uintmax_t *p_uintmax;
	
	// Determine the right type to get.
	switch (current_item->length) {
		case LENGTH_none:
			// PASS-THROUGH
		case LENGTH_hh:
			// PASS-THROUGH
		case LENGTH_h:
			p_uint = malloc(sizeof(unsigned int));
			if (p_uint == NULL) {
				return false;
			}
			*p_uint = va_arg(valist->valist, unsigned int);
			current_item->item = (void*) p_uint;
			break;
		case LENGTH_l:
			p_l_uint = malloc(sizeof(unsigned long int));
			if (p_l_uint == NULL) {
				return false;
			}
			*p_l_uint = va_arg(valist->valist, unsigned long int);
			current_item->item = (void*) p_l_uint;
			break;
		case LENGTH_ll:
			p_ll_uint = malloc(sizeof(unsigned long long int));
			if (p_ll_uint == NULL) {
				return false;
			}
			*p_ll_uint = va_arg(valist->valist, unsigned long long int);
			current_item->item = (void*) p_ll_uint;
			break;
		case LENGTH_j:
			p_uintmax = malloc(sizeof(uintmax_t));
			if (p_uintmax == NULL) {
				return false;
			}
			*p_uintmax = va_arg(valist->valist, uintmax_t);
			current_item->item = (void*) p_uintmax;
			break;
		case LENGTH_z:
			p_size = malloc(sizeof(size_t));
			if (p_size == NULL) {
				return false;
			}
			*p_size = va_arg(valist->valist, size_t);
			current_item->item = (void*) p_size;
			break;
		case LENGTH_t:
			p_ptrdiff = malloc(sizeof(ptrdiff_t));
			if (p_ptrdiff == NULL) {
				return false;
			}
			*p_ptrdiff = va_arg(valist->valist, ptrdiff_t);
			current_item->item = (void*) p_ptrdiff;
			break;
		default:
			// This should never be called.
			return false;
			break;
	}
	return true;
}



// Pops a floating point number off the valist and stores it in the 
// positional_info.
// Parameters:
//     current_item - A positional_info item in the relevant place in the
//         positional_info_array.
//     valist - Struct holding a va_list for us to pop off of.
// Returns:
//     true on success, false on failure.
static bool pop_and_store_floating_point(positional_info *current_item, 
										 va_list_s *valist)
{
	double *p_double;
	long double *p_l_double;
	
	switch (current_item->length) {
		case LENGTH_none:
			p_double = malloc(sizeof(double));
			if (p_double == NULL) {
				return false;
			}
			*p_double = va_arg(valist->valist, double);
			current_item->item = (void*) p_double;
			break;
		case LENGTH_L:
			p_l_double = malloc(sizeof(long double));
			if (p_l_double == NULL) {
				return false;
			}
			*p_l_double = va_arg(valist->valist, double);
			current_item->item = (void*) p_l_double;
			break;
		default:
			// This should never be called.
			return false;
			break;
	}
	return true;
}



// Pops a character off the valist and stores it in the positional_info.
// Parameters:
//     current_item - A positional_info item in the relevant place in
//         the positional_info_array.
//     valist - Struct holding a va_list for us to pop off of.
// Returns:
//     true on success, false on failure.
static bool pop_and_store_character(positional_info *current_item, 
									va_list_s *valist)
{
	int *p_int = malloc(sizeof(int));
	if (p_int == NULL) {
		return false;
	}
	*p_int = va_arg(valist->valist, int);
	current_item->item = (void*) p_int;
	return true;
}
				
				
				
// Pops a void pointer off the valist and stores it in the positional_info.
// Parameters:
//     current_item - A positional_info item in the relevant place in
//         the positional_info_array.
//     valist - Struct holding a va_list for us to pop off of.
// Returns:
//     true on success, false on failure.
static bool pop_and_store_pointer(positional_info *current_item, 
								  va_list_s *valist)
{
	void **pp_void = malloc(sizeof(void*));
	if (pp_void == NULL) {
		return false;
	}
	*pp_void = va_arg(valist->valist, void*);
	current_item->item = (void*) pp_void;
	return true;
}



// Pops a pointer to char off the valist and stores it in the positional_info.
// Parameters:
//     current_item - A positional_info item in the relevant place in
//         the positional_info_array.
//     valist - Struct holding a va_list for us to pop off of.
// Returns:
//     true on success, false on failure.
static bool pop_and_store_string(positional_info *current_item, 
								 va_list_s *valist)
{
	char **pp_char = malloc(sizeof(char*));
	if (pp_char == NULL) {
		return false;
	}
	*pp_char = va_arg(valist->valist, char*);
	current_item->item = (void*) pp_char;
	return true;
}



// Pops a pointer for "%n" off the valist and stores it in the positional_info.
// Parameters:
//     current_item - A positional_info item in the relevant place in
//         the positional_info_array.
//     valist - Struct holding a va_list for us to pop off of.
// Returns:
//     true on success, false on failure.
static bool pop_and_store_n_pointer(positional_info *current_item, 
									va_list_s *valist)
{
	signed char **pp_char;
	short **pp_short;
	int **pp_int;
    long int **pp_l_int;
    long long int **pp_ll_int;
    intmax_t **pp_intmax;
    size_t **pp_size;
    ptrdiff_t **pp_ptrdiff;
	
	
	switch (current_item->length) {
		case LENGTH_none:
			pp_int = malloc(sizeof(int*));
			if (pp_int == NULL) {
				return false;
			}
			*pp_int = va_arg(valist->valist, int*);
			current_item->item = (void*) pp_int;
			break;
		case LENGTH_hh:
			pp_char = malloc(sizeof(signed char*));
			if (pp_char == NULL) {
				return false;
			}
			*pp_char = va_arg(valist->valist, signed char*);
			current_item->item = (void*) pp_char;
			break;
		case LENGTH_h:
			pp_short = malloc(sizeof(short*));
			if (pp_short == NULL) {
				return false;
			}
			*pp_short = va_arg(valist->valist, short*);
			current_item->item = (void*) pp_short;
			break;
		case LENGTH_l:
			pp_l_int = malloc(sizeof(long int*));
			if (pp_l_int == NULL) {
				return false;
			}
			*pp_l_int = va_arg(valist->valist, long int*);
			current_item->item = (void*) pp_l_int;
			break;
		case LENGTH_ll:
			pp_ll_int = malloc(sizeof(long long int*));
			if (pp_ll_int == NULL) {
				return false;
			}
			*pp_ll_int = va_arg(valist->valist, long long int*);
			current_item->item = (void*) pp_ll_int;
			break;
		case LENGTH_j:
			pp_intmax = malloc(sizeof(intmax_t*));
			if (pp_intmax == NULL) {
				return false;
			}
			*pp_intmax = va_arg(valist->valist, intmax_t*);
			current_item->item = (void*) pp_intmax;
			break;
		case LENGTH_z:
			pp_size = malloc(sizeof(size_t*));
			if (pp_size == NULL) {
				return false;
			}
			*pp_size = va_arg(valist->valist, size_t*);
			current_item->item = (void*) pp_size;
			break;
		case LENGTH_t:
			pp_ptrdiff = malloc(sizeof(ptrdiff_t*));
			if (pp_ptrdiff == NULL) {
				return false;
			}
			*pp_ptrdiff = va_arg(valist->valist, ptrdiff_t*);
			current_item->item = (void*) pp_ptrdiff;
			break;
		default:
			// This should never be called.
			return false;
			break;
	}
	return true;
}



// Frees memory allocated via pop_and_store functions.
// Parameters:
//     items - A positional_info_array of parsed items.
//     count - The number of items parsed.
void pop_and_store_cleanup(positional_info_array *pia, int count)          
{
	for (int i = 0; i < count - 1; i++) {
		free((pia->array + i)->item);
	}
}



// Debug function - Prints out a positional_info_array.
// Parameters:
//     items - A positional_info_array of parsed items.
//     count - The number of items parsed.
void print_positional_info_stuff(const positional_info *items, int count)
{
	if (items == NULL) {
		printf("You sent NULL\n");
		return;
	}
	for (int i = 0; i < count; i++) {
		printf("Item %d:\n", i + 1);
		printf("Length: ");
		switch ((items + i)->length) {
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
		switch ((items + i)->type) {
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
		printf("Pointer: %p\n", (items + i)->item);
	}
}



// Checks the size of the positional_info_array, and makes it larger and 
// initialises the new parts if necessary.
// Parameters:
//     pia - pointer to the positional_info_array.
//     required_size - the funciton makes sure the size of the array is at
//         least this size.
// Returns:
//     pia - updates to new size if necessary.
//     return - true on success, false on failure.
static bool pia_check_size_and_update(positional_info_array *pia, 
									  int required_size)
{		
	int current_size = pia->size;
	positional_info *new_array = NULL;
	
	if (required_size > current_size) {
		// We need to resize.
		while (current_size < required_size) {
			// FIXME Check overflow.
			current_size *= 2;
		}
		
		new_array = realloc(pia->array, sizeof(positional_info) * current_size);
		if (new_array == NULL) {
			return false;
		}

		// Initialise new members.
		for (int i = pia->size; i < current_size; i++) {
			(new_array + i)->type = TYPE_ERROR;
			(new_array + i)->length = LENGTH_none;
			(new_array + i)->item = NULL;
		}
		// Update the pia.
		pia->array = new_array;
		pia->size = current_size;
	}
	
	return true;
}



// Creates and initialises a new positional_info_array.
// Parameters:
//     pia - Pointer to the positional_info_array.
// Returns:
//     true on success, false on failure.
static bool pia_initialise(positional_info_array *pia)
{		
	pia->array = malloc(sizeof(positional_info) * DEFAULT_PIA_SIZE);
	if (pia->array == NULL) {
		return false;
	}

	// Initialise members.
	for (int i = 0; i < DEFAULT_PIA_SIZE; i++) {
		(pia->array + i)->type = TYPE_ERROR;
		(pia->array + i)->length = LENGTH_none;
		(pia->array + i)->item = NULL;
	}
	// Update the pia.
	pia->size = DEFAULT_PIA_SIZE;
	
	return true;
}



// Parses the whole format string looking for format specifiers and places
// them in a list so we can pop off in positional order.
// Parameters:
//     format - The printf format string.
//     pia - The positional_info_array to store the result in.
//     max - Pointer to an int for the return of the number of 
//         positions.
// Returns:
//     max - The number of the highest position in max.
//     pia - Stores the result in here.
//     return - true on success, false on failure.
bool parse_format_string_for_positions(const char* format, 
									   positional_info_array *pia, int *max)
{
	positional_info *current_item;
	int max_found = 0;	    
    format_error error = FORMAT_okay;
	
	if (!pia_initialise(pia)) {
		return false;
	}
	
    // While format string is not end of line character.
    while (*format != '\0') {
		// Checking for "%%"
		if (*format == '%' && *(format + 1) == '%') {
			format += 2;
		} else if (*format == '%') {
            // Starting a format specifier.
            format++;
                       
            format_specifier fs;
            error = read_format_string(format, &fs);
            if (format_error_is_error(error)) {
				free(pia->array);
				return false;
			}
			
			// Make sure we are using positions.
            if (fs.position == 0) {
				free(pia->array);
				return false;
			}
            
            // Check preceding values. 
            if (fs.preceding_width != 0) {
				if (!pia_check_size_and_update(pia, fs.preceding_width)) {
					free(pia->array);
					return false;
				}
				current_item = pia->array + fs.preceding_width - 1;
				if (current_item->type != TYPE_ERROR) {
					// This item has been used before. Check that its length
					// and type match last time.
					if (current_item->type != TYPE_i ||
						current_item->length != LENGTH_none)
					{
						free(pia->array);
						return false;
					}
				}
				// It holds an int.
				current_item->type = TYPE_i;
				current_item->length = LENGTH_none;
				if (fs.preceding_width > max_found) {
					max_found = fs.preceding_width;
				}
            }
			
			if (fs.preceding_precision != 0) {
				if (!pia_check_size_and_update(pia, fs.preceding_precision)) {
					free(pia->array);
					return false;
				}
				current_item = pia->array + fs.preceding_width - 1;
				if (current_item->type != TYPE_ERROR) {
					// This item has been used before. Check that its length
					// and type match last time.
					if (current_item->type != TYPE_i ||
						current_item->length != LENGTH_none)
					{
						free(pia->array);
						return false;
					}
				}

				// It holds an int.
				current_item->type = TYPE_i;
				current_item->length = LENGTH_none;
				if (fs.preceding_precision > max_found) {
					max_found = fs.preceding_precision;
				}
			}
			
			// For printed type.
			if (!pia_check_size_and_update(pia, fs.position)) {
				free(pia->array);
				return false;
			}
			current_item = pia->array + fs.position - 1;
			if (current_item->type != TYPE_ERROR) {
				// This item has been used before. Check that its length
				// and type match last time.
				if (current_item->type != fs.type ||
					current_item->length != fs.length)
				{
					free(pia->array);
					return false;
				}
			}
			current_item->type = fs.type;
			current_item->length = fs.length;
			if (fs.position > max_found) {
				max_found = fs.position;
			}
        } else {
			format++;
        }
    }
	
	// Make sure that each element from 1 to max_found has actually
	// been given, otherwise it is an error.
	for (int i = 0; i < max_found; i++) {
		current_item = pia->array + i;
		if (current_item->type == TYPE_ERROR) {
			free(pia->array);
			return false;
		}
	}
	
	*max = max_found;
	return true;
	
}

