/* AS_pb_decode.h: Functions to decode protocol buffers. Depends on AS_pb_decode.c.
 * The main function is AS_pb_decode. You also need an input stream, and the
 * field descriptions created by nanoAS_pb_generator.py.
 */

#ifndef AS_PB_DECODE_H_INCLUDED
#define AS_PB_DECODE_H_INCLUDED

#include "AS_pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Structure for defining custom input streams. You will need to provide
 * a callback function to read the bytes from your storage, which can be
 * for example a file or a network socket.
 * 
 * The callback must conform to these rules:
 *
 * 1) Return false on IO errors. This will cause decoding to abort.
 * 2) You can use state to store your own data (e.g. buffer pointer),
 *    and rely on AS_pb_read to verify that no-body reads past bytes_left.
 * 3) Your callback may be used with substreams, in which case bytes_left
 *    is different than from the main stream. Don't use bytes_left to compute
 *    any pointers.
 */
struct AS_pb_istream_s
{
#ifdef AS_PB_BUFFER_ONLY
    /* Callback pointer is not used in buffer-only configuration.
     * Having an int pointer here allows binary compatibility but
     * gives an error if someone tries to assign callback function.
     */
    int *callback;
#else
    bool (*callback)(AS_pb_istream_t *stream, AS_pb_byte_t *buf, size_t count);
#endif

    void *state; /* Free field for use by callback implementation */
    size_t bytes_left;
    
#ifndef AS_PB_NO_ERRMSG
    const char *errmsg;
#endif
};

#ifndef AS_PB_NO_ERRMSG
#define AS_PB_ISTREAM_EMPTY {0,0,0,0}
#else
#define AS_PB_ISTREAM_EMPTY {0,0,0}
#endif

/***************************
 * Main decoding functions *
 ***************************/
 
/* Decode a single protocol buffers message from input stream into a C structure.
 * Returns true on success, false on any failure.
 * The actual struct pointed to by dest must match the description in fields.
 * Callback fields of the destination structure must be initialized by caller.
 * All other fields will be initialized by this function.
 *
 * Example usage:
 *    MyMessage msg = {};
 *    uint8_t buffer[64];
 *    AS_pb_istream_t stream;
 *    
 *    // ... read some data into buffer ...
 *
 *    stream = AS_pb_istream_from_buffer(buffer, count);
 *    AS_pb_decode(&stream, MyMessage_fields, &msg);
 */
bool AS_pb_decode(AS_pb_istream_t *stream, const AS_pb_msgdesc_t *fields, void *dest_struct);

/* Extended version of AS_pb_decode, with several options to control
 * the decoding process:
 *
 * AS_PB_DECODE_NOINIT:         Do not initialize the fields to default values.
 *                           This is slightly faster if you do not need the default
 *                           values and instead initialize the structure to 0 using
 *                           e.g. memset(). This can also be used for merging two
 *                           messages, i.e. combine already existing data with new
 *                           values.
 *
 * AS_PB_DECODE_DELIMITED:      Input message starts with the message size as varint.
 *                           Corresponds to parseDelimitedFrom() in Google's
 *                           protobuf API.
 *
 * AS_PB_DECODE_NULLTERMINATED: Stop reading when field tag is read as 0. This allows
 *                           reading null terminated messages.
 *                           NOTE: Until nanopb-0.4.0, AS_pb_decode() also allows
 *                           null-termination. This behaviour is not supported in
 *                           most other protobuf implementations, so AS_PB_DECODE_DELIMITED
 *                           is a better option for compatibility.
 *
 * Multiple flags can be combined with bitwise or (| operator)
 */
#define AS_PB_DECODE_NOINIT          0x01U
#define AS_PB_DECODE_DELIMITED       0x02U
#define AS_PB_DECODE_NULLTERMINATED  0x04U
bool AS_pb_decode_ex(AS_pb_istream_t *stream, const AS_pb_msgdesc_t *fields, void *dest_struct, unsigned int flags);

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define AS_pb_decode_noinit(s,f,d) AS_pb_decode_ex(s,f,d, AS_PB_DECODE_NOINIT)
#define AS_pb_decode_delimited(s,f,d) AS_pb_decode_ex(s,f,d, AS_PB_DECODE_DELIMITED)
#define AS_pb_decode_delimited_noinit(s,f,d) AS_pb_decode_ex(s,f,d, AS_PB_DECODE_DELIMITED | AS_PB_DECODE_NOINIT)
#define AS_pb_decode_nullterminated(s,f,d) AS_pb_decode_ex(s,f,d, AS_PB_DECODE_NULLTERMINATED)

