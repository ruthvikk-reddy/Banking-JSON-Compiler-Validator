#ifndef JSON_VALUE_H
#define JSON_VALUE_H

#include "../include/common.h"

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOL,
    JSON_NULL
} JsonType;

struct JsonValue;

typedef struct JsonObjectMember {
    char *key;
    struct JsonValue *value;
    int line;
    int col;
    struct JsonObjectMember *next;
} JsonObjectMember;

typedef struct JsonArrayElement {
    struct JsonValue *value;
    struct JsonArrayElement *next;
} JsonArrayElement;

typedef struct JsonValue {
    JsonType type;
    int line;
    int col;
    struct JsonValue *gc_next;
    union {
        char *string_val;
        struct {
            double num_val;
            char *raw_text;
        } number_val;
        bool bool_val;
        struct {
            JsonObjectMember *head;
            JsonObjectMember *tail;
            int count;
        } object_val;
        struct {
            JsonArrayElement *head;
            JsonArrayElement *tail;
            int count;
        } array_val;
    } data;
} JsonValue;

/* Value Constructors */
JsonValue *json_value_create_object(int line, int col);
JsonValue *json_value_create_array(int line, int col);
JsonValue *json_value_create_string(const char *str, int line, int col);
JsonValue *json_value_create_number(double num, const char *raw, int line, int col);
JsonValue *json_value_create_bool(bool b, int line, int col);
JsonValue *json_value_create_null(int line, int col);

/* Object & Array Builders */
bool json_object_add_member(JsonValue *obj, char *key, JsonValue *val, int key_line, int key_col);
void json_array_add_element(JsonValue *arr, JsonValue *val);

/* Lookup and inspection */
JsonObjectMember *json_object_find_member(const JsonValue *obj, const char *key);
int json_object_count_key_occurrences(const JsonValue *obj, const char *key);
const char *json_type_name(JsonType type);

/* Free */
void json_value_free(JsonValue *val);
void json_value_free_all(void);

#endif /* JSON_VALUE_H */
