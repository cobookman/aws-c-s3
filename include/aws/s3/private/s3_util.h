#ifndef AWS_S3_UTIL_H
#define AWS_S3_UTIL_H

/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0.
 */

/* This file provides access to useful constants and simple utility functions. */

#include <aws/auth/signing_config.h>
#include <aws/common/byte_buf.h>
#include <aws/s3/s3.h>

#if ASSERT_LOCK_HELD
#    define ASSERT_SYNCED_DATA_LOCK_HELD(object)                                                                       \
        {                                                                                                              \
            int cached_error = aws_last_error();                                                                       \
            AWS_ASSERT(aws_mutex_try_lock(&(object)->synced_data.lock) == AWS_OP_ERR);                                 \
            aws_raise_error(cached_error);                                                                             \
        }
#else
#    define ASSERT_SYNCED_DATA_LOCK_HELD(object)
#endif
#define KB_TO_BYTES(kb) ((kb)*1024)
#define MB_TO_BYTES(mb) ((mb)*1024 * 1024)

struct aws_allocator;
struct aws_http_stream;
struct aws_http_headers;
struct aws_http_message;
struct aws_event_loop;

enum aws_s3_response_status {
    AWS_S3_RESPONSE_STATUS_SUCCESS = 200,
    AWS_S3_RESPONSE_STATUS_NO_CONTENT_SUCCESS = 204,
    AWS_S3_RESPONSE_STATUS_RANGE_SUCCESS = 206,
    AWS_S3_RESPONSE_STATUS_INTERNAL_ERROR = 500,
    AWS_S3_RESPONSE_STATUS_SLOW_DOWN = 503,
};

struct aws_cached_signing_config_aws {
    struct aws_allocator *allocator;
    struct aws_string *service;
    struct aws_string *region;
    struct aws_string *signed_body_value;

    struct aws_signing_config_aws config;
};

AWS_EXTERN_C_BEGIN

AWS_S3_API
extern const struct aws_byte_cursor g_content_md5_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_s3_client_version;

AWS_S3_API
extern const struct aws_byte_cursor g_user_agent_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_user_agent_header_product_name;

AWS_S3_API
extern const struct aws_byte_cursor g_acl_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_host_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_content_type_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_content_length_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_etag_header_name;

AWS_S3_API
extern const size_t g_s3_min_upload_part_size;

AWS_S3_API
extern const struct aws_byte_cursor g_s3_service_name;

AWS_S3_API
extern const struct aws_byte_cursor g_range_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_content_range_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_accept_ranges_header_name;

AWS_S3_API
extern const struct aws_byte_cursor g_post_method;

AWS_S3_API
extern const struct aws_byte_cursor g_head_method;

AWS_S3_API
extern const struct aws_byte_cursor g_delete_method;

AWS_S3_API
extern const uint32_t g_s3_max_num_upload_parts;

struct aws_cached_signing_config_aws *aws_cached_signing_config_new(
    struct aws_allocator *allocator,
    const struct aws_signing_config_aws *signing_config);

void aws_cached_signing_config_destroy(struct aws_cached_signing_config_aws *cached_signing_config);

/* Sets all headers specified for src on dest */
AWS_S3_API
void copy_http_headers(const struct aws_http_headers *src, struct aws_http_headers *dest);

/* Get a top-level (exists directly under the root tag) tag value. */
struct aws_string *get_top_level_xml_tag_value(
    struct aws_allocator *allocator,
    const struct aws_byte_cursor *tag_name,
    struct aws_byte_cursor *xml_body);

AWS_S3_API
void replace_quote_entities(struct aws_allocator *allocator, struct aws_string *str, struct aws_byte_buf *out_buf);

/* TODO could be moved to aws-c-common. */
AWS_S3_API
int aws_last_error_or_unknown(void);

AWS_S3_API
void aws_s3_add_user_agent_header(struct aws_allocator *allocator, struct aws_http_message *message);

/* Given the response headers list, finds the Content-Range header and parses the range-start, range-end and
 * object-size. All output arguments are optional.*/
AWS_S3_API
int aws_s3_parse_content_range_response_header(
    struct aws_allocator *allocator,
    struct aws_http_headers *response_headers,
    uint64_t *out_range_start,
    uint64_t *out_range_end,
    uint64_t *out_object_size);

/* Given response headers, parses the content-length from a content-length respone header.*/
AWS_S3_API
int aws_s3_parse_content_length_response_header(
    struct aws_allocator *allocator,
    struct aws_http_headers *response_headers,
    uint64_t *out_content_length);

/* Calculate the number of parts based on overall object-range and part_size. This takes into account aligning
 * part-ranges on part_size. (ie: if object_range_start is not evenly divisible by part_size, it is considered in the
 * middle of a contiguous part, and that first part will be smaller than part_size.) */
AWS_S3_API
uint32_t aws_s3_get_num_parts(size_t part_size, uint64_t object_range_start, uint64_t object_range_end);

/* Calculates the part range for a part given overall object range, size of each part, and the part's number. Note: part
 * numbers begin at one. This takes into account aligning part-ranges on part_size. Intended to be used in conjunction
 * with aws_s3_get_num_parts. part_number should be less than or equal to the result of aws_s3_get_num_parts. */
AWS_S3_API
void aws_s3_get_part_range(
    uint64_t object_range_start,
    uint64_t object_range_end,
    size_t part_size,
    uint32_t part_number,
    uint64_t *out_part_range_start,
    uint64_t *out_part_range_end);

AWS_EXTERN_C_END

#endif /* AWS_S3_UTIL_H */
