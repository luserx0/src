#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

/*
 * Export symbols for UBSan
 * !Currently dummy functions!
 */

#include <sys/ubsan.h>


void __ubsan_handle_add_overflow(struct overflow_data *, unsigned long, unsigned long);
void __ubsan_handle_add_overflow(struct overflow_data *data,
				unsigned long lhs,
				unsigned long rhs)
{}

void __ubsan_handle_sub_overflow(struct overflow_data *, unsigned long, unsigned long);
void __ubsan_handle_sub_overflow(struct overflow_data *data,
				unsigned long lhs,
				unsigned long rhs)
{}

void __ubsan_handle_mul_overflow(struct overflow_data *, unsigned long, unsigned long);
void __ubsan_handle_mul_overflow(struct overflow_data *data,
				unsigned long lhs,
				unsigned long rhs)
{}

void __ubsan_handle_negate_overflow(struct overflow_data *, unsigned long);
void __ubsan_handle_negate_overflow(struct overflow_data *data,
				unsigned long old_val)
{}

void __ubsan_handle_divrem_overflow(struct overflow_data *, unsigned long, unsigned long);
void __ubsan_handle_divrem_overflow(struct overflow_data *data,
				unsigned long lhs,
				unsigned long rhs)
{}

void __ubsan_handle_type_mismatch(struct type_mismatch_data *, unsigned long);
void __ubsan_handle_type_mismatch(struct type_mismatch_data *data,
				unsigned long ptr)
{}

void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data_v1 *, unsigned long);
void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data_v1 *data,
				unsigned long ptr)
{}

void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *, unsigned long);
void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *data,
					unsigned long bound)
{}

void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *, unsigned long);
void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *data,
				unsigned long index)
{}

void __ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *, unsigned long, unsigned long);
void __ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *data,
					unsigned long lhs, unsigned long rhs)
{}

/*
void __noreturn
__ubsan_handle_builtin_unreachable(struct unreachable_data *data);
void __noreturn
__ubsan_handle_builtin_unreachable(struct unreachable_data *data)
{}
*/

void __ubsan_handle_load_invalid_value(struct invalid_value_data *, unsigned long);
void __ubsan_handle_load_invalid_value(struct invalid_value_data *data,
				unsigned long val)
{}

void __ubsan_handle_nonnull_return(void *data);
void __ubsan_handle_nonnull_return(void *data) {}

/* Compiler moaning */
void __ubsan_handle_nonnull_arg(void *data);
void __ubsan_handle_nonnull_arg(void *data) {}
