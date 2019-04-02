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

#define ID_NODE    -1
#define INT_NODE   -2
#define FLOAT_NODE -3
#define STR_NODE   -4
#define CHAR_NODE  -5
#define STACK_NODE -6

typedef int node_type;
typedef void (*free_f)(void*);

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

void nop_free(void *obj) {}

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
  if(!*this) return;
  stack_t *prev = 0;
  stack_t *curr = *this;
  stack_t *next = (*this)->next;
  for(; next; next = next->next) {
    curr->next = prev;
    prev = curr;
    curr = next;
  }
  curr->next = prev;
  *this = curr;
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
  int is_std;
} input_t;

input_t *input_new(FILE *file) {
  input_t *res = alloc(sizeof(input_t));
  if(!file) {
    res->file = stdin;
    res->is_std = 1;
  } else {
    res->file = file;
    res->is_std = 0;
  }
  return res;
}

void input_free(input_t *this) {
  if(!this) return;
  if(!this->is_std) {
    if(fclose(this->file) == EOF) error("unable to close input stream");
  }
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
    if(!c) return count;
  } while(strchr(set, c));
  input_rewind(this, 1);
  return count;
}

//---------------------------------------
// OUTPUT 
//---------------------------------------

typedef struct output_t {
  FILE *file;
  int is_std;
} output_t;

output_t *output_new(FILE *file) {
  output_t *res = alloc(sizeof(output_t));
  if(!file) {
    res->file = stdout;
    res->is_std = 1;
  } else {
    res->file = file;
    res->is_std = 0;
  }
  return res;
}

void output_free(output_t *this) {
  if(!this) return;
  if(!this->is_std) {
    if(fclose(this->file) == EOF) error("unable to close output stream");
  }
  free(this);
}

void emit(output_t *this, char *str) {
  fprintf(this->file, "%s", str);
}

void emit_line(output_t *this, char *str) {
  fprintf(this->file, "%s\n", str);
}

