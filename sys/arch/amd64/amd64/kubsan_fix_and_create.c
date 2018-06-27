#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/kernel.h>
#include <sys/param.h> //Kamil indication
#include <sys/types.h>
#include <sys/sched.h>
#include <sys/bitops.h>
#include <sys/atomic.h> //For new test and set bits
#include <sys/systm.h>  // aprint and many more

//Check if these are useful
#include <sys/proc.h>
#include <sys/sched.h>
#include <sys/systm.h>

/* Non-Existent:
#include <linux/bug.h>
#include <linux/ctype.h>
#include <linux/init.h>
*/


/* Description -- TODO */

/* Might have to move this around -- TODO*/
#include <sys/ubsan.h>

/* TODO: Gotta understand this first to implement
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

/* WEIRD LOW LEVEL STUFF
 * TODO: Understand their functionality*/
#define REPORTED_BIT 31

#if (BITS_PER_LONG == 64) && defined(__BIG_ENDIAN)
#define COLUMN_MASK (~(1U << REPORTED_BIT))
#define LINE_MASK   (~0U)
#else
#define COLUMN_MASK   (~0U)
#define LINE_MASK (~(1U << REPORTED_BIT))
#endif

#define VALUE_LENGTH 40


/* TODO:
 * - pr_err --> aprint_error() || aprint_debug() && int aprint_get_error_count(void);
 * - current --> sched.h --> TROUBLE --> need help
 * - spin_lock_irqsave
 * - static DEFINE_SPINLOCK(report_lock);
 * - report_lock
 * - dump_stack
 * - scnprintf
*/

/* TODO: Refactor */
static inline unsigned long
test_and_set_bit(unsigned int bit, volatile unsigned long *ptr)
{
	const unsigned int units = (sizeof(*ptr) * CHAR_BIT);
	volatile unsigned long *const p = &ptr[bit / units];
	const unsigned long mask = (1UL << (bit % units));
	unsigned long v;

	do v = *p; while (atomic_cas_ulong(p, v, (v | mask)) != v);

	return ((v & mask) != 0);
}

static bool was_reported(struct source_location *location)
{
	return test_and_set_bit(REPORTED_BIT, &location->reported);
}

static void print_source_location(const char *prefix,
				struct source_location *loc)
{
	aprint_error("%s %s:%d:%d\n", prefix, loc->file_name,
		loc->line & LINE_MASK, loc->column & COLUMN_MASK);
}

/* TOUBLE
static bool suppress_report(struct source_location *loc)
{
	return current->in_ubsan || was_reported(loc);
}
*/
static bool type_is_int(struct type_descriptor *type)
{
	return type->type_kind == type_kind_int;
}

static bool type_is_signed(struct type_descriptor *type)
{
	KASSERT(!type_is_int(type));
	return  type->type_info & 1;
}


static unsigned type_bit_width(struct type_descriptor *type)
{
	return 1 << (type->type_info >> 1);
}


static bool is_inline_int(struct type_descriptor *type)
{
	unsigned inline_bits = sizeof(unsigned long)*8;
	unsigned bits = type_bit_width(type);

	KASSERT(!type_is_int(type));

	return bits <= inline_bits;
}

static s_max get_signed_val(struct type_descriptor *type, unsigned long val)
{
	if (is_inline_int(type)) {
		unsigned extra_bits = sizeof(s_max)*8 - type_bit_width(type);
		return ((s_max)val) << extra_bits >> extra_bits;
	}

	if (type_bit_width(type) == 64)
		return *(int64_t *)val;

	return *(s_max *)val;
}

static bool val_is_negative(struct type_descriptor *type, unsigned long val)
{
	return type_is_signed(type) && get_signed_val(type, val) < 0;
}

static u_max get_unsigned_val(struct type_descriptor *type, unsigned long val)
{
	if (is_inline_int(type))
		return val;

	if (type_bit_width(type) == 64)
		return *(uint64_t *)val;

	return *(u_max *)val;
}

static void val_to_string(char *str, size_t size, struct type_descriptor *type,
	unsigned long value)
{
	if (type_is_int(type)) {
		if (type_bit_width(type) == 128) {
/*#if defined(CONFIG_ARCH_SUPPORTS_INT128) && defined(__SIZEOF_INT128__)
			u_max val = get_unsigned_val(type, value);

			scnprintf(str, size, "0x%08x%08x%08x%08x",
				(u32)(val >> 96),
				(u32)(val >> 64),
				(u32)(val >> 32),
				(u32)(val));
#else
			WARN_ON(1);
#endif
*/
		} else if (type_is_signed(type)) {
			scnprintf(str, size, "%lld",
				(s64)get_signed_val(type, value));
		} else {
			scnprintf(str, size, "%llu",
				(u64)get_unsigned_val(type, value));
		}
	}
}


/* Introductory messages and clock definitions */
static DEFINE_SPINLOCK(report_lock);

static void kubsan_open(struct source_location *location,
			unsigned long *flags)
{
	current->in_ubsan++;
	spin_lock_irqsave(&report_lock, *flags);

	aprint_error("========================================"
		"========================================\n");
	print_source_location("KUBSan: Undefined behaviour in", location);
}

static void kubsan_close(unsigned long *flags)
{
	//dump_stack();
	aprint_error("========================================"
		"========================================\n");
	spin_unlock_irqrestore(&report_lock, *flags);
	current->in_ubsan--;
}


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
