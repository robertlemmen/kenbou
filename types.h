#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdlib.h>
// XXX just for dump_value below, which needs to move to ports
#include <stdio.h>

/* Value Representation
 *
 * values are represented as 64-bit, using tagging in the low order bits. This is
 * targeting a 64bit pointer size, and the heap objects are aligned to 16bytes, so
 * the lower 4 bits are used for tagging. a value can either hold a "immediate" value,
 * or hold a pointer to a heap-allocated object. the lowest order bit is used to
 * distinguish between immmediate and non-immediate values.
 *   0 - immediate
 *   1 - non-immediate
 *
 * in the case of immediate values, the next 3 bits are used to determine the type
 * of the value: 
 *   000_ - enumerated
 *   001_ - integer
 *   010_ - float
 *   011_ - short symbol
 *   100_ - short string
 *   101_ - lookup vector
 *
 * in the case of floats and integers, the top 32 bits contain the value, in the 
 * case of an enumerated value, the next bits are used to specify the exact value:
 *   0000____ - Nil (internal, not usable in the scheme layer)
 *   0010____ - True
 *   0100____ - False
 *   0110____ - Empty List
 *   1000____ - End Of File
 *
 * 'Specials' are enums, but all have the lowest enum bit set to make testing for 
 * them easier:
 *   00001____ - 'If' Special
 *   00011____ - 'Define' Special
 *   00101____ - 'Lambda' Special
 *   00111____ - 'Begin' Special
 *   01001____ - 'Quote' Special
 *   01011____ - 'Let' Special
 *   01101____ - 'Let*' Special
 *   01111____ - 'Letrec' Special
 *   10001____ - 'Apply' Special
 *   10011____ - 'Set' Special
 *   10101____ - 'Eval' Special
 *   10111____ - 'Parameterize'
 *
 * this means that enumerated "special" values can be compared for equality 
 * directly, by comparing the value bitwise.
 *
 * in the case of a non-immediate value, the next three bits are used to store 
 * type information, and the remaining bits contain the pointer (when the lower 
 * three bits are masked off) of the heap cell. We have a set of non-immediate 
 * types:
 *   000_ - symbol
 *   001_ - string
 *   010_ - cons cell
 *   011_ - builtin (stuff callable from scheme but written in C, with arity)
 *   100_ - interpreter lambda
 *   101_ - vector
 *   110_ - 
 *   111_ - boxed / other
 *
 * in the case of "boxed" non-immediates, the sub-type is stored on the heap as
 * well, in the first 8 bits. currently supported:
 *   000 - environment
 *   001 - parameter
 *   010 - port
 *   011 - environment entry
 *   100 - interpreter dynamic frames
 *
 *  */

// XXX we have to make sure that we never have CONS entries that are empty,
// otherwise we need to support EMPTY_LIST == CONS.

struct allocator;
struct allocator_gc_ctx;

#if __SIZEOF_POINTER__ != 8
#error "This code targets 64-bit systems only"
#endif

// XXX this could be fixed easily, and then we could do either
#if __BYTE_ORDER != __LITTLE_ENDIAN
#error "This code targets little-endian systems only"
#endif

typedef uint64_t value;

#define value_is_immediate(x) (!((x) & 1))
#define value_to_cell(x) (void*)((x) & ~15)
#define value_type(x) ((x) & 15)
#define value_subtype(x) (*(uint8_t*)(value_to_cell(x)))

#define TYPE_ENUM             0b0000
#define TYPE_INT              0b0010
#define TYPE_FLOAT            0b0100
#define TYPE_SHORT_SYMBOL     0b0110
#define TYPE_SHORT_STRING     0b1000
#define TYPE_LOOKUP_VECTOR    0b1010

#define TYPE_SYMBOL           0b0001
#define TYPE_STRING           0b0011
#define TYPE_CONS             0b0101
#define TYPE_BUILTIN          0b0111
#define TYPE_INTERP_LAMBDA    0b1001
#define TYPE_VECTOR           0b1011
// unused                     0b1101
#define TYPE_BOXED            0b1111

#define SUBTYPE_ENV            0b000
#define SUBTYPE_PARAM          0b001
#define SUBTYPE_PORT           0b010
#define SUBTYPE_ENV_ENTRY      0b011
#define SUBTYPE_DYN_FRAME      0b100

#define VALUE_NIL         0b00000000
#define VALUE_TRUE        0b00100000
#define VALUE_FALSE       0b01000000
#define VALUE_EMPTY_LIST  0b01100000
#define VALUE_EOF         0b10000000

#define VALUE_SP_IF      0b000010000
#define VALUE_SP_DEFINE  0b000110000
#define VALUE_SP_LAMBDA  0b001010000
#define VALUE_SP_BEGIN   0b001110000
#define VALUE_SP_QUOTE   0b010010000
#define VALUE_SP_LET     0b010110000
#define VALUE_SP_LETS    0b011010000
#define VALUE_SP_LETREC  0b011110000
#define VALUE_SP_APPLY   0b100010000
#define VALUE_SP_SET     0b100110000
#define VALUE_SP_EVAL    0b101010000
#define VALUE_SP_PARAM   0b101110000

#define value_is_special(x) (((x) & 0b00011111) == 0b00010000)

// this is a little bit slower than native integers, but they could only be
// 32bits, and it's still a lot fasster than bignums. this gives us a range
// of [36028797018963968, -36028797018963968]
#define psignbit(x)         ((uint64_t)((x) < 0 ? 1 : 0) << 63)
#define gsignbit(x)         ((uint64_t)(((x) >> 63) == 0 ? 1 : -1))

#define intval(x)           (int64_t)((((x) & 0x7fffffffffffffff) >> 4) \
                                * gsignbit(x))