void emitf(output_t *this, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(this->file, fmt, args);
  va_end(args);
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
// NODE_TYPE
//---------------------------------------

typedef struct node_t {
  node_type type;
  void      *node;
  void (*free)(void*);
} node_t;

node_t *node_new(node_type type, void *node, void(*free)(void*)) {
  node_t *res = alloc(sizeof(node_t));
  res->type = type;
  res->node = node;
  res->free = free;
  return res;
}

void node_free(node_t *this) {
  if(!this) return;
  this->free(this->node);
  free(this);
}

void *node_unwrap(node_t *this) {
  if(!this) return 0;
  void *res = this->node;
  free(this);
  return res;
}

void node_stack_free(stack_t *this) {
  if(!this) return;
  stack_free(&this, node_free);
}

//---------------------------------------
// PRIMATIVE_NODE_TYPES
//---------------------------------------

// -- STRING ----------------------------

typedef struct str_t {
  char *val;
} str_t;

str_t *str_new(char *str) {
  str_t *res = alloc(sizeof(str_t));
  res->val = alloc(strlen(str));
  strcpy(res->val, str);
  return res;
}

void str_free(str_t *this) {
  if(!this) return;
  free(this->val);
  free(this);
}

// -- INTEGER ---------------------------

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

// -- FLOAT -----------------------------

typedef struct float_t {
  double val;
} float_t;

float_t *float_new(double val) {
  float_t *res = alloc(sizeof(float_t));
  res->val = val;
  return res;
}

void float_emit(float_t *this, output_t *out) {
  emitf(out, "%lf", this->val);
}

void float_free(float_t *this) {
  if(!this) return;
  free(this);
}

// -- CHAR ------------------------------

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
// COMBINATOR_STRUCTURE
//---------------------------------------

typedef enum comb_e {
 COMB_NONE,
 COMB_JUST,
 COMB_OR,
 COMB_AND,
 COMB_OPT
} comb_e;

typedef node_t*(*parse_f)(void*, input_t*, ulong*);

typedef struct comb_t {
  comb_e type;
  union {
    // -- JUST
    struct {
      parse_f parse;
      void *env;
      void (*env_free)(void*);
    };
    // -- OR/AND
    struct {
      stack_t *stack;
    };
    // -- OPT
    struct {
      struct comb_t *elem;
      struct comb_t *sep;
      // 1 if sep needs to be at the end | 0 otherwise
      // if 1 there also needs to be an element in front of last sep
      int sl; 
    };
  };
  node_type n_type;
  int ref_count;
} comb_t;

comb_t *comb_new() {
  comb_t *res = alloc(sizeof(comb_t));
  memset(res, 0, sizeof(comb_t));
  res->ref_count = 1;
  return res;
}

comb_t *comb_share(comb_t *this) {
  this->ref_count++;
  return this;
}

void comb_free(comb_t *this) {
  if(!this) return;
  this->ref_count--;
  if(this->ref_count > 0) return;
  switch(this->type) {
    case COMB_NONE: break;
    case COMB_JUST:
      if(this->env) this->env_free(this->env);
      break;
    case COMB_OR:
    case COMB_AND:
      stack_free(&this->stack, comb_free);
      break;
    case COMB_OPT:
      comb_free(this->elem);
      comb_free(this->sep);
      break;
  }
  free(this);
}

node_t *comb_parse(input_t *input, comb_t *this, ulong *rcr) {
  if(!this) return 0;
  node_t     *res = 0;
  stack_t    *stack = 0;
  stack_t    *in_stack = this->stack;
  comb_t     *comb_elem = 0;
  ulong      rc = 0;
  if(this->type == COMB_JUST) {
    res = this->parse(this->env, input, &rc);
  } else if(this->type == COMB_OR) {
    while((comb_elem = stack_next(&in_stack))) {
      if((res = comb_parse(input, comb_elem, &rc))) {
        break;
      }
      input_rewind(input, rc);
      rc = 0;
    }
  } else if(this->type == COMB_AND) {
    while((comb_elem = stack_next(&in_stack))) {
      if(!(res = comb_parse(input, comb_elem, &rc))) {
        input_rewind(input, rc);
        rc = 0;
        return 0;
      }
      stack_push(&stack, res);
    }
    stack_inverse(&stack);
    res = node_new(this->n_type, stack, (free_f)node_stack_free);
  } else if(this->type == COMB_OPT) {
    for(;;) {
      if(!(res = comb_parse(input, this->elem, &rc))) {
        break;
      }
      stack_push(&stack, res);
      if(this->sep) {
        if(!(res = comb_parse(input, this->sep, &rc))) {
          if(this->sl) {
            stack_free(&stack, node_free);
            input_rewind(input, rc);
            rc = 0;
            return 0;
          } else {
            break;
          }
        } 
      }
    }
    stack_inverse(&stack);
    res = node_new(this->n_type, stack, (free_f)node_stack_free);
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
  stack_t *comb_stack;
} parser_t;

parser_t *parser_new(input_t *input, comb_t *base, stack_t *comb_stack) {
  parser_t *res = alloc(sizeof(parser_t));
  res->input      = input;
  res->base       = base;
  res->comb_stack = comb_stack;
  return res;
}

void parser_free(parser_t *this) {
  if(!this) return;
  input_free(this->input);
  comb_free(this->base);
  stack_free(&this->comb_stack, comb_free);
  free(this);
}

#define input_move() (rc++, input_next(input))

#define input_fail() {     \
  input_rewind(input, rc); \
  return (node_t*)0;       \
}

// -- ID_PARSER -------------------------

node_t *parse_id(void *env, input_t *input, ulong *rcr) {
  ulong rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  
  rc += input_skip(input, IGNORE_SET);

  if(!is_alpha(buffer[0] = input_move())) input_fail();
  size_t i = 1;
  for(;is_alpha_num(buffer[i] = input_move()); i++) {
    if(i >= MAX_STR_LEN) panic("identifier string too long");
  }
  input_rewind(input, 1);
  rc--;
  buffer[i] = 0;

  *rcr += rc;
  return node_new(ID_NODE, str_new(buffer), (free_f)str_free);
}

// -- INTEGER_PARSER --------------------

node_t *parse_int(void *env, input_t *input, ulong *rcr) {
  ulong rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  
  rc += input_skip(input, IGNORE_SET);

  size_t i = 0;
  if(!is_num(input_peek(input))) input_fail();
  for(;is_num(buffer[i] = input_move()); i++) {
    if(i >= MAX_STR_LEN) panic("integer string too long");
  }
  if(strchr(".f", buffer[i])) input_fail(); // TODO: what if: 123fid
  input_rewind(input, 1);

  *rcr += rc;
  return node_new(INT_NODE, int_new(strtol(buffer, 0, 10)), (free_f)int_free);
}

// -- FLOAT_PARSER ----------------------

node_t *parse_float(void *env, input_t *input, ulong *rcr) {
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
    return node_new(FLOAT_NODE, float_new(strtod(buffer, 0)), (free_f)float_free);
  } else if(buffer[i] == '.') {
    for(i++; is_num(buffer[i] = input_move()); i++) {
      if(i >= MAX_STR_LEN) panic("float string too long");
    }
    input_rewind(input, 1);
    *rcr += rc;
    return node_new(FLOAT_NODE, float_new(strtod(buffer, 0)), (free_f)float_free);
  }
  input_fail();
}

