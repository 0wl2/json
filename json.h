/*
 * json parser in C
 */

#pragma once

/*includes****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*macros******************************************************************************************/

#define PARSE_STRING \
	ptr->value->string = process_between_quotes(*p, &ptr->value->raw_string_length); \
	ptr->value->string_length = strlen(ptr->value->string); \
	*p += ptr->value->raw_string_length;

#define PARSE_BOOLEAN \
	switch(**p) { \
	case 't': \
		if(!strncmp("true", *p, 4)) { \
			ptr->value->number = 1; \
			*p += 4; \
		} else goto bad; \
		break; \
	case 'f': \
		if(!strncmp("false", *p, 5)) { \
			ptr->value->number = 0; \
			*p += 5; \
		} else goto bad; \
		break; \
	}

#define PARSE_NUMBER \
	ptr->value->decimal = atof(*p); \
	ptr->value->number = (int)ptr->value->decimal; \
	while(apart_of_number(*++(*p))) {}

#define PARSE_NULL \
	if(!strncmp("null", *p, 4)) { \
		free(ptr->value); \
		*p += 4; \
	} else goto bad; \

#define PARSE_OBJECT \
	(*p)++; \
	ptr->value->object = parse_object(p); \
	if(!ptr->value->object) goto bad;

#define PARSE_ARRAY \
	(*p)++; \
	ptr->value->array = parse_array(p); \
	if(!ptr->value->array) goto bad; \
	ptr->value->array_length = get_array_length(ptr->value->array);

#define VALUE_FETCH \
	switch(ptr->value->type) { \
	case STRING: PARSE_STRING; break; \
	case BOOLEAN: PARSE_BOOLEAN; break; \
	case NUMBER: PARSE_NUMBER; break; \
	case NOTHING: PARSE_NULL; break; \
	case OBJECT: PARSE_OBJECT; break; \
	case ARRAY:	PARSE_ARRAY; break; \
	case CLOSER: state = PARSE_FINISH; break; \
	} \
	state = CHECK_COMMA;

