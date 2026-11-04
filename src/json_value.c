#include "json_value.h"

static JsonValue *s_allocated_values_head = NULL;

static JsonValue *allocate_json_value(JsonType type, int line, int col) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (!v) {
        fprintf(stderr, "Fatal error: out of memory allocating JsonValue\n");
        exit(EXIT_FAILURE);
    }
    v->type = type;
    v->line = line;
    v->col = col;
    v->gc_next = s_allocated_values_head;
    s_allocated_values_head = v;
    return v;
}

JsonValue *json_value_create_object(int line, int col) {
    JsonValue *v = allocate_json_value(JSON_OBJECT, line, col);
    v->data.object_val.head = NULL;
    v->data.object_val.tail = NULL;
    v->data.object_val.count = 0;
    return v;
}

JsonValue *json_value_create_array(int line, int col) {
    JsonValue *v = allocate_json_value(JSON_ARRAY, line, col);
    v->data.array_val.head = NULL;
    v->data.array_val.tail = NULL;
    v->data.array_val.count = 0;
    return v;
}

JsonValue *json_value_create_string(const char *str, int line, int col) {
    JsonValue *v = allocate_json_value(JSON_STRING, line, col);
    v->data.string_val = str ? strdup(str) : strdup("");
    return v;
}

JsonValue *json_value_create_number(double num, const char *raw, int line, int col) {
    JsonValue *v = allocate_json_value(JSON_NUMBER, line, col);
    v->data.number_val.num_val = num;
    v->data.number_val.raw_text = raw ? strdup(raw) : NULL;
    return v;
}

JsonValue *json_value_create_bool(bool b, int line, int col) {
    JsonValue *v = allocate_json_value(JSON_BOOL, line, col);
    v->data.bool_val = b;
    return v;
}

JsonValue *json_value_create_null(int line, int col) {
    return allocate_json_value(JSON_NULL, line, col);
}

bool json_object_add_member(JsonValue *obj, char *key, JsonValue *val, int key_line, int key_col) {
    if (!obj || obj->type != JSON_OBJECT || !key || !val) {
        return false;
    }

    JsonObjectMember *m = (JsonObjectMember *)calloc(1, sizeof(JsonObjectMember));
    if (!m) {
        fprintf(stderr, "Fatal error: out of memory in json_object_add_member\n");
        exit(EXIT_FAILURE);
    }
    m->key = key;
    m->value = val;
    m->line = key_line;
    m->col = key_col;
    m->next = NULL;

    if (!obj->data.object_val.head) {
        obj->data.object_val.head = m;
        obj->data.object_val.tail = m;
    } else {
        obj->data.object_val.tail->next = m;
        obj->data.object_val.tail = m;
    }
    obj->data.object_val.count++;
    return true;
}

void json_array_add_element(JsonValue *arr, JsonValue *val) {
    if (!arr || arr->type != JSON_ARRAY || !val) return;

    JsonArrayElement *elem = (JsonArrayElement *)calloc(1, sizeof(JsonArrayElement));
    if (!elem) {
        fprintf(stderr, "Fatal error: out of memory in json_array_add_element\n");
        exit(EXIT_FAILURE);
    }
    elem->value = val;
    elem->next = NULL;

    if (!arr->data.array_val.head) {
        arr->data.array_val.head = elem;
        arr->data.array_val.tail = elem;
    } else {
        arr->data.array_val.tail->next = elem;
        arr->data.array_val.tail = elem;
    }
    arr->data.array_val.count++;
}

JsonObjectMember *json_object_find_member(const JsonValue *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT || !key) return NULL;
    JsonObjectMember *curr = obj->data.object_val.head;
    while (curr) {
        if (curr->key && strcmp(curr->key, key) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int json_object_count_key_occurrences(const JsonValue *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT || !key) return 0;
    int count = 0;
    JsonObjectMember *curr = obj->data.object_val.head;
    while (curr) {
        if (curr->key && strcmp(curr->key, key) == 0) {
            count++;
        }
        curr = curr->next;
    }
    return count;
}

const char *json_type_name(JsonType type) {
    switch (type) {
        case JSON_OBJECT: return "Object";
        case JSON_ARRAY:  return "Array";
        case JSON_STRING: return "String";
        case JSON_NUMBER: return "Number";
        case JSON_BOOL:   return "Boolean";
        case JSON_NULL:   return "Null";
        default:          return "Unknown";
    }
}

void json_value_free_all(void) {
    JsonValue *curr = s_allocated_values_head;
    while (curr) {
        JsonValue *next = curr->gc_next;

        switch (curr->type) {
            case JSON_STRING:
                if (curr->data.string_val) {
                    free(curr->data.string_val);
                    curr->data.string_val = NULL;
                }
                break;

            case JSON_NUMBER:
                if (curr->data.number_val.raw_text) {
                    free(curr->data.number_val.raw_text);
                    curr->data.number_val.raw_text = NULL;
                }
                break;

            case JSON_OBJECT: {
                JsonObjectMember *m = curr->data.object_val.head;
                while (m) {
                    JsonObjectMember *mnext = m->next;
                    if (m->key) free(m->key);
                    free(m);
                    m = mnext;
                }
                curr->data.object_val.head = NULL;
                curr->data.object_val.tail = NULL;
                break;
            }

            case JSON_ARRAY: {
                JsonArrayElement *e = curr->data.array_val.head;
                while (e) {
                    JsonArrayElement *enext = e->next;
                    free(e);
                    e = enext;
                }
                curr->data.array_val.head = NULL;
                curr->data.array_val.tail = NULL;
                break;
            }

            default:
                break;
        }

        free(curr);
        curr = next;
    }
    s_allocated_values_head = NULL;
}

void json_value_free(JsonValue *val) {
    (void)val;
    json_value_free_all();
}