#ifdef AS_PB_ENABLE_MALLOC
/* Release any allocated pointer fields. If you use dynamic allocation, you should
 * call this for any successfully decoded message when you are done with it. If
 * AS_pb_decode() returns with an error, the message is already released.
 */
void AS_pb_release(const AS_pb_msgdesc_t *fields, void *dest_struct);
#else
/* Allocation is not supported, so release is no-op */
#define AS_pb_release(fields, dest_struct) AS_PB_UNUSED(fields); AS_PB_UNUSED(dest_struct);
#endif


/**************************************
 * Functions for manipulating streams *
 **************************************/

/* Create an input stream for reading from a memory buffer.
 *
 * msglen should be the actual length of the message, not the full size of
 * allocated buffer.
 *
 * Alternatively, you can use a custom stream that reads directly from e.g.
 * a file or a network socket.
 */
AS_pb_istream_t AS_pb_istream_from_buffer(const AS_pb_byte_t *buf, size_t msglen);

/* Function to read from a AS_pb_istream_t. You can use this if you need to
 * read some custom header data, or to read data in field callbacks.
 */
bool AS_pb_read(AS_pb_istream_t *stream, AS_pb_byte_t *buf, size_t count);


/************************************************
 * Helper functions for writing field callbacks *
 ************************************************/

/* Decode the tag for the next field in the stream. Gives the wire type and
 * field tag. At end of the message, returns false and sets eof to true. */
bool AS_pb_decode_tag(AS_pb_istream_t *stream, AS_pb_wire_type_t *wire_type, uint32_t *tag, bool *eof);

/* Skip the field payload data, given the wire type. */
bool AS_pb_skip_field(AS_pb_istream_t *stream, AS_pb_wire_type_t wire_type);

/* Decode an integer in the varint format. This works for enum, int32,
 * int64, uint32 and uint64 field types. */
#ifndef AS_PB_WITHOUT_64BIT
bool AS_pb_decode_varint(AS_pb_istream_t *stream, uint64_t *dest);
#else
#define AS_pb_decode_varint AS_pb_decode_varint32
#endif

/* Decode an integer in the varint format. This works for enum, int32,
 * and uint32 field types. */
bool AS_pb_decode_varint32(AS_pb_istream_t *stream, uint32_t *dest);

/* Decode a bool value in varint format. */
bool AS_pb_decode_bool(AS_pb_istream_t *stream, bool *dest);

/* Decode an integer in the zig-zagged svarint format. This works for sint32
 * and sint64. */
#ifndef AS_PB_WITHOUT_64BIT
bool AS_pb_decode_svarint(AS_pb_istream_t *stream, int64_t *dest);
#else
bool AS_pb_decode_svarint(AS_pb_istream_t *stream, int32_t *dest);
#endif

/* Decode a fixed32, sfixed32 or float value. You need to pass a pointer to
 * a 4-byte wide C variable. */
bool AS_pb_decode_fixed32(AS_pb_istream_t *stream, void *dest);

#ifndef AS_PB_WITHOUT_64BIT
/* Decode a fixed64, sfixed64 or double value. You need to pass a pointer to
 * a 8-byte wide C variable. */
bool AS_pb_decode_fixed64(AS_pb_istream_t *stream, void *dest);
#endif

#ifdef AS_PB_CONVERT_DOUBLE_FLOAT
/* Decode a double value into float variable. */
bool AS_pb_decode_double_as_float(AS_pb_istream_t *stream, float *dest);
#endif

/* Make a limited-length substream for reading a AS_PB_WT_STRING field. */
bool AS_pb_make_string_substream(AS_pb_istream_t *stream, AS_pb_istream_t *substream);
bool AS_pb_close_string_substream(AS_pb_istream_t *stream, AS_pb_istream_t *substream);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
