# json
almost complete JSON parser in C

## is it usable?

pretty much, just missing unicode (unicode will be ignored)

## example usage

```c
#include "jj.h"

int main()
{
  struct json_value v = parse_string("{\"foo\": \"bar\", \"baz\": [1, 2.3, \"three\"]}");
  struct json_value* vv = get_value_from_object(&v, "baz");
  printf("baz[1]: %f\n", get_value_from_array(vv, 1)->decimal);
  printf("foo: %s\n", get_value_from_object(&v, "foo")->string);
  free_json(v);
  return 0;
}
```

## todo

- unicode