// -- CHAR_PARSER -----------------------

node_t *parse_char(void *env, input_t *input, ulong *rcr) {
  ulong rc = 0;

  rc += input_skip(input, IGNORE_SET);
  
  char c = 0;
  if(input_move() != '\'') input_fail();
  if((c = input_move()) == '\\') {
    switch((c = input_move())) {
      case 'n': c = '\n'; break;
      case 't': c = '\t'; break;
      case 'r': c = '\r'; break;
      case '\'': c = '\''; break;
      case '\\': c = '\\'; break;
      default: panic("invalid escape char %c", c);
    }
  }
  if(!c) panic("end of file not expected");
  
  if(input_move() != '\'') panic("\"'\" expected");

  *rcr += rc;
  return node_new(CHAR_NODE, char_new(c), (free_f)char_free);
}

// -- STRING_PARSER ---------------------

node_t *parse_str(void *env, input_t *input, ulong *rcr) {
  ulong rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  
  rc += input_skip(input, IGNORE_SET);

  if(input_move() != '"') input_fail();
  size_t i = 0;
  for(; (buffer[i] = input_move()); i++) {
    if(buffer[i] == '"') {
      if(i && buffer[i-1] == '\\') continue;
      else break;
    }
    if(!is_str(buffer[i])) panic("invalid char inside string: %c", buffer[i]);
  }
  buffer[i] = 0;

  *rcr += rc;
  return node_new(STR_NODE, str_new(buffer), (free_f)str_free);
}

// --  ----------------------------------

typedef struct closure_env_t {
  char *ref;
  node_type type;
  int is_op;
} closure_env_t;

void closure_env_free(closure_env_t *this) {
  if(!this) return;
  free(this);
}

// -- CUSTOM_PARSER ---------------------

node_t *parse_op(closure_env_t *env, input_t *input, ulong *rcr) {
  ulong rc = 0;

  rc += input_skip(input, IGNORE_SET);

  for(size_t i = 0; env->ref[i]; i++) {
    if(input_move() != env->ref[i]) input_fail();
  }
  if(!env->is_op && is_alpha_num(input_peek(input))) input_fail();

  *rcr += rc;
  return node_new(env->type, 0, (free_f)nop_free);
}

// -- MAIN_PARSER  ----------------------

node_t *parse(parser_t *parser) {
  ulong rc = 0;
  return comb_parse(parser->input, parser->base, &rc);
}

//---------------------------------------
// COMBINATOR_FUNCTIONS 
//---------------------------------------

// -- PRIMITIVE_COMBINATORS -------------

comb_t *match_id() {
  comb_t *res = comb_new(); 
  res->type = COMB_JUST;
  res->parse = parse_id;
  return res;
}

comb_t *match_int() {
  comb_t *res = comb_new();
  res->type =COMB_JUST;
  res->parse = parse_int;
  return res;
}

comb_t *match_float() {
  comb_t *res = comb_new();
  res->type = COMB_JUST;
  res->parse = parse_float;
  return res;
}

comb_t *match_char() {
  comb_t *res = comb_new();
  res->type = COMB_JUST;
  res->parse = parse_char;
  return res;
}
 
comb_t *match_str() {
  comb_t *res = comb_new();
  res->type = COMB_JUST;
  res->parse = parse_str;
  return res;
}

comb_t *match_custom(char *str, node_type type, int is_op) {
  comb_t *res = comb_new();
  res->type = COMB_JUST;
  closure_env_t *env = alloc(sizeof(closure_env_t));
  env->ref = str;
  env->type = type;
  env->is_op = is_op;
  res->env = env;
  res->env_free = (void (*)(void*))closure_env_free;
  res->parse = (parse_f)parse_op;
  return res;
}

comb_t *match_op(char *op, node_type type) {
  return match_custom(op, type, 1);
}

comb_t *match_key(char *key, node_type type) {
  return match_custom(key, type, 0);
}

// -- JOINED_COMBINATORS ----------------

comb_t *match_or(comb_t *this, stack_t *stack) {
  if(this->type != COMB_NONE) error("combinator already defined");
  this->type = COMB_OR;
  this->stack = stack;
  return this;
}

