#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

//---------------------------------------
// DEFINITIONS
//---------------------------------------

#define MAX_STR_LEN 1024

#define IGNORE_SET " \n\r\t"

//typedef size_t uint;
typedef long unsigned int ulong;

#define AST_TYPE_LIST \
  X(AST_STR)          \
  X(AST_ID)           \
  X(AST_FLOAT)        \
  X(AST_INT)          

typedef enum ast_type_e {
#define X(tok) tok,
AST_TYPE_LIST
#undef X
} ast_type_e;

//---------------------------------------
// UTIL 
//---------------------------------------

#define error(...) {             \
  fprintf(stderr, "|ERROR| - "); \
  fprintf(stderr, __VA_ARGS__);  \
  fprintf(stderr, "\n");         \
}

#define log(...) {              \
  fprintf(stdout, "|LOG| - ");  \
  fprintf(stdout, __VA_ARGS__); \
  fprintf(stdout, "\n");        \
}

#define panic(...) {                   \
  fprintf(stderr, "|FATAL ERROR| - "); \
  fprintf(stderr, __VA_ARGS__);        \
  fprintf(stderr, "\n");               \
  exit(-1);                            \
}

void *alloc(size_t size) {
  void *res = malloc(size);
  if(!res) panic("unable to allocate %lu bytes", size);
  return res;
}

//---------------------------------------
// STACK 
//---------------------------------------

typedef struct stack_t {
  void           *obj;
  struct stack_t *next;
} stack_t;

stack_t *stack_new(void *obj) {
  stack_t *res = alloc(sizeof(stack_t));
  res->obj  = obj;
  res->next = 0;
  return res;
}

void stack_push(stack_t **this, void *obj) {
  if(!*this) {
    *this = stack_new(obj);
    return;
  }
  stack_t *s = alloc(sizeof(stack_t));
  s->obj  = obj;
  s->next = *this;
  *this = s;
}

void *stack_pop(stack_t **this) {
  if(!*this) return 0;
  stack_t *f = *this;
  void *res = (*this)->obj;
  *this = (*this)->next;
  free(f);
  return res;
}

void *stack_next(stack_t **this) {
  if(!*this) return 0;
  void *res = (*this)->obj;
  *this = (*this)->next;
  return res;
}

void stack_inverse(stack_t **this) {
  stack_t *prev = *this;
  stack_t *curr = (*this)->next;
  prev->next = 0;
  for(; curr; curr = curr->next) {
    curr->next = prev;
    prev = curr;
  }
  *this = prev;
}

stack_t *stack_from(void *obj, ...) {
  stack_t *res = 0;
  va_list args;
  va_start(args, obj);
  while(obj) {
    stack_push(&res, obj);
    obj = va_arg(args, void*);
  }
  va_end(args);
  stack_inverse(&res);
  return res;
}

#define stack_free(stack, free_fun) {                          \
  for(void *obj = 0; (obj = stack_pop(stack)); free_fun(obj)); \
}

// this macro also destroyes the stack
#define stack_for_each(stack, block) {                    \
  for(void *obj = 0; (obj = stack_pop(stack));) { block } \
}

//---------------------------------------
// INPUT
//---------------------------------------

typedef struct input_t {
  FILE *file;
} input_t;

input_t *input_new(FILE *file) {
  input_t *res = alloc(sizeof(input_t));
  res->file = file;
  return res;
}

void input_free(input_t *this) {
  if(!this) return;
  if(fclose(this->file) == EOF) error("unable to close input stream");
  free(this);
}

char input_next(input_t *this) {
  int c = fgetc(this->file); 
  if(c == EOF) {
    log("end of file");
    return (char)0;
  }
  return (char)c;
}

void input_rewind(input_t *this, ulong n) {
  if(fseek(this->file, (long int)-n, SEEK_CUR)) {
    panic("unable to rewind %lu chars", n);
  }
}

char input_peek(input_t *this) {
  char c = input_next(this);
  if(c) input_rewind(this, 1);
  return c;
}

int input_skip(input_t *this, char *set) {
  char c = 0;
  int count = -1;
  do {
    c = input_next(this);
    count++;
    if(!c) return -1;
  } while(strchr(set, c));
  input_rewind(this, 1);
  return count;
}

//---------------------------------------
// CHAR_UTIL 
//---------------------------------------

// checks if char is 0 .. 9
int is_num(char _c) {
  return (_c >= '0' && _c <= '9');
}

