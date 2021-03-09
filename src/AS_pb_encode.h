/* AS_pb_encode.h: Functions to encode protocol buffers. Depends on AS_pb_encode.c.
 * The main function is AS_pb_encode. You also need an output stream, and the
 * field descriptions created by nanoAS_pb_generator.py.
 */

#ifndef AS_PB_ENCODE_H_INCLUDED
#define AS_PB_ENCODE_H_INCLUDED

#include "AS_pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Structure for defining custom output streams. You will need to provide
 * a callback function to write the bytes to your storage, which can be
 * for example a file or a network socket.
 *
 * The callback must conform to these rules:
 *
 * 1) Return false on IO errors. This will cause encoding to abort.
 * 2) You can use state to store your own data (e.g. buffer pointer).
 * 3) AS_pb_write will update bytes_written after your callback runs.
 * 4) Substreams will modify max_size and bytes_written. Don't use them
 *    to calculate any pointers.
 */
struct AS_pb_ostream_s
{
#ifdef AS_PB_BUFFER_ONLY
    /* Callback pointer is not used in buffer-only configuration.
     * Having an int pointer here allows binary compatibility but
     * gives an error if someone tries to assign callback function.
     * Also, NULL pointer marks a 'sizing stream' that does not
     * write anything.
     */
    int *callback;
#else
    bool (*callback)(AS_pb_ostream_t *stream, const AS_pb_byte_t *buf, size_t count);
#endif
    void *state;          /* Free field for use by callback implementation. */
    size_t max_size;      /* Limit number of output bytes written (or use SIZE_MAX). */
    size_t bytes_written; /* Number of bytes written so far. */
    
#ifndef AS_PB_NO_ERRMSG
    const char *errmsg;
#endif
};

/***************************
 * Main encoding functions *
 ***************************/

/* Encode a single protocol buffers message from C structure into a stream.
 * Returns true on success, false on any failure.
 * The actual struct pointed to by src_struct must match the description in fields.
 * All required fields in the struct are assumed to have been filled in.
 *
 * Example usage:
 *    MyMessage msg = {};
 *    uint8_t buffer[64];
 *    AS_pb_ostream_t stream;
 *
 *    msg.field1 = 42;
 *    stream = AS_pb_ostream_from_buffer(buffer, sizeof(buffer));
 *    AS_pb_encode(&stream, MyMessage_fields, &msg);
 */
bool AS_pb_encode(AS_pb_ostream_t *stream, const AS_pb_msgdesc_t *fields, const void *src_struct);

/* Extended version of AS_pb_encode, with several options to control the
 * encoding process:
 *
 * AS_PB_ENCODE_DELIMITED:      Prepend the length of message as a varint.
 *                           Corresponds to writeDelimitedTo() in Google's
 *                           protobuf API.
 *
 * AS_PB_ENCODE_NULLTERMINATED: Append a null byte to the message for termination.
 *                           NOTE: This behaviour is not supported in most other
 *                           protobuf implementations, so AS_PB_ENCODE_DELIMITED
 *                           is a better option for compatibility.
 */
#define AS_PB_ENCODE_DELIMITED       0x02U
#define AS_PB_ENCODE_NULLTERMINATED  0x04U
bool AS_pb_encode_ex(AS_pb_ostream_t *stream, const AS_pb_msgdesc_t *fields, const void *src_struct, unsigned int flags);

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define AS_pb_encode_delimited(s,f,d) AS_pb_encode_ex(s,f,d, AS_PB_ENCODE_DELIMITED)
#define AS_pb_encode_nullterminated(s,f,d) AS_pb_encode_ex(s,f,d, AS_PB_ENCODE_NULLTERMINATED)

/* Encode the message to get the size of the encoded data, but do not store
 * the data. */
bool AS_pb_get_encoded_size(size_t *size, const AS_pb_msgdesc_t *fields, const void *src_struct);