comb_t *match_and(comb_t *this, node_type type, stack_t *stack) {
  if(this->type != COMB_NONE) error("combinator already defined");
  this->type   = COMB_AND;
  this->stack  = stack;
  this->n_type = type;
  return this;
}

comb_t *comb_add(comb_t *this, stack_t *stack) {
  if(this->type == COMB_JUST) panic("unable to add to \'JUST\' combinator");
  stack_inverse(&this->stack);
  for(void *obj = 0; (obj = stack_pop(&stack));) {
    stack_push(&this->stack, obj);
  }
  stack_inverse(&this->stack);
  return this;
}

comb_t *match_opt(comb_t *this, node_type type, comb_t *elem, comb_t *sep, int sl) {
  if(this->type != COMB_NONE) error("combinator already defined");
  this->type   = COMB_OPT;
  this->elem   = elem;
  this->sep    = sep;
  this->sl     = sl;
  this->n_type = type;
  return this;
}

//---------------------------------------
//---------------------------------------

// -- UTIL ------------------------------

typedef void (*stack_emit_f)(void*, output_t*);
void stack_emit(stack_t *this, output_t *out, stack_emit_f emit_f) {
  for(void *obj = 0; (obj = stack_next(&this));) {
    emit_f(obj, out);
  }
}

void char_emit(node_t *this, output_t *out) {
  emitf(out, "%c",  ((char_t*)this->node)->val);
}

void charl_emit(node_t *this, output_t *out) {
  emit(out, "'");
  char_emit(this, out);
  emit(out, "'");
}

void int_emit(node_t *this, output_t *out) {
  emitf(out, "%d", ((int_t*)this->node)->val);
}

void str_emit(node_t *this, output_t *out) {
  emit(out, ((str_t*)this->node)->val);
}

void strl_emit(node_t *this, output_t *out) {
  emit(out, "\"");
  str_emit(this, out);
  emit(out, "\"");
}

//---------------------------------------
// CUSTOM_NODE_TYPES
//---------------------------------------
// -- DECLARATIONS ----------------------
void type_emit_head(node_t *this, output_t *out);
void type_emit_tail(node_t *this, output_t *out);
void type_emit(node_t *this, output_t *out);
void var_emit(node_t *this, output_t *out);
void struct_emit(node_t *this, output_t *out);
void decl_emit(node_t *this, output_t *out);
void fun_emit(node_t *this, output_t *out);
void stm_emit(node_t *this, output_t *out);
void exp_emit(node_t *this, output_t *out);
// --  ----------------------------------
#define U8_NODE 1
#define I8_NODE 2
#define U16_NODE 3
#define I16_NODE 4
#define U32_NODE 5
#define I32_NODE 6
#define U64_NODE 7
#define I64_NODE 8
#define F32_NODE 9
#define F64_NODE 10
#define VOID_NODE 11
#define PTR_NODE 12
#define DECL_NODE 765
#define DECL_LIST_NODE 766
#define STRUCT_NODE 13
#define VAR_NODE 14
#define VAR_LIST_NODE 55
#define PARAM_LIST_NODE 5764
#define FUN_NODE 66

// TYPES
#define ID_TYPE_NODE 9001
#define PTR_TYPE_NODE 9002
#define FUN_TYPE_NODE 9003
#define ARR_TYPE_NODE 9004
#define TYPE_LIST_NODE 9005

// STATEMENTS
#define EXP_STM_NODE 301
#define JMP_STM_NODE     303
#define JMP_CON_STM_NODE 308
#define LABEL_STM_NODE   304
#define RET_STM_NODE 305
#define STM_LIST_NODE 306

// EXPRESSIONS
#define INT_EXP_NODE 700
#define ID_EXP_NODE  701
#define FLOAT_EXP_NODE 702
#define STR_EXP_NODE 709
#define CALL_EXP_NODE 703
#define EXP_LIST_NODE 708

#define L_C_B_NODE 50
#define R_C_B_NODE 51
#define L_R_B_NODE 52
#define R_R_B_NODE 53
#define L_S_B_NODE 54
#define R_S_B_NODE 55
#define LT_NODE    801
#define GT_NODE    802
#define DOT_NODE   800
#define ARROW_NODE 100
#define COMMA_NODE 101
#define COLON_NODE 56
#define SEMICOLON_NODE 57
#define EQ_NODE  909
#define AS_NODE  910

#define SIZEOF_NODE 400
#define JMP_NODE    401
#define RET_NODE    402

// -- TYPE ------------------------------