// checks if char is a .. z | A .. Z | _
int is_alpha(char _c) {
  return ((_c >= 'a' && _c <= 'z') || (_c >= 'A' && _c <= 'Z') || _c == '_');
}

// checks if char is a .. z | A .. Z | 0 .. 9 | _
int is_alpha_num(char _c) {
  return ((_c >= 'a' && _c <= 'z') || (_c >= 'A' && _c <= 'Z') || (_c >= '0' && _c <= '9') || _c == '_');
}

// checks if char is valid str char
int is_str(char _c) {
  return (_c >= 32 && _c <= 126);
}

//---------------------------------------
// AST_TYPES 
//---------------------------------------

typedef struct ast_type_t {
  ast_type_e type;
  void       *node;
  void (*free)(void*);
} ast_type_t;

ast_type_t *ast_type_new(ast_type_e type, void *node, void(*free)(void*)) {
  ast_type_t *res = alloc(sizeof(ast_type_t));
  res->type = type;
  res->node = node;
  res->free = free;
  return res;
}

void ast_type_free(ast_type_t *this) {
  if(!this) return;
  this->free(this->node);
  free(this);
}

typedef void (*free_f)(void*);

typedef char* str_t;

str_t str_new(char *str) {
  str_t res = alloc(strlen(str));
  strcpy(res, str);
  return res;
}

void str_free(str_t *this) {
  free(this);
}

typedef struct int_t {
  int val;
} int_t;

int_t *int_new(int val) {
  int_t *res = alloc(sizeof(int_t));
  res->val = val;
  return res;
}

void int_free(int_t *this) {
  if(!this) return;
  free(this);
}

typedef struct float_t {
  double val;
} float_t;

float_t *float_new(double val) {
  float_t *res = alloc(sizeof(float_t));
  res->val = val;
  return res;
}

void float_free(float_t *this) {
  if(!this) return;
  free(this);
}

typedef struct char_t {
  char val;
} char_t;

char_t *char_new(char val) {
  char_t *res = alloc(sizeof(char_t));
  res->val = val;
  return res;
}

void char_free(char_t *this) {
  if(!this) return;
  free(this);
}


//---------------------------------------
// PARSER_COMBINATOR
//---------------------------------------

typedef enum comb_e {
 COMB_JUST,
 COMB_OR,
 COMB_AND
} comb_e;

typedef ast_type_t*(*parse_f)(input_t*, ulong*);

typedef struct comb_t {
  comb_e type;
  union {
    parse_f parse;
    stack_t *stack;
  };
  ast_type_t *(*combine)(stack_t*); //only important for and parser
} comb_t;

comb_t *comb_new(comb_e type) {
  comb_t *res = alloc(sizeof(comb_t));
  res->type    = type;
  res->stack   = 0;
  res->combine = 0;
  return res;
}

void comb_free(comb_t *this) {
  if(!this) return;
  free(this);
}

ast_type_t *comb_parse(input_t *input, comb_t *this, ulong *rcr) {
  ast_type_t *res = 0;
  stack_t    *stack = 0;
  stack_t    *in_stack = this->stack;
  comb_t     *comb_elem = 0;
  ulong      rc = 0;
  if(this->type == COMB_JUST) {
    res = this->parse(input, &rc);
  } else if(this->type == COMB_OR) {
    while((comb_elem = stack_next(&in_stack))) {
      if((res = comb_elem->parse(input, &rc))) {
        break;
      }
      input_rewind(input, rc);
      rc = 0;
    }
  } else if(this->type == COMB_AND) {
    while((comb_elem = stack_next(&in_stack))) {
      if(!(res = comb_elem->parse(input, &rc))) {
        stack_free(&stack, ast_type_free);
        input_rewind(input, rc);
        rc = 0;
        break;
      }
      stack_push(&stack, res);
    }
    stack_inverse(&stack);
    res = this->combine(stack);
  } else {
    panic("undefined parser combinator");
  }
  *rcr += rc;
  return res;
}

//---------------------------------------
// PARSER 
//---------------------------------------

typedef struct parser_t {
  input_t *input;
  comb_t  *base;
} parser_t;

parser_t *parser_new(input_t *input, comb_t *base) {
  parser_t *res = alloc(sizeof(parser_t));
  res->input = input;
  res->base  = base;
  return res;
}

void parser_free(parser_t *this) {
  if(!this) return;
  input_free(this->input);
  comb_free(this->base);
  free(this);
}

