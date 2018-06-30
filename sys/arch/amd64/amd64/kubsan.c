#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/sched.h>
#include <sys/bitops.h>
#include <sys/atomic.h>
#include <sys/systm.h>
#include <sys/endian.h>

//Check if these are useful
#include <sys/proc.h>
#include <sys/lwp.h>

#include <sys/kubsan.h>

/* These things will get ya killd*/
unsigned int in_ubsan;
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0) //From <linux/kernel.h> line 61


/* Description -- TODO */

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


/* WEIRD LOW LEVEL STUFF */
#define REPORTED_BIT 31

#if defined(_LP64) && BYTE_ORDER == BIG_ENDIAN
#define COLUMN_MASK (~(1U << REPORTED_BIT))
#define LINE_MASK   (~0U)
#else
#define COLUMN_MASK   (~0U)
#define LINE_MASK (~(1U << REPORTED_BIT))
#endif

#define VALUE_LENGTH 40


/* TODO:
 * Refactor test_and_set_bit; Check whether test_test_and_set || compare_and_swap would be better.
 * Add description
 * Understand type_check_kinds, maybe re-implement
 * Understand the low level bit definitions
 * Move kubsan.c to sys/sys/kern/kubsan.c -- CHECK distrib files for the transition
 * Refactor val_tostring() to return char *
*/

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

static void print_source_location(char *buf, size_t size,
				struct source_location *loc)
{
	snprintf(buf, size,
		"%s:%d:%d\n", loc->file_name, loc->line & LINE_MASK, loc->column & COLUMN_MASK);
}

static bool suppress_report(struct source_location *loc)
{
	return /*curlwp->in_ubsan ||*/was_reported(loc);
}

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
			panic("KUBSan: Unexpected type width: 128 bit\n");
		} else if (type_is_signed(type)) {
			snprintf(str, size, "%ld",
				(int64_t)get_signed_val(type, value));

		} else {
			snprintf(str, size, "%lu",
				(uint64_t)get_unsigned_val(type, value));
		}
		/* NOTE: Our s_max and u_max are ld not lld, snprintf might have problems */
	}
}



/* Introductory messages
 * Excluded for now. Will be implemented later with mutex locking
static DEFINE_SPINLOCK(report_lock);

static void kubsan_open(struct source_location *location,
			unsigned long *flags)
{
	curlwp->in_ubsan++;
	//spin_lock_irqsave(&report_lock, *flags);

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
	curlwp->in_ubsan--;
}
*/
static void handle_overflow(struct overflow_data *data, unsigned long lhs,
			unsigned long rhs, char op)
{

	struct type_descriptor *type = data->type;
	char lhs_val_str[VALUE_LENGTH];
	char rhs_val_str[VALUE_LENGTH];
	char src_location_buffer[VALUE_LENGTH];

	if (suppress_report(&data->location))
		return;

	val_to_string(lhs_val_str, sizeof(lhs_val_str), type, lhs);
	val_to_string(rhs_val_str, sizeof(rhs_val_str), type, rhs);

	print_source_location(src_location_buffer, sizeof(src_location_buffer), &data->location);
	panic("KUBSan: Undefined Behavior in: %s, %s integer overflow:\n%s %c %s cannot be represented in type %s\n",
		src_location_buffer,
		type_is_signed(type) ? "signed" : "unsigned",
		lhs_val_str,
		op,
		rhs_val_str,
		type->type_name);
}


/* Function handles */

void __ubsan_handle_add_overflow(struct overflow_data *, unsigned long, unsigned long);
void __ubsan_handle_add_overflow(struct overflow_data *data,
				unsigned long lhs,
				unsigned long rhs)
{
	handle_overflow(data, lhs, rhs, '+');
}

void __ubsan_handle_sub_overflow(struct overflow_data *, unsigned long, unsigned long);
void __ubsan_handle_sub_overflow(struct overflow_data *data,
				unsigned long lhs,
				unsigned long rhs)
{
	handle_overflow(data, lhs, rhs, '-');

}

void __ubsan_handle_mul_overflow(struct overflow_data *, unsigned long, unsigned long);
void __ubsan_handle_mul_overflow(struct overflow_data *data,
				unsigned long lhs,
				unsigned long rhs)
{
	handle_overflow(data, lhs, rhs, '*');
}