// ID_TYPE_NODE: 
//  | STR
// PTR_TYPE_NODE: 
//  | *
//  | TYPE
// FUN_TYPE_NODE: 
//  | ( 
//  | | TYPE
//  | | ...
//  | )
//  | ->
//  | TYPE
// ARR_TYPE_NODE: 
//  | [
//  | EXP
//  | ;
//  | EXP
//  | ]

void type_emit_head(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  switch(this->type) {
    case ID_TYPE_NODE:
      str_emit(stack_next(&stack), out);
      break;
    case PTR_TYPE_NODE: {
      stack_next(&stack); // *
      type_emit_head(stack_next(&stack), out);
      emit(out, "*");
      break;
    }
    case FUN_TYPE_NODE: {
      stack_next(&stack); // (
      stack_next(&stack); // stack_t
      stack_next(&stack); // )
      stack_next(&stack); // ->
      type_emit(stack_next(&stack), out);
      emit(out, "(*");
      break;
    }
    case ARR_TYPE_NODE: {
      stack_next(&stack); // [
      type_emit_head(stack_next(&stack), out);
      break;
    }
  }
}

void type_emit_tail(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  switch(this->type) {
    case ID_TYPE_NODE:
      break;
    case PTR_TYPE_NODE:
      stack_next(&stack); // *
      type_emit_tail(stack_next(&stack), out);
      break;
    case ARR_TYPE_NODE: {
      stack_next(&stack); // [
      node_t *n = stack_next(&stack);
      stack_next(&stack); // ;
      emit(out, "[");
      exp_emit(stack_next(&stack), out);
      emit(out, "]");
      type_emit_tail(n, out);
      break;
    }
    case FUN_TYPE_NODE: {
      stack_next(&stack); // (
      stack = node_unwrap(stack_next(&stack)); 
      emit(out, ")(");
      node_t *obj = stack_next(&stack);
      while(obj) {
        type_emit(obj, out);
        if((obj = stack_next(&stack))) emit(out, ", ");
      }
      emit(out, ")");
      break;
    }
  }
}

void type_emit(node_t *this, output_t *out) {
  type_emit_head(this, out);
  type_emit_tail(this, out);
}

// -- VARIABLE --------------------------

// VAR_NODE:
//  | STR
//  | :
//  | TYPE

void var_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  node_t *id_node = stack_next(&stack);
  stack_next(&stack); // :
  node_t *type_node = stack_next(&stack);
  type_emit_head(type_node, out);
  emit(out, " ");
  str_emit(id_node, out);
  emit(out, " ");
  type_emit_tail(type_node, out);
}

// -- STRUCT ----------------------------

// STRUCT_NODE:
//  | STR
//  | {
//  | | VAR
//  | | ...
//  | }

void var_member_emit(node_t *this, output_t *out) {
  var_emit(this, out);
  emit_line(out, ";");
}

void struct_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  node_t *id_node = stack_next(&stack);
  stack_next(&stack);
  stack_t *var_stack = node_unwrap(stack_next(&stack));
  emit(out, "typedef struct ");
  str_emit(id_node, out);
  emit(out, " ");
  str_emit(id_node, out);
  emit_line(out, ";");
  emit(out, "typedef struct ");
  str_emit(id_node, out);
  emit_line(out, " {");
  stack_emit(var_stack, out, (stack_emit_f)var_member_emit);
  emit(out, "} ");
  str_emit(id_node, out);
  emit_line(out, ";");
}

// -- FUNCTION --------------------------

// FUNCTION_NODE:
//  | STR
//  | (
//  | | VAR
//  | | ...
//  | )
//  | ->
//  | TYPE
//  | | DECL
//  | | ...
//  | {
//  | | STM
//  | | ...
//  | }

// DECL_NODE:
//  | VAR
//  | =
//  | EXP
//  | ;

void decl_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  node_t *var_node = stack_next(&stack);
  stack_next(&stack);
  node_t *exp_node = stack_next(&stack);
  var_emit(var_node, out);
  emit(out, " = ");
  exp_emit(exp_node, out);
  emit_line(out, ";");
}

void fun_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  node_t *id_node = stack_next(&stack);
  stack_next(&stack);
  stack_t *var_stack = node_unwrap(stack_next(&stack));
  stack_next(&stack);
  stack_next(&stack);
  node_t *type_node = stack_next(&stack);
  stack_t *decl_stack = node_unwrap(stack_next(&stack));
  stack_next(&stack);
  stack_t *stm_stack = node_unwrap(stack_next(&stack));
  type_emit_head(type_node, out);
  emit(out, " ");
  str_emit(id_node, out);
  emit(out, "(");
  node_t *var = stack_next(&var_stack);
  while(var) {
    var_emit(var, out);
    var = stack_next(&var_stack);
    if(var) emit(out, ", ");
  }
  emit(out, ")");
  type_emit_tail(type_node, out);
  emit_line(out, " {");
  stack_emit(decl_stack, out, (stack_emit_f)decl_emit);
  stack_emit(stm_stack, out, (stack_emit_f)stm_emit);
  emit_line(out, "}");
}