#define input_move() (rc++, input_next(input))

#define input_fail() {    \
  input_rewind(input, 1); \
  return (ast_type_t*)0;  \
}

ast_type_t *parse_id(input_t *input, ulong *rcr) {
  ulong rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  
  rc += input_skip(input, IGNORE_SET);

  if(!is_alpha(buffer[0] = input_move())) input_fail();
  size_t i = 1;
  for(;is_alpha_num(buffer[i] = input_move()); i++) {
    if(i >= MAX_STR_LEN) panic("identifier string too long");
  }
  input_rewind(input, 0);
  buffer[i] = 0;

  *rcr += rc;
  return ast_type_new(AST_ID, str_new(buffer), (free_f)str_free);
}

ast_type_t *parse_int(input_t *input, ulong *rcr) {
  ulong rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  
  rc += input_skip(input, IGNORE_SET);

  size_t i = 0;
  if(!is_num(input_peek(input))) input_fail();
  for(;is_num(buffer[i] = input_move()); i++) {
    if(i >= MAX_STR_LEN) panic("integer string too long");
  }
  if(strchr(".f", buffer[i])) char_fail(); // TODO: what if: 123fid
  input_rewind(input, 1);

  *rcr += rc;
  return ast_type_new(AST_INT, strtol(buffer, 0, 10), (free_f)int_free);
}

ast_type_t *parse_float(input_t *input, ulong *rcr) {
  ulong rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  size_t i = 0;

  rc += input_skip(input, IGNORE_SET);

  if(!is_num(input_peek(input))) input_fail();
  for(; is_num(buffer[i] = input_move()); i++) {
    if(i >= MAX_STR_LEN) panic("float string too long");
  }
  if(buffer[i] == 'f') {
    *rcr += rc;
    return ast_type_new(AST_FLOAT, strtod(buffer, 0), (free_f)float_free);
  } else if(buffer[i] == '.') {
    for(i++; is_num(buffer[i] = input_move()); i++) {
      if(i >= MAX_STR_LEN) panic("float string too long");
    }
    input_rewind(input, 1);
    *rcr += rc;
    return ast_type_new(AST_FLOAT, strtod(buffer, 0), (free_f)float_free);
  }
  input_fail();
}

ast_type_t *parse(parser_t *parser) {
  ulong rc = 0;
  return comb_parse(parser->input, parser->base, &rc);
}

//---------------------------------------
// COMBINATOR_FUNCTIONS 
//---------------------------------------

comb_t *match_id() {
  comb_t *res = comb_new(COMB_JUST); 
  res->parse = parse_id;
  return res;
}

comb_t *match_int() {
  comb_t *res = comb_new(COMB_JUST);
  res->parse = parse_int;
  return res;
}

comb_t *math_float() {
  comb_t *res = comb_new(COMB_JUST);
  res->parse = parse_float;
  return res;
}

comb_t *match_or(stack_t *stack) {
  comb_t *res = comb_new(COMB_OR);
  res->stack = stack;
  return res;
}

comb_t *match_and(stack_t *stack) {
  comb_t *res = comb_new(COMB_AND);
  res->stack = stack;
  return res;
}

//---------------------------------------
// MAIN_PROGRAM 
//---------------------------------------

typedef struct foo_t {
  str_t *id;
  int_t *int_val;
  float_t *float_val;
} foo_t;

void foo_free(foo_t *this) {}

ast_type_t *test_comb(stack_t *stack) {
  foo_t *res = alloc(sizeof(foo_t));
  
  res->id        = stack_pop(&stack);
  res->int_val   = stack_pop(&stack);
  res->float_val = stack_pop(&stack);
  
  return ast_type_new(AST_STR, res, (free_f)foo_free);
}

void foo_print(foo_t *this) {
  printf("foo:\n");
  printf("id: %s\n", this->id);
  printf("int_val: %d\n", this->int_val->val);
  printf("float_val: %lf\n", this->float_val->val);
}

int main(int argc, char *argv) {
  if(argc < 2) panic("no input file specified");
  FILE *file = fopen(argv[1]);
  if(!file) panic("unable to open input file");
  
  comb_t *base = match_and(stack_from(match_id(), match_int(), match_float()));
  
  parser_t *parser = parser_new(file, base);
  
  foo_print(parse(parser));
  
  parser_free(parser);
  
  return 0;
}