/**************************************
 * Functions for manipulating streams *
 **************************************/

/* Create an output stream for writing into a memory buffer.
 * The number of bytes written can be found in stream.bytes_written after
 * encoding the message.
 *
 * Alternatively, you can use a custom stream that writes directly to e.g.
 * a file or a network socket.
 */
AS_pb_ostream_t AS_pb_ostream_from_buffer(AS_pb_byte_t *buf, size_t bufsize);

/* Pseudo-stream for measuring the size of a message without actually storing
 * the encoded data.
 * 
 * Example usage:
 *    MyMessage msg = {};
 *    AS_pb_ostream_t stream = AS_PB_OSTREAM_SIZING;
 *    AS_pb_encode(&stream, MyMessage_fields, &msg);
 *    printf("Message size is %d\n", stream.bytes_written);
 */
#ifndef AS_PB_NO_ERRMSG
#define AS_PB_OSTREAM_SIZING {0,0,0,0,0}
#else
#define AS_PB_OSTREAM_SIZING {0,0,0,0}
#endif

/* Function to write into a AS_pb_ostream_t stream. You can use this if you need
 * to append or prepend some custom headers to the message.
 */
bool AS_pb_write(AS_pb_ostream_t *stream, const AS_pb_byte_t *buf, size_t count);


/************************************************
 * Helper functions for writing field callbacks *
 ************************************************/

/* Encode field header based on type and field number defined in the field
 * structure. Call this from the callback before writing out field contents. */
bool AS_pb_encode_tag_for_field(AS_pb_ostream_t *stream, const AS_pb_field_iter_t *field);

/* Encode field header by manually specifying wire type. You need to use this
 * if you want to write out packed arrays from a callback field. */
bool AS_pb_encode_tag(AS_pb_ostream_t *stream, AS_pb_wire_type_t wiretype, uint32_t field_number);

/* Encode an integer in the varint format.
 * This works for bool, enum, int32, int64, uint32 and uint64 field types. */
#ifndef AS_PB_WITHOUT_64BIT
bool AS_pb_encode_varint(AS_pb_ostream_t *stream, uint64_t value);
#else
bool AS_pb_encode_varint(AS_pb_ostream_t *stream, uint32_t value);
#endif

/* Encode an integer in the zig-zagged svarint format.
 * This works for sint32 and sint64. */
#ifndef AS_PB_WITHOUT_64BIT
bool AS_pb_encode_svarint(AS_pb_ostream_t *stream, int64_t value);
#else
bool AS_pb_encode_svarint(AS_pb_ostream_t *stream, int32_t value);
#endif

/* Encode a string or bytes type field. For strings, pass strlen(s) as size. */
bool AS_pb_encode_string(AS_pb_ostream_t *stream, const AS_pb_byte_t *buffer, size_t size);

/* Encode a fixed32, sfixed32 or float value.
 * You need to pass a pointer to a 4-byte wide C variable. */
bool AS_pb_encode_fixed32(AS_pb_ostream_t *stream, const void *value);

#ifndef AS_PB_WITHOUT_64BIT
/* Encode a fixed64, sfixed64 or double value.
 * You need to pass a pointer to a 8-byte wide C variable. */
bool AS_pb_encode_fixed64(AS_pb_ostream_t *stream, const void *value);
#endif

#ifdef AS_PB_CONVERT_DOUBLE_FLOAT
/* Encode a float value so that it appears like a double in the encoded
 * message. */
bool AS_pb_encode_float_as_double(AS_pb_ostream_t *stream, float value);
#endif

/* Encode a submessage field.
 * You need to pass the AS_pb_field_t array and pointer to struct, just like
 * with AS_pb_encode(). This internally encodes the submessage twice, first to
 * calculate message size and then to actually write it out.
 */
bool AS_pb_encode_submessage(AS_pb_ostream_t *stream, const AS_pb_msgdesc_t *fields, const void *src_struct);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