// -- STATEMENT -------------------------

// SEMICOLON_NODE:
//  ;
// EXP_STM_NODE: 
//  | EXP
//  | ;
// LABEL_STM_NODE: 
//  | STR
//  | :
// JMP_CON_STM_NODE: 
//  | jmp
//  | EXP
//  | STR
//  | ;
// JMP_STM_NODE: 
//  | jmp
//  | STR
//  | ;
// RET_STM_NODE: 
//  | ret
//  | EXP
//  | ;

void stm_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  switch(this->type) {
    case SEMICOLON_NODE:
      break;
    case EXP_STM_NODE: 
      exp_emit(stack_next(&stack), out);
      emit_line(out, ";");
      break;
    case LABEL_STM_NODE:
      str_emit(stack_next(&stack), out);
      emit_line(out, ":");
      break;
    case JMP_CON_STM_NODE:
      emit(out, "if(");
      stack_next(&stack);
      exp_emit(stack_next(&stack), out);
      emit(out, ") goto ");
      str_emit(stack_next(&stack), out);
      emit(out, ";");
      break;
    case RET_STM_NODE:
      emit(out, "return ");
      stack_next(&stack);
      exp_emit(stack_next(&stack), out);
      emit_line(out, ";");
      break;
  }
}

// -- EXPRESSION ------------------------

// INT_EXP: 
//  | INT
// ID_EXP: 
//  | STR
// STR_EXP: 
//  | STR
// FLOAT_EXP: 
//  | FLOAT
// CALL_EXP: 
//  | (
//  | | EXP
//  | | ...
//  | )

void exp_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  switch(this->type) {
    case INT_EXP_NODE:
      int_emit(stack_next(&stack), out);
      break;
    case ID_EXP_NODE:
      str_emit(stack_next(&stack), out);
      break;
    case STR_EXP_NODE:
      strl_emit(stack_next(&stack), out);
      break;
    case FLOAT_EXP_NODE:
      float_emit(stack_next(&stack), out);
      break;
    case CALL_EXP_NODE: {
      stack_next(&stack);
      stack_t *exp_stack = node_unwrap(stack_next(&stack));
      node_t *exp = stack_next(&exp_stack);
      if(!exp) {
        error("invalid function call exp");
        break;
      }
      exp_emit(exp, out);
      emit(out, "(");
      exp = stack_next(&exp_stack);
      while(exp) {
        exp_emit(exp, out);
        exp = stack_next(&exp_stack);
        if(exp) emit(out, ", ");
      }
      emit(out, ")");
      break;       
    }
  }
}

// --  ----------------------------------