void __ubsan_handle_negate_overflow(struct overflow_data *, unsigned long);
void __ubsan_handle_negate_overflow(struct overflow_data *data,
				unsigned long old_val)
{
	char old_val_str[VALUE_LENGTH];
	char src_location_buffer[VALUE_LENGTH];

	if (suppress_report(&data->location))
		return;

	print_source_location(src_location_buffer, sizeof(src_location_buffer), &data->location);
	val_to_string(old_val_str, sizeof(old_val_str), data->type, old_val);

	panic("KUBSan: Undefined behavior in %s, negation of %s cannot be represented in type %s\n",
		src_location_buffer,
		old_val_str,
		data->type->type_name);
}

void __ubsan_handle_divrem_overflow(struct overflow_data *, unsigned long, unsigned long);
void __ubsan_handle_divrem_overflow(struct overflow_data *data,
				unsigned long lhs,
				unsigned long rhs)
{
	char rhs_val_str[VALUE_LENGTH];
	char src_location_buffer[VALUE_LENGTH];

	if (suppress_report(&data->location))
		return;

	print_source_location(src_location_buffer, sizeof(src_location_buffer), &data->location);
	val_to_string(rhs_val_str, sizeof(rhs_val_str), data->type, rhs);

	if ( type_is_signed(data->type) && get_signed_val(data->type, rhs) == -1)
	{
		panic("KUBSan: Undefined behavior in %s, division of %s by -1 cannot be represented in type %s\n", src_location_buffer, rhs_val_str, data->type->type_name);
	}
	else
	{
		panic("KUBSan: Undefined behavior in %s, division by zero\n", src_location_buffer);
	}
}

static void handle_null_ptr_deref(struct type_mismatch_data_common *data)
{
	char src_location_buffer[VALUE_LENGTH];

	if (suppress_report(data->location))
		return;

	print_source_location(src_location_buffer, sizeof(src_location_buffer), data->location);

	panic("KUBSan: Undefined behavior in %s, %s null pointer of type %s\n",
		src_location_buffer,
		type_check_kinds[data->type_check_kind],
		data->type->type_name);
}

static void handle_misaligned_access(struct type_mismatch_data_common *data,
				unsigned long ptr)
{
	char src_location_buffer[VALUE_LENGTH];
	if (suppress_report(data->location))
		return;

	print_source_location(src_location_buffer, sizeof(src_location_buffer), data->location);

	panic("KUBSan: Undefined behavior in %s, %s misaligned address %p for type %s\nwhich requires %ld byte alignment\n",
		src_location_buffer,
		type_check_kinds[data->type_check_kind],
		(void *)ptr,
		data->type->type_name,
		data->alignment);
}

static void handle_object_size_mismatch(struct type_mismatch_data_common *data,
					unsigned long ptr)
{
	char src_location_buffer[VALUE_LENGTH];
	if (suppress_report(data->location))
		return;

	print_source_location(src_location_buffer, sizeof(src_location_buffer), data->location);

	panic("KUBSan: Undefined behavior in %s, %s address %p with insufficient space\nfor an object of type %s\n",
		src_location_buffer,
		type_check_kinds[data->type_check_kind],
		(void *) ptr,
		data->type->type_name);
}

static void ubsan_type_mismatch_common(struct type_mismatch_data_common *data,
				unsigned long ptr)
{

	if (!ptr)
		handle_null_ptr_deref(data);
	else if (data->alignment && !IS_ALIGNED(ptr, data->alignment))
		handle_misaligned_access(data, ptr);
	else
		handle_object_size_mismatch(data, ptr);
}

void __ubsan_handle_type_mismatch(struct type_mismatch_data *, unsigned long);
void __ubsan_handle_type_mismatch(struct type_mismatch_data *data,
				unsigned long ptr)
{
	struct type_mismatch_data_common common_data = {
		.location = &data->location,
		.type = data->type,
		.alignment = data->alignment,
		.type_check_kind = data->type_check_kind
	};

	ubsan_type_mismatch_common(&common_data, ptr);
}

