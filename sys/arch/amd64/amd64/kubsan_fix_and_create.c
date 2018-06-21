#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/sched.h>
#include <bitops.h>
/* Non-Existent:
#include <linux/bug.h>
#include <linux/ctype.h>
#include <linux/init.h>
*/


/* Description -- TODO */

/* Might have to move this around -- TODO*/
#include <sys/ubsan.h>

/* TODO: Gotta Understand this first to implement
const char *type_check_kinds[] = {
	"load of",
	"store to",
	"reference binding to",
	"member access within",
	"member call on",
	"constructor call on",
	"downcast of",
	"downcast of"
};
*/

/* WEIRD LOW LEVEL STUFF */
#define REPORTED_BIT 31

#if (BITS_PER_LONG == 64) && defined(__BIG_ENDIAN)
#define COLUMN_MASK (~(1U << REPORTED_BIT))
#define LINE_MASK   (~0U)
#else
#define COLUMN_MASK   (~0U)
#define LINE_MASK (~(1U << REPORTED_BIT))
#endif

#define VALUE_LENGTH 40


static bool was_reported(struct source_location *location)
{
	return test_and_set_bit(REPORTED_BIT, &location->reported);
}


/* Introductory messages and clock definitions
static DEFINE_SPINLOCK(report_lock);

static void kubsan_open(struct source_location *location,
			unsigned long *flags)
{
	current->in_ubsan++;
	spin_lock_irqsave(&report_lock, *flags);

	pr_err("========================================"
		"========================================\n");
	print_source_location("KUBSAN: Undefined behaviour in", location);
}

static void kubsan_close(unsigned long *flags)
{
	dump_stack();
	pr_err("========================================"
		"========================================\n");
	spin_unlock_irqrestore(&report_lock, *flags);
	current->in_ubsan--;
}
*/


/* Function handles --to be filled-- */

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