parser_t *parser_create(input_t *input) {
  comb_t *base_comb        = comb_new();
  comb_t *struct_comb      = comb_new();
  comb_t *fun_comb         = comb_new();
  comb_t *var_comb         = comb_new();
  comb_t *var_list_comb    = comb_new();
  comb_t *decl_comb        = comb_new();
  comb_t *decl_list_comb   = comb_new();
  comb_t *param_list_comb  = comb_new();
  comb_t *type_comb        = comb_new();
  comb_t *stm_comb         = comb_new();
  comb_t *stm_list_comb    = comb_new();
  comb_t *exp_comb         = comb_new();
  
  // types
  comb_t *id_type_comb     = comb_new();
  comb_t *ptr_type_comb    = comb_new();
  comb_t *fun_type_comb    = comb_new();
  comb_t *arr_type_comb    = comb_new();
  comb_t *type_list_comb   = comb_new();
  
  // statements 
  comb_t *exp_stm_comb     = comb_new();
  comb_t *label_stm_comb   = comb_new();
  comb_t *jmp_con_stm_comb = comb_new();
  comb_t *jmp_stm_comb     = comb_new();
  comb_t *ret_stm_comb     = comb_new();
  
  // expressions
  comb_t *int_exp_comb     = comb_new();
  comb_t *id_exp_comb      = comb_new();
  comb_t *str_exp_comb     = comb_new();
  comb_t *float_exp_comb   = comb_new();
  comb_t *exp_list_comb    = comb_new();
  comb_t *call_exp_comb    = comb_new();
  comb_t *dot_exp_comb     = comb_new();
  comb_t *arrow_exp_comb   = comb_new();

  // operators
  comb_t *l_c_b_o     = match_op("{", L_C_B_NODE);
  comb_t *r_c_b_o     = match_op("}", R_C_B_NODE);
  comb_t *l_r_b_o     = match_op("(", L_R_B_NODE);
  comb_t *r_r_b_o     = match_op(")", R_R_B_NODE);
  comb_t *l_s_b_o     = match_op("[", L_S_B_NODE);
  comb_t *r_s_b_o     = match_op("]", R_S_B_NODE);
  comb_t *lt_o        = match_op("<", LT_NODE);
  comb_t *gt_o        = match_op(">", GT_NODE);
  comb_t *dot_o       = match_op(".", DOT_NODE);
  comb_t *arrow_o     = match_op("->", ARROW_NODE);
  comb_t *colon_o     = match_op(":", COLON_NODE);
  comb_t *semicolon_o = match_op(";", SEMICOLON_NODE);
  comb_t *comma_o     = match_op(",", COMMA_NODE);
  comb_t *eq_o        = match_op("=", EQ_NODE);
  comb_t *as_o        = match_op("*", AS_NODE);
  
  // keywords
  comb_t *u8_k     = match_key("u8", U8_NODE);
  comb_t *u16_k    = match_key("u16", U16_NODE);
  comb_t *sizeof_k = match_key("sizeof", SIZEOF_NODE);
  comb_t *jmp_k    = match_key("jmp", JMP_NODE);
  comb_t *ret_k    = match_key("ret", RET_NODE);
  
  stack_t *comb_stack = stack_from(base_comb, struct_comb, fun_comb, var_comb, 
                           var_list_comb, decl_comb, decl_list_comb, param_list_comb, 
                           type_comb, stm_comb, stm_list_comb, exp_comb, 
                           id_type_comb, ptr_type_comb, fun_type_comb, arr_type_comb,
                           exp_stm_comb, label_stm_comb, jmp_con_stm_comb, jmp_stm_comb, 
                           ret_stm_comb, int_exp_comb, id_exp_comb, str_exp_comb, 
                           float_exp_comb, exp_list_comb, call_exp_comb, dot_exp_comb, 
                           arrow_exp_comb, l_c_b_o, 
                           r_c_b_o, l_r_b_o, r_r_b_o, lt_o, gt_o, dot_o, arrow_o, 
                           colon_o, semicolon_o, comma_o, eq_o, u8_k, u16_k, sizeof_k, 
                           jmp_k, ret_k ,0);

#define share comb_share
  
  // types
  type_list_comb  = match_opt(type_list_comb, TYPE_LIST_NODE, share(type_comb), share(comma_o), 0);
  id_type_comb    = match_and(id_type_comb, ID_TYPE_NODE, stack_from(match_id(), 0)); 
  ptr_type_comb   = match_and(ptr_type_comb, PTR_TYPE_NODE, stack_from(share(as_o), share(type_comb), 0));
  fun_type_comb   = match_and(fun_type_comb, FUN_TYPE_NODE, stack_from(share(l_r_b_o), share(type_list_comb), share(r_r_b_o), share(arrow_o), share(type_comb), 0));
  arr_type_comb   = match_and(arr_type_comb, ARR_TYPE_NODE, stack_from(share(l_s_b_o), share(type_comb), share(semicolon_o), share(exp_comb), share(r_s_b_o), 0));
  type_comb       = match_or(type_comb, stack_from(share(arr_type_comb), share(fun_type_comb), share(ptr_type_comb), share(id_type_comb), 0));

  var_comb        = match_and(var_comb, VAR_NODE, stack_from(match_id(), share(colon_o), share(type_comb), 0));
  var_list_comb   = match_opt(var_list_comb, VAR_LIST_NODE, share(var_comb), share(semicolon_o), 1);
  decl_comb       = match_and(decl_comb, DECL_NODE, stack_from(share(var_comb), share(eq_o), share(exp_comb), 0));
  decl_list_comb  = match_opt(decl_list_comb, DECL_LIST_NODE, share(decl_comb), share(semicolon_o), 1);
  param_list_comb = match_opt(param_list_comb, PARAM_LIST_NODE, share(var_comb), share(comma_o), 0);
  struct_comb     = match_and(struct_comb, STRUCT_NODE, stack_from(match_id(), share(l_c_b_o), share(var_list_comb), share(r_c_b_o), 0));
  fun_comb        = match_and(fun_comb, FUN_NODE, stack_from(match_id(), share(l_r_b_o), share(param_list_comb), share(r_r_b_o), share(arrow_o), share(type_comb), share(decl_list_comb), share(l_c_b_o), share(stm_list_comb), share(r_c_b_o), 0));
  
  exp_comb       = match_or(exp_comb, stack_from(share(int_exp_comb), share(id_exp_comb), share(str_exp_comb), share(float_exp_comb), share(call_exp_comb), 0));

  stm_comb        = match_or(stm_comb, stack_from(share(semicolon_o), share(exp_stm_comb), share(label_stm_comb), share(jmp_stm_comb), share(jmp_con_stm_comb), share(ret_stm_comb), 0));
  stm_list_comb   = match_opt(stm_list_comb, STM_LIST_NODE, share(stm_comb), 0, 0);
  
  // statements
  exp_stm_comb     = match_and(exp_stm_comb, EXP_STM_NODE, stack_from(share(exp_comb), share(semicolon_o), 0));
  label_stm_comb   = match_and(label_stm_comb, LABEL_STM_NODE, stack_from(match_id(), share(colon_o), 0));
  jmp_con_stm_comb = match_and(jmp_con_stm_comb, JMP_CON_STM_NODE, stack_from(share(jmp_k), share(exp_comb), match_id(), share(semicolon_o), 0));
  jmp_stm_comb     = match_and(jmp_stm_comb, JMP_STM_NODE, stack_from(share(jmp_k), match_id(), share(semicolon_o), 0));
  ret_stm_comb     = match_and(ret_stm_comb, RET_STM_NODE, stack_from(share(ret_k), share(exp_comb), share(semicolon_o), 0));

  // expressions
  int_exp_comb     = match_and(int_exp_comb, INT_EXP_NODE, stack_from(match_int(), 0));
  id_exp_comb      = match_and(id_exp_comb, ID_EXP_NODE, stack_from(match_id(), 0));
  str_exp_comb     = match_and(str_exp_comb, STR_EXP_NODE, stack_from(match_str(), 0));
  float_exp_comb   = match_and(float_exp_comb, FLOAT_EXP_NODE, stack_from(match_float(), 0));
  exp_list_comb    = match_opt(exp_list_comb, EXP_LIST_NODE, share(exp_comb), 0, 0);
  call_exp_comb    = match_and(call_exp_comb, CALL_EXP_NODE, stack_from(share(l_r_b_o), share(exp_list_comb), share(r_r_b_o), 0));
  
#undef share
  
  base_comb = match_or(base_comb, stack_from(struct_comb, fun_comb, 0));
  
  return parser_new(input, comb_share(base_comb), comb_stack);
}

