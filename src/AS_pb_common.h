/* AS_pb_common.h: Common support functions for AS_pb_encode.c and AS_pb_decode.c.
 * These functions are rarely needed by applications directly.
 */

#ifndef AS_PB_COMMON_H_INCLUDED
#define AS_PB_COMMON_H_INCLUDED

#include "AS_pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the field iterator structure to beginning.
 * Returns false if the message type is empty. */
bool AS_pb_field_iter_begin(AS_pb_field_iter_t *iter, const AS_pb_msgdesc_t *desc, void *message);

/* Get a field iterator for extension field. */
bool AS_pb_field_iter_begin_extension(AS_pb_field_iter_t *iter, AS_pb_extension_t *extension);

/* Same as AS_pb_field_iter_begin(), but for const message pointer.
 * Note that the pointers in AS_pb_field_iter_t will be non-const but shouldn't
 * be written to when using these functions. */
bool AS_pb_field_iter_begin_const(AS_pb_field_iter_t *iter, const AS_pb_msgdesc_t *desc, const void *message);
bool AS_pb_field_iter_begin_extension_const(AS_pb_field_iter_t *iter, const AS_pb_extension_t *extension);

/* Advance the iterator to the next field.
 * Returns false when the iterator wraps back to the first field. */
bool AS_pb_field_iter_next(AS_pb_field_iter_t *iter);

/* Advance the iterator until it points at a field with the given tag.
 * Returns false if no such field exists. */
bool AS_pb_field_iter_find(AS_pb_field_iter_t *iter, uint32_t tag);

/* Find a field with type AS_PB_LTYPE_EXTENSION, or return false if not found.
 * There can be only one extension range field per message. */
bool AS_pb_field_iter_find_extension(AS_pb_field_iter_t *iter);

#ifdef AS_PB_VALIDATE_UTF8
/* Validate UTF-8 text string */
bool AS_pb_validate_utf8(const char *s);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