void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data_v1 *, unsigned long);
void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data_v1 *data,
				unsigned long ptr)
{
	struct type_mismatch_data_common common_data = {
		.location = &data->location,
		.type = data->type,
		.alignment = 1UL << data->log_alignment,
		.type_check_kind = data->type_check_kind
	};

	ubsan_type_mismatch_common(&common_data, ptr);
}

void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *, unsigned long);
void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *data,
					unsigned long bound)
{
	char bound_str[VALUE_LENGTH];
	char src_location_buffer[VALUE_LENGTH];

	if (suppress_report(&data->location))
		return;

	val_to_string(bound_str, sizeof(bound_str), data->type, bound);
	print_source_location(src_location_buffer, sizeof(src_location_buffer), &data->location),

	panic("KUBSan: Undefined behavior in %s variable length array bound value %s <= 0\n",
			src_location_buffer,
			bound_str);
}

void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *, unsigned long);
void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *data,
				unsigned long index)
{
	char index_str[VALUE_LENGTH];
	char src_location_buffer[VALUE_LENGTH];

	if (suppress_report(&data->location))
		return;

	print_source_location(src_location_buffer, sizeof(src_location_buffer), &data->location);
	val_to_string(index_str, sizeof(index_str), data->index_type, index);

	panic("KUBSan: Undefined behavior in %s, index %s is out of range for type %s\n",
		src_location_buffer,
		index_str,
		data->array_type->type_name);


}

void __ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *, unsigned long, unsigned long);
void __ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *data,
					unsigned long lhs, unsigned long rhs)
{
	struct type_descriptor *rhs_type = data->rhs_type;
	struct type_descriptor *lhs_type = data->lhs_type;
	char rhs_str[VALUE_LENGTH];
	char lhs_str[VALUE_LENGTH];
	char src_location_buffer[VALUE_LENGTH];

	if (suppress_report(&data->location))
		return;

	print_source_location(src_location_buffer, sizeof(src_location_buffer), &data->location);

	val_to_string(rhs_str, sizeof(rhs_str), rhs_type, rhs);
	val_to_string(lhs_str, sizeof(lhs_str), lhs_type, lhs);

	/* There has to be a better way; this is sloppy */
	if (val_is_negative(rhs_type, rhs))
		panic("KUBSan: Undefined behavior in %s, shift exponent %s is negative\n",
				src_location_buffer,
				rhs_str);

	else if (get_unsigned_val(rhs_type, rhs) >= type_bit_width(lhs_type))
		panic("KUBSan: Undefined behavior in %s, shift exponent %s is too large for %u-bit type %s\n",
			src_location_buffer,
			rhs_str,
			type_bit_width(lhs_type),
			lhs_type->type_name);

	else if (val_is_negative(lhs_type, lhs))
		panic("KUBSan: Undefined behavior in %s, left shift of negative value %s\n",
			src_location_buffer,
			lhs_str);
	else
		panic("KUBSan: Undefined behavior in %s, left shift of %s by %s places cannot be represented in type %s\n",
			src_location_buffer,
			lhs_str,
			rhs_str,
			lhs_type->type_name);

}

/* This is unimplemented as unused
void __noreturn
__ubsan_handle_builtin_unreachable(struct unreachable_data *data);
void __noreturn
__ubsan_handle_builtin_unreachable(struct unreachable_data *data)
{}
*/

void __ubsan_handle_load_invalid_value(struct invalid_value_data *, unsigned long);
void __ubsan_handle_load_invalid_value(struct invalid_value_data *data,
				unsigned long val)
{
	char val_str[VALUE_LENGTH];
	char src_location_buffer[VALUE_LENGTH];

	if (suppress_report(&data->location))
		return;

	print_source_location(src_location_buffer, sizeof(src_location_buffer), &data->location);
	val_to_string(val_str, sizeof(val_str), data->type, val);

	panic("KUBSan: Undefined behavior in %s, load of value %s is not a valid value for type %s\n",
			src_location_buffer,
			val_str,
			data->type->type_name);


}

void __ubsan_handle_nonnull_return(void *data);
void __ubsan_handle_nonnull_return(void *data)
{
/* Unimplemented; Kernel does not use it */
}

/* Compiler moaning */
void __ubsan_handle_nonnull_arg(void *data);
void __ubsan_handle_nonnull_arg(void *data)
{
/* Unimplemented; Kernel doesn't use it */
}