//---------------------------------------
//---------------------------------------

//---------------------------------------
// FILE_PREFIX 
//---------------------------------------

const char *file_prefix =            "\
#define set(var, val) var = val     \n\
#define ref(var) &var               \n\
#define deref(var) *var             \n\
#define get(var1, var2) var1.var2   \n\
#define pget(var1, var2) var1->var2 \n\
#define cast(var, type) ((type)var) \n\
#define size(exp) sizeof(exp)       \n\
";

//---------------------------------------
//---------------------------------------


int main(int argc, char **argv) {
  printf("+---------------------------+\n");
  printf("| Starting SC-Lang Compiler |\n");
  printf("| Author:  Gerrit Proessl   |\n");
  printf("| Version: 0.0.1            |\n");
  printf("+---------------------------+\n");

  // open input file
  if(argc < 2) panic("no input file specified");
  FILE *inf = fopen(argv[1], "r");
  if(!inf) panic("unable to open input file");

  // open output file
  FILE *outf = 0;
  if(argc == 3) {
    outf = fopen(argv[2], "w");
    if(!outf) panic("unable to open outpuf file");
  }

  input_t  *input  = input_new(inf);
  output_t *output = output_new(outf);
  
  parser_t *parser = parser_create(input);
  
  // print prefix
  emitf(output, "%s\n", file_prefix);

  for(node_t *node = 0;;) {
    node = parse(parser);
    if(!node) break;
    switch(node->type) {
      case STRUCT_NODE: {
        log("parsed struct");
        struct_emit(node, output);
        break;
      }
      case FUN_NODE: {
        log("parse function");
        fun_emit(node, output);
        break;
      }
      default:
        panic("parsed undefined node");
    }
  }
  
  printf("done!\n");
  
  // cleanup
  parser_free(parser);
  output_free(output);
  
  return 0;
}