#define	make_int(a, x)      ((uint64_t)(labs(x) << 4) \
                                | psignbit(x) | TYPE_INT)


/* Float values, these are ony 32bit
 * */
#define value_is_float(x) (value_type(x) == TYPE_FLOAT)
value make_float(struct allocator *a, float x);
float floatval(value v);

/* Cons cells
 *
 * in our implementation, cons cells are simply two values. this means it is
 * the most simple heap cell imaginable.
 * */

struct cons {
    value car;
    value cdr;
};

value make_cons(struct allocator *a, value car, value cdr);

#define car(x) (((struct cons*)value_to_cell(x))->car)
#define cdr(x) (((struct cons*)value_to_cell(x))->cdr)

// return pointers to, this is required by GC to update for moved items
#define carptr(x) (&((struct cons*)value_to_cell(x))->car)
#define cdrptr(x) (&((struct cons*)value_to_cell(x))->cdr)

void set_car(struct allocator *a, value c, value n);
void set_cdr(struct allocator *a, value c, value n);

/* Symbols
 *
 * Symbols are represented on the heap as a null-terminated string
 * */
#define value_is_symbol(x) ((value_type(x) == TYPE_SHORT_SYMBOL) || (value_type(x) == TYPE_SYMBOL))

value make_symbol(struct allocator *a, char *s);
/* weirdly we need to pass this by address due to the short string optimization.
 * also means that the caller has to be very careful as the value returned from
 * this does not outlive the value passed in! */
char* value_to_symbol(value *s);

/* Strings
 *
 * Strings work just like symbols, so the same conditions apply
 * */
#define value_is_string(x) ((value_type(x) == TYPE_SHORT_STRING) || (value_type(x) == TYPE_STRING))

value make_string(struct allocator *a, char *s);
value make_stringn(struct allocator *a, char *s, int n);
/* weirdly we need to pass this by address due to the short string optimization.
 * also means that the caller has to be very careful as the value returned from
 * this does not outlive the value passed in! */
char* value_to_string(value *s);

/* Lookup Vectors
 *
 * Lookup Vectors are a dynamic optimization, instead of looking up symbols
 * every time they are evaluated, they are only evaluated once and then replaced
 * with a vector of two numbers: how many environments to move up, and how many
 * entries into that environment. */
value make_lookup_vector(struct allocator *a, uint16_t envs, uint16_t entries);
uint16_t lookup_vector_envs(value l);
uint16_t lookup_vector_entries(value l);

/* Builtins
 * */

#define BUILTIN_ARITY_VARIADIC -1
int builtin_arity(value);

char* builtin_name(value);

typedef value (*t_builtin0)(struct allocator*);
typedef value (*t_builtin1)(struct allocator*, value);
typedef value (*t_builtin2)(struct allocator*, value, value);
typedef value (*t_builtin3)(struct allocator*, value, value, value);
typedef value (*t_builtinv)(struct allocator*, value);

value make_builtin0(struct allocator *a, t_builtin0 funcptr, char *name);
t_builtin0 builtin0_ptr(value);

value make_builtin1(struct allocator *a, t_builtin1 funcptr, char *name);
t_builtin1 builtin1_ptr(value);

value make_builtin2(struct allocator *a, t_builtin2 funcptr, char *name);
t_builtin2 builtin2_ptr(value);

value make_builtin3(struct allocator *a, t_builtin3 funcptr, char *name);
t_builtin3 builtin3_ptr(value);

value make_builtinv(struct allocator *a, t_builtinv funcptr, char *name);
t_builtinv builtinv_ptr(value);

/* Lambdas
 * */
#define make_interp_lambda(x) ((uint64_t)(x) | TYPE_INTERP_LAMBDA)
#define value_to_interp_lambda(x) ((struct interp_lambda*)value_to_cell(x))

/* Vectors
 * */
value make_vector(struct allocator *a, int length, value fill);
int vector_length(value v);
value vector_ref(value v, int pos);
void vector_set(value v, int pos, value i);
void traverse_vector(struct allocator_gc_ctx *gc, value v);

/* Input and Output
 *
 * these routines allow reading and printing of values. They are aimed
 * at development, debugging and bootstrapping, at program runtime equivalent
 * functions from the scheme layer should be used
 * */
// XXX whether these should be in here or in ports is questionable...
void dump_value(value v, FILE *f);
void dump_string_value(value v, FILE *f);

/* Environment references
 * */
#define value_is_env(x) (   (value_type(x) == TYPE_BOXED) \
                         && (value_subtype(x) == SUBTYPE_ENV))

struct interp_env;
value make_environment(struct allocator *a, struct interp_env *env);
struct interp_env* value_to_environment(value v);

#define value_is_env_entry(x) (   (value_type(x) == TYPE_BOXED) \
                               && (value_subtype(x) == SUBTYPE_ENV_ENTRY))

struct interp_env_entry;
value make_env_entry(struct allocator *a, struct interp_env_entry *entry);
struct interp_env_entry* value_to_env_entry(value v);

/* Interpreter Dynamic Frames
 * */

#define value_is_dyn_frame(x) (   (value_type(x) == TYPE_BOXED) \
                               && (value_subtype(x) == SUBTYPE_DYN_FRAME))

struct dynamic_frame;
value make_dyn_frame(struct allocator *a, struct dynamic_frame *df);
struct dynamic_frame* value_to_dyn_frame(value v);

/* Dynamic Bindings
 * */
struct param {
    value init;
    value convert;
};

#define value_is_param(x) ((value_type(x) == TYPE_BOXED) && (value_subtype(x) == SUBTYPE_PARAM))

value make_parameter(struct allocator *a, value init, value convert);
struct param* value_to_parameter(value v);

#endif /* TYPES_H */