#define COMMA_NEXT(t, n) \
	if(**p == ',') { \
		ptr->next = (struct json_##t*)malloc(sizeof(struct json_##t)); \
		if(!ptr->next) goto bad; \
		ptr->next->prev = ptr; \
		ptr = ptr->next; \
		state = n; \
	} else state = FETCH_CLOSE;

#define ALLOC_VALUE \
	ptr->value = (struct json_value*)malloc(sizeof(struct json_value)); \
	if(!ptr->value) goto bad; \
	ptr->value->type = get_value_type(**p); \
	if(ptr->value->type == CLOSER) { \
		state = PARSE_FINISH; \
		free(ptr->value); \
	} else state = FETCH_VALUE;

/*structs & types*********************************************************************************/

enum json_type
{
	STRING,
	NUMBER,
	OBJECT,
	ARRAY,
	BOOLEAN,
	NOTHING,
	CLOSER
};

enum json_fetch_state
{
	FETCH_NAME,
	PRE_FETCH_VALUE_TYPE,
	FETCH_VALUE_TYPE,
	FETCH_VALUE,
	CHECK_COMMA,
	FETCH_CLOSE,
	PARSE_FINISH
};

struct json_object
{
	int key;
	struct json_value* value;

	struct json_object* prev;
	struct json_object* next;
};

struct json_array
{
	struct json_value* value;

	struct json_array* prev;
	struct json_array* next;
};

struct json_value
{
	enum json_type type;

	char* string;
	int string_length;
	int raw_string_length;

	int number;
	float decimal;

	struct json_array* array;
	int array_length;

	struct json_object* object;
};

/*function prototypes*****************************************************************************/

char*                     process_between_quotes(char*, int*);
unsigned int              crc32(char*);
int                       apart_of_number(char);
enum json_type            get_value_type(char);
int                       get_array_length(struct json_array*);

struct json_array*        parse_array(char**);
struct json_object*       parse_object(char**);

void                      free_object(struct json_object*);
void                      free_array(struct json_array*);
void                      free_json(struct json_value);

struct json_value         parse_string(char*);

struct json_value*        get_value_from_object(struct json_value*, char*);
struct json_value*        get_value_from_array(struct json_value*, int);

/*functions***************************************************************************************/

char* process_between_quotes(char* p, int* l)
{
	char* ptr1 = p, *ptr2 = NULL;
	char* pre = NULL, *new = NULL;
	int length = 0;

	do { 
		length++; 
		if(*ptr1 == '\\') { 
			if(*++ptr1 != '\0') continue; 
		} 
	} while(*++ptr1 != '"');

	pre = malloc(length);

	if(!pre) return NULL;

	strncpy(pre, p+1, length);

	new = malloc(length);

	if(!new) {
		free(pre);
		return NULL;
	}

	ptr1 = pre;
	ptr2 = new;

	do {
		if(*ptr1 == '\\') {
			switch(*++ptr1) {
			case '"': *ptr2 = '"'; break;
			case '\\': *ptr2 = '\\'; break;
			case '/': *ptr2 = '/'; break;
			case 'b': *ptr2 = '\b'; break;
			case 'f': *ptr2 = '\f'; break;
			case 'n': *ptr2 = '\n'; break;
			case 'r': *ptr2 = '\r'; break;
			case 't': *ptr2 = '\t'; break;
			case 'u': ptr += 4; length++; break; 
			}
		} else {
			*ptr2 = *ptr1;
		}

		ptr2++;
	} while(*++ptr1 != '\0' && *ptr1 != '"');
	
	free(pre);
	if(l) *l = length + 1;
	return new;
}

unsigned int crc32(char* p) 
{
	int i = 0, j;
	unsigned int crc = 0xFFFFFFFF, mask;

	while (p[i] != 0) {
		crc = crc ^ p[i++];
		for (j = 7; j >= 0; j--) {
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	}

	return ~crc;
}

int apart_of_number(char c)
{
	return isdigit(c) || 
	c == 'E' || 
	c == 'e' || 
	c == '+' || 
	c == '-' || 
	c == '.';
}

enum json_type get_value_type(char c)
{
	if(apart_of_number(c)) return NUMBER;
	switch(c) {
	case '"': return STRING;
	case '[': return ARRAY;
	case '{': return OBJECT;
	case 't': case 'f': return BOOLEAN;
	case '}': case ']': return CLOSER;
	default: case 'n': return NOTHING;
	}
}

int get_array_length(struct json_array* array)
{
	int l = 1;
	struct json_array* ptr = array;

	for(; ptr != NULL && 
		ptr->next && ptr->value; 
		ptr = ptr->next, l++) {}

	return l;
}

struct json_array* parse_array(char** p)
{
	struct json_array* array = (struct json_array*)malloc(sizeof(struct json_array));
	struct json_array* ptr = NULL;

	enum json_fetch_state state;

	if(!array) return NULL;

	ptr = array;

	state = FETCH_VALUE_TYPE;
	do {
		if(isspace(**p)) continue;

		switch(state) {
		case PARSE_FINISH: return array;
		case FETCH_NAME:
		case PRE_FETCH_VALUE_TYPE:
			goto bad;

		case FETCH_VALUE_TYPE: ALLOC_VALUE;

		/* intentional fallthrough */

		case FETCH_VALUE: VALUE_FETCH;

		/* intentional fallthrough */

		case CHECK_COMMA: COMMA_NEXT(array, FETCH_VALUE_TYPE);

		/* intentional fallthrough */

		case FETCH_CLOSE: if(**p == ']') state = PARSE_FINISH;
		}
	} while(*(*p)++ != '\0');

	return array;

bad:
	/*
	if(state != PARSE_FINISH)
		puts("ERROR: parse cycle incomplete");
 	*/

	free(array);
	return NULL;
}

struct json_object* parse_object(char** p)
{
	struct json_object* object = (struct json_object*)malloc(sizeof(struct json_object));
	struct json_object* ptr;
	char* obj_name = NULL;
	enum json_fetch_state state;

	if(!object) return NULL;

	ptr = object;

	state = FETCH_NAME;
	do {
		if(isspace(**p)) continue;

		switch(state) {
		case PARSE_FINISH: return object;
		case FETCH_NAME:
			if(**p == '"') {
				obj_name = process_between_quotes(*p, NULL);
				if(!obj_name) goto bad;

				ptr->key = (int)crc32(obj_name);
				free(obj_name);
				obj_name = NULL;

				state = PRE_FETCH_VALUE_TYPE;
			}

			break;

		case PRE_FETCH_VALUE_TYPE:
			if(**p == ':') {
				state = FETCH_VALUE_TYPE;
			}

			break;

		case FETCH_VALUE_TYPE: ALLOC_VALUE;

		/* intentional fallthrough */
		
		case FETCH_VALUE: VALUE_FETCH;

		/* intentional fallthrough */

		case CHECK_COMMA: COMMA_NEXT(object, FETCH_NAME);

		/* intentional fallthrough */

		case FETCH_CLOSE: if(**p == '}') state = PARSE_FINISH;
		}
	} while(*(*p)++ != '\0');

	return object;

bad:
	/*
	if(state != PARSE_FINISH)
		puts("ERROR: parse cycle incomplete");
 	*/

	free(object);
	return NULL;
}

struct json_value parse_string(char* buf)
{
	char* p = buf;
	struct json_value value;

	do {
		switch(*p) {
		case '{':
			++p;
			value.type = OBJECT;
			value.object = parse_object(&p);
			if(!value.object)
				return value;
			break;

		case '[':
			++p;
			value.type = ARRAY;
			value.array = parse_array(&p);
			value.array_length = get_array_length(value.array);
			if(!value.array)
				return value;
		}
	} while(*p++ != '\0');

	return value;
}

struct json_value* get_value_from_object(struct json_value* val, char* name)
{
	struct json_object* ptr = val->object;
	int h = (int)crc32(name);

	for(; ptr != NULL; ptr = ptr->next) {
		if(ptr->key == h)
			return ptr->value;
	}

	return NULL;
}

struct json_value* get_value_from_array(struct json_value* val, int index)
{
	int i = 0;
	struct json_array* ptr = val->array;

	if(index >= val->array_length) return NULL;
	while(i++ != index && ptr != NULL) ptr = ptr->next;
	return ptr->value;
}

void free_value(struct json_value* value)
{
	if(value) {
		free(value->string);
		free_object(value->object);
		free_array(value->array);
		free(value);
	}
}

void free_array(struct json_array* array)
{
	struct json_array* ptr = NULL;

	while(array) {
		ptr = array->next;
		free_value(array->value);		
		free(array);
		array = ptr;
	}
}

void free_object(struct json_object* object)
{
	struct json_object* ptr = NULL;

	while(object) {
		ptr = object->next;
		free_value(object->value);
		free(object);
		object = ptr;
	}
}

void free_json(struct json_value v)
{
	switch(v.type) {
	case STRING:
	case BOOLEAN:
	case NOTHING:
	case NUMBER:
	case CLOSER:
		break;
	
	case OBJECT: free_object(v.object); break;
	case ARRAY: free_array(v.array); break;
	}
}

/*************************************************************************************************/
