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
#define EOF_NODE   -7

typedef int node_type;
typedef void (*free_f)(void*);

//---------------------------------------
// UTIL 
//---------------------------------------

//#define DEBUG

#define error(...) {             \
  fprintf(stderr, "|ERROR| - "); \
  fprintf(stderr, __VA_ARGS__);  \
  fprintf(stderr, "\n");         \
}

#ifdef DEBUG

#define log(...) {              \
  fprintf(stdout, "|LOG| - ");  \
  fprintf(stdout, __VA_ARGS__); \
  fprintf(stdout, "\n");        \
}

#else

#define log(...) 

#endif

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

void stack_free(stack_t **stack, void(*free_f)(void*)) {
  for(void *obj = 0; (obj = stack_pop(stack)); free_f(obj));
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
  input_rewind(this, 1);
  return c;
}

int input_skip(input_t *this) {
  char c = 0;
  int count = -1;
  // 0 - normal | 1 - in line comment | 2 - in multiline comment
  int state = 0; 
  do {
    c = input_next(this);
    count++;
    if(state == 1) {
      if(c == '\n') { 
        c = input_next(this);
        count++;
        state = 0;
      }
    } else if(state == 2) {
      if(c == '*' && input_peek(this) == '/') {
        c = (input_next(this), input_next(this));
        count += 2;
        state = 0;
      }
    } else {
      if(c == '/') {
        char p = input_peek(this);
        if(p == '/') state = 1;
        if(p == '*') { 
          state = 2;
          input_next(this);
          count++;
        }
      }
    }
  } while(c && (state || strchr(IGNORE_SET, c)));
  if(c) input_rewind(this, 1);
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
  return this->node;
}

void node_stack_free(stack_t *this) {
  if(!this) return;
  stack_free(&this, (free_f)node_free);
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
 COMB_OPT,
 COMB_EXPECT
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
    // -- EXPECT
    struct {
      struct comb_t *exp;
      char *desc;
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

void comb_error(comb_t *this, input_t *input) {
  fprintf(stdout, "|PARSER ERROR| Expected: %s\n", this->desc);
  exit(-1);
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
      stack_free(&this->stack, (free_f)comb_free);
      break;
    case COMB_OPT:
      comb_free(this->elem);
      comb_free(this->sep);
      break;
    case COMB_EXPECT:
      comb_free(this->exp);
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
        stack_free(&stack, (free_f)node_free); 
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
            stack_free(&stack, (free_f)node_free);
            input_rewind(input, rc);
            rc = 0;
            return 0;
          } else {
            break;
          }
        } else {
          node_free(res);
        } 
      }
    }
    stack_inverse(&stack);
    res = node_new(this->n_type, stack, (free_f)node_stack_free);
  } else if(this->type == COMB_EXPECT) {
    res = comb_parse(input, this->exp, &rc);
    if(!res) {
      comb_error(this, input);
    }
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
  stack_free(&this->comb_stack, (free_f)comb_free);
  free(this);
}

#define input_move() ((cc = input_next(input)) ? (rc++, cc) : cc) //(rc++, input_next(input))

#define input_fail() {     \
  input_rewind(input, rc); \
  return (node_t*)0;       \
}

// -- ID_PARSER -------------------------

node_t *parse_id(void *env, input_t *input, ulong *rcr) {
  ulong rc = 0;
  char cc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  
  rc += input_skip(input);

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
  char cc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  
  rc += input_skip(input);

  size_t i = 0;
  if(!is_num(input_peek(input))) input_fail();
  for(;is_num(buffer[i] = input_move()); i++) {
    if(i >= MAX_STR_LEN) panic("integer string too long");
  }
  if(strchr(".f", buffer[i])) input_fail();
  input_rewind(input, 1);

  *rcr += rc;
  return node_new(INT_NODE, int_new(strtol(buffer, 0, 10)), (free_f)int_free);
}

// -- FLOAT_PARSER ----------------------

node_t *parse_float(void *env, input_t *input, ulong *rcr) {
  ulong rc = 0;
  char cc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  size_t i = 0;

  rc += input_skip(input);

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
  char cc = 0;

  rc += input_skip(input);
  
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
  char cc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  
  rc += input_skip(input);

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
  char cc = 0;

  rc += input_skip(input);

  for(size_t i = 0; env->ref[i]; i++) {
    if(input_move() != env->ref[i]) input_fail();
  }
  if(!env->is_op && is_alpha_num(input_peek(input))) input_fail();

  *rcr += rc;
  return node_new(env->type, 0, (free_f)nop_free);
}

// -- EOF_PARSER ------------------------

node_t *parse_eof(void *env, input_t *input, ulong *rcr) {
  ulong rc = 0;
  char cc = 0;
  
  rc += input_skip(input);
  
  if(input_move() != 0) input_fail();
  
  *rcr += rc;
  return node_new(EOF_NODE, 0, (free_f)nop_free);
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

comb_t *match_eof() {
  comb_t *res = comb_new();
  res->type = COMB_JUST;
  res->parse = parse_eof;
  return res;
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

comb_t *expect(comb_t *this, char *desc) {
  comb_t *res = comb_new();
  res->type = COMB_EXPECT;
  res->exp  = this;
  res->desc = desc;
  return res;
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
void var_decl_emit(node_t *this, output_t *out);
void var_emit(node_t *this, output_t *out);
void var_def_emit(node_t *this, output_t *out);
void struct_decl_emit(node_t *this, output_t *out);
void struct_emit(node_t *this, output_t *out);
void fun_decl_emit(node_t *this, output_t *out);
void fun_emit(node_t *this, output_t *out);
void stm_emit(node_t *this, output_t *out);
void exp_emit(node_t *this, output_t *out);
// --  ----------------------------------
#define PTR_NODE          1
#define VAR_DEF_NODE      2
#define VAR_DEF_LIST_NODE 3
#define STRUCT_DECL_NODE  4
#define STRUCT_NODE       5
#define VAR_DECL_NODE     6
#define VAR_NODE          7
#define VAR_LIST_NODE     8
#define PARAM_LIST_NODE   9
#define FUN_DECL_NODE     10
#define FUN_NODE          11

// TYPES
#define ID_TYPE_NODE      12
#define PTR_TYPE_NODE     13
#define FUN_TYPE_NODE     14
#define ARR_TYPE_NODE     15
#define TYPE_LIST_NODE    16

// STATEMENTS
#define EXP_STM_NODE      17
#define JMP_STM_NODE      18
#define JMP_CON_STM_NODE  19
#define LABEL_STM_NODE    20
#define RET_STM_NODE      21  
#define STM_LIST_NODE     22

// EXPRESSIONS
#define INT_EXP_NODE      23
#define ID_EXP_NODE       24
#define FLOAT_EXP_NODE    25
#define STR_EXP_NODE      26
#define CALL_EXP_NODE     27
#define EXP_LIST_NODE     28

#define L_C_B_NODE        29
#define R_C_B_NODE        30
#define L_R_B_NODE        31
#define R_R_B_NODE        32
#define L_S_B_NODE        33
#define R_S_B_NODE        34
#define ARROW_NODE        35
#define COMMA_NODE        36
#define COLON_NODE        37
#define SEMICOLON_NODE    38
#define EQ_NODE           39
#define AS_NODE           40

#define JMP_NODE          41
#define RET_NODE          42
#define EXTERN_NODE       43

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

// -- VAR_DECL --------------------------

// VAR_DECL_NODE:
//  | extern
//  | VAR
//  | semicolon

void var_decl_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  stack_next(&stack);
  emit(out, "extern ");
  var_emit(stack_next(&stack), out);
  emit_line(out, ";");
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

// -- STRUCT_FORWARD_DECLARATION --------

// STRUCT_DECL_NODE:
//  | STR
//  | ;

void struct_decl_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  node_t *id_node = stack_next(&stack);
  emit(out, "typedef struct ");
  str_emit(id_node, out);
  emit(out, " ");
  str_emit(id_node, out);
  emit_line(out, ";");
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

// -- FUNCTION_FORWARD_DECLARATION ------

// FUN_DECL_NODE
//  | STR
//  | | (
//  | | | TYPE
//  | | | ...
//  | | ) 
//  | | -> 
//  | | TYPE
//  | | ;

void fun_decl_emit(node_t *this, output_t *out) {
  stack_t *stack = this->node;
  node_t *id_node = stack_next(&stack);
  stack_t *type_stack = node_unwrap(stack_next(&stack));
  stack_next(&type_stack);
  stack_t *param_stack = node_unwrap(stack_next(&type_stack));
  stack_next(&type_stack);
  stack_next(&type_stack);
  node_t *type = stack_next(&type_stack);
  type_emit_head(type, out);
  emit(out, " ");
  str_emit(id_node, out);
  emit(out, "(");
  node_t *param = stack_next(&param_stack);
  while(param) {
    type_emit(param, out);
    if((param = stack_next(&param_stack))) emit(out, ", ");
  }
  emit(out, ")");
  type_emit_tail(type, out);
  emit_line(out, ";");
}

// -- FUNCTION --------------------------

// FUN_NODE:
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

// VAR_DEF_NODE:
//  | VAR
//  | =
//  | EXP
//  | ;

void var_def_emit(node_t *this, output_t *out) {
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
  stack_t *var_def_stack = node_unwrap(stack_next(&stack));
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
  stack_emit(var_def_stack, out, (stack_emit_f)var_def_emit);
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
      emit_line(out, ";");
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
  comb_t *base_comb         = comb_new();
  comb_t *struct_decl_comb  = comb_new();
  comb_t *struct_comb       = comb_new();
  comb_t *fun_decl_comb     = comb_new();
  comb_t *fun_comb          = comb_new();
  comb_t *var_decl_comb     = comb_new();
  comb_t *var_comb          = comb_new();
  comb_t *var_list_comb     = comb_new();
  comb_t *var_def_comb      = comb_new();
  comb_t *var_def_list_comb = comb_new();
  comb_t *param_list_comb   = comb_new();
 
  // types
  comb_t *type_comb         = comb_new();
  comb_t *id_type_comb      = comb_new();
  comb_t *ptr_type_comb     = comb_new();
  comb_t *fun_type_comb     = comb_new();
  comb_t *arr_type_comb     = comb_new();
  comb_t *type_list_comb    = comb_new();
  
  // statements 
  comb_t *stm_comb          = comb_new();
  comb_t *stm_list_comb     = comb_new();
  comb_t *exp_stm_comb      = comb_new();
  comb_t *label_stm_comb    = comb_new();
  comb_t *jmp_con_stm_comb  = comb_new();
  comb_t *jmp_stm_comb      = comb_new();
  comb_t *ret_stm_comb      = comb_new();
  comb_t *eof_comb          = match_eof();
  
  // EXPRESSIONS
  comb_t *exp_comb          = comb_new();
  comb_t *int_exp_comb      = comb_new();
  comb_t *id_exp_comb       = comb_new();
  comb_t *str_exp_comb      = comb_new();
  comb_t *float_exp_comb    = comb_new();
  comb_t *exp_list_comb     = comb_new();
  comb_t *call_exp_comb     = comb_new();
  comb_t *dot_exp_comb      = comb_new();
  comb_t *arrow_exp_comb    = comb_new();

  // OPERATORS
  comb_t *l_c_b_o           = match_op("{", L_C_B_NODE);
  comb_t *r_c_b_o           = match_op("}", R_C_B_NODE);
  comb_t *l_r_b_o           = match_op("(", L_R_B_NODE);
  comb_t *r_r_b_o           = match_op(")", R_R_B_NODE);
  comb_t *l_s_b_o           = match_op("[", L_S_B_NODE);
  comb_t *r_s_b_o           = match_op("]", R_S_B_NODE);
  comb_t *arrow_o           = match_op("->", ARROW_NODE);
  comb_t *colon_o           = match_op(":", COLON_NODE);
  comb_t *semicolon_o       = match_op(";", SEMICOLON_NODE);
  comb_t *comma_o           = match_op(",", COMMA_NODE);
  comb_t *eq_o              = match_op("=", EQ_NODE);
  comb_t *as_o              = match_op("*", AS_NODE);
  
  // KEYWORDS
  comb_t *jmp_k             = match_key("jmp", JMP_NODE);
  comb_t *ret_k             = match_key("ret", RET_NODE);
  comb_t *extern_k          = match_key("extern", EXTERN_NODE);
  
  
  // COMBINATOR STACK
  stack_t *comb_stack = stack_from(base_comb, struct_comb, fun_comb, var_comb, 
                           var_list_comb, var_def_comb, var_def_list_comb, param_list_comb, 
                           type_comb, stm_comb, stm_list_comb, exp_comb, struct_decl_comb,
                           id_type_comb, ptr_type_comb, fun_type_comb, arr_type_comb,
                           exp_stm_comb, label_stm_comb, jmp_con_stm_comb, jmp_stm_comb, 
                           ret_stm_comb, int_exp_comb, id_exp_comb, str_exp_comb, 
                           float_exp_comb, exp_list_comb, call_exp_comb, dot_exp_comb, 
                           arrow_exp_comb, l_c_b_o, fun_decl_comb, eof_comb, extern_k,
                           r_c_b_o, l_r_b_o, r_r_b_o, arrow_o, 
                           colon_o, semicolon_o, comma_o, eq_o, jmp_k, ret_k ,0);
                           

#define share comb_share
#define MATCH_AND(comb, node_type, ...) \
  comb = match_and(comb, node_type, stack_from(__VA_ARGS__, 0));
#define MATCH_OR(comb, ...) \
  comb = match_or(comb, stack_from(__VA_ARGS__, 0));
#define MATCH_OPT(comb, node_type, elem, sep, sl) \
  comb = match_opt(comb, node_type, elem, sep, sl);
  
  MATCH_AND(var_decl_comb,                                   // ________________________
            VAR_DECL_NODE,                                   // - VARIABLE_DECLARATION -
            share(extern_k),                                 // extern
            expect(share(var_comb), "variable declaration"), // VAR
            expect(share(semicolon_o), ";"));                // ;

  MATCH_AND(var_comb,                                        // ____________
            VAR_NODE,                                        // - VARIABLE -
            match_id(),                                      // ID
            share(colon_o),                                  // :
            share(type_comb));                               // TYPE

  MATCH_OPT(var_list_comb,                                   // _________________
            VAR_LIST_NODE,                                   // - VARIABLE_LIST -
            share(var_comb),                                 // VAR
            share(semicolon_o),                              // ;
            1);

  MATCH_AND(var_def_comb,                                    // ______________________
            VAR_DEF_NODE,                                    // - VARIABLE_DEFINTION -
            share(var_comb),                                 // VAR
            share(eq_o),                                     // =
            share(exp_comb),                                 // EXP
            share(semicolon_o));                             // ;

  MATCH_OPT(var_def_list_comb,                               // ____________________________
            VAR_DEF_LIST_NODE,                               // - VARIABLE_DEFINITION_LIST -
            share(var_def_comb),                             // VAR_DEF
            0, 
            0);

  MATCH_OPT(param_list_comb,                                 // __________________
            PARAM_LIST_NODE,                                 // - PARAMETER_LIST -
            share(var_comb),                                 // VAR
            share(comma_o),                                  // ,
            0);

  MATCH_AND(struct_decl_comb,                                // ______________________
            STRUCT_DECL_NODE,                                // - STRUCT_DECLARATION -
            match_id(),                                      // ID
            share(semicolon_o));                             // ;

  MATCH_AND(struct_comb,                                     // __________
            STRUCT_NODE,                                     // - STRUCT -
            match_id(),                                      // ID
            share(l_c_b_o),                                  // {
            share(var_list_comb),                            // VAR_LIST
            expect(share(r_c_b_o), "}"));                    // }

  MATCH_AND(fun_decl_comb,                                   // ________________________
            FUN_DECL_NODE,                                   // - FUNCTION_DECLARATION -
            match_id(),                                      // ID
            share(fun_type_comb),                            // FUN_TYPE
            share(semicolon_o));                             // ;

  MATCH_AND(fun_comb,                                        // ____________
            FUN_NODE,                                        // - FUNCTION -
            match_id(),                                      // ID
            share(l_r_b_o),                                  // (
            share(param_list_comb),                          // PARAM_LIST
            share(r_r_b_o),                                  // )
            share(arrow_o),                                  // ->
            share(type_comb),                                // TYPE
            share(var_def_list_comb),                        // VAR_DEF_LIST
            share(l_c_b_o),                                  // {
            share(stm_list_comb),                            // STM_LIST
            share(r_c_b_o));                                 // }
  
  // TYPES
  MATCH_OR(type_comb,                                        // - TYPE -
           share(arr_type_comb),                             // | ARR_TYPE 
           share(fun_type_comb),                             // | FUN_TYPE
           share(ptr_type_comb),                             // | PTR_TYPE
           share(id_type_comb));                             // | ID_TYPE

  MATCH_OPT(type_list_comb,                                  // _____________
            TYPE_LIST_NODE,                                  // - TYPE_LIST -
            share(type_comb),                                // TYPE
            share(comma_o),                                  // ,
            0);

  MATCH_AND(id_type_comb,                                    // ___________
            ID_TYPE_NODE,                                    // - ID_TYPE -
            match_id());                                     // ID

  MATCH_AND(ptr_type_comb,                                   // ________________
            PTR_TYPE_NODE,                                   // - POINTER_TYPE -
            share(as_o),                                     // *
            expect(share(type_comb), "type"));               // TYPE

  MATCH_AND(fun_type_comb,                                   // _________________
            FUN_TYPE_NODE,                                   // - FUNCTION_TYPE -
            share(l_r_b_o),                                  // (
            share(type_list_comb),                           // TYPE_LIST
            share(r_r_b_o),                                  // )
            share(arrow_o),                                  // ->
            share(type_comb));                               // TYPE

  MATCH_AND(arr_type_comb,                                   // ______________
            ARR_TYPE_NODE,                                   // - ARRAY_TYPE -
            share(l_s_b_o),                                  // [
            expect(share(type_comb), "type"),                // TYPE
            expect(share(semicolon_o), ";"),                 // ;
            expect(share(exp_comb), "expression"),           // EXP
            expect(share(r_s_b_o), "]"));                    // ]

  // STATEMENTS
  MATCH_OR(stm_comb,                                         // - STATEMENT -
           share(semicolon_o),                               // | ;
           share(exp_stm_comb),                              // | EXP_STM
           share(label_stm_comb),                            // | LABEL_STM
           share(jmp_stm_comb),                              // | JMP_STM
           share(jmp_con_stm_comb),                          // | JMP_CON_STM
           share(ret_stm_comb));                             // | RET_STM

  MATCH_OPT(stm_list_comb,                                   // __________________
            STM_LIST_NODE,                                   // - STATEMENT_LIST -
            share(stm_comb),                                 // STM
            0, 
            0);

  MATCH_AND(exp_stm_comb,                                    // ________________________
            EXP_STM_NODE,                                    // - EXPRESSION_STATEMENT -
            share(exp_comb),                                 // EXP
            share(semicolon_o));                             // ;

  MATCH_AND(label_stm_comb,                                  // ___________________
            LABEL_STM_NODE,                                  // - LABEL_STATEMENT -
            match_id(),                                      // ID
            share(colon_o));                                 // :

  MATCH_AND(jmp_con_stm_comb,                                // ____________________________
            JMP_CON_STM_NODE,                                // - JUMP_CONDITION_STATEMENT -
            share(jmp_k),                                    // jmp
            share(exp_comb),                                 // EXP
            match_id(),                                      // ID
            share(semicolon_o));                             // ;

  MATCH_AND(jmp_stm_comb,                                    // __________________
            JMP_STM_NODE,                                    // - JUMP_STATEMENT -
            share(jmp_k),                                    // jmp
            match_id(),                                      // ID
            share(semicolon_o));                             // ;

  MATCH_AND(ret_stm_comb,                                    // ____________________
            RET_STM_NODE,                                    // - RETURN_STATEMENT -
            share(ret_k),                                    // ret
            share(exp_comb),                                 // EXP
            share(semicolon_o));                             // ;

  // EXPRESSIONS
  MATCH_OR(exp_comb,                                         // - EXPRESSION -
           share(int_exp_comb),                              // | INT_EXP
           share(id_exp_comb),                               // | ID_EXP
           share(str_exp_comb),                              // | STR_EXP
           share(float_exp_comb),                            // | FLOAT_EXP
           share(call_exp_comb));                            // | CALL_EXP

  MATCH_OPT(exp_list_comb,                                   // ___________________
            EXP_LIST_NODE,                                   // - EXPRESSION_LIST -
            share(exp_comb),                                 // EXP
            0, 
            0);

  MATCH_AND(int_exp_comb,                                    // ______________________
            INT_EXP_NODE,                                    // - INTEGER_EXPRESSION -
            match_int());                                    // INT

  MATCH_AND(id_exp_comb,                                     // _________________________
            ID_EXP_NODE,                                     // - IDENTIFIER_EXPRESSION -
            match_id());                                     // ID

  MATCH_AND(str_exp_comb,                                    // _____________________
            STR_EXP_NODE,                                    // - STRING_EXPRESSION -
            match_str());                                    // STR

  MATCH_AND(float_exp_comb,                                  // ____________________
            FLOAT_EXP_NODE,                                  // - FLOAT_EXPRESSION -
            match_float());                                  // FLOAT

  MATCH_AND(call_exp_comb,                                   // ___________________
            CALL_EXP_NODE,                                   // - CALL_EXPRESSION -
            share(l_r_b_o),                                  // (
            share(exp_list_comb),                            // EXP_LIST
            share(r_r_b_o));                                 // )
  
  // BASEE_COMBINATOR
  MATCH_OR(base_comb,                                        // - [ BASE_COMBINATOR ] -
           share(struct_decl_comb),                          // | STRUCT_DECL
           share(struct_comb),                               // | STRUCT
           share(var_def_comb),                              // | VAR_DEF
           share(var_decl_comb),                             // | VAR_DECL
           share(fun_decl_comb),                             // | FUN_DECL
           share(fun_comb),                                  // | FUN
           share(eof_comb));                                 // | EOF
  
#undef COMB_AND
#undef COMB_OR
#undef COMB_OPT
#undef share

  return parser_new(input, comb_share(base_comb), comb_stack);
}

//---------------------------------------
// FILE_PREFIX 
//---------------------------------------

const char *file_prefix =                 "\
#define set(lexp, rexp)  (lexp = rexp)   \n\
#define ref(exp)         (&exp)          \n\
#define deref(exp)       (*exp)          \n\
#define get(lexp, rexp)  (lexp.rexp)     \n\
#define pget(lexp, rexp) (lexp->rexp)    \n\
#define aget(exp, index) (exp[index])    \n\
#define cast(exp, type)  ((type)exp)     \n\
#define size(exp)        (sizeof(exp))   \n\
#define lst(...)         ( __VA_ARGS__ ) \n\
#define init(...)        { __VA_ARGS__ } \n\
// UNARY_OPERATORS                       \n\
#define inc(exp)         (exp++)         \n\
#define dec(exp)         (exp--)         \n\
#define pos(exp)         (+exp)          \n\
#define neg(exp)         (-exp)          \n\
#define bnot(exp)        (~exp)          \n\
#define not(exp)         (!exp)          \n\
// BINARY_OPERATORS                      \n\
#define add(lexp, rexp)  (lexp + rexp)   \n\
#define sub(lexp, rexp)  (lexp - rexp)   \n\
#define mul(lexp, rexp)  (lexp * rexp)   \n\
#define div(lexp, rexp)  (lexp / rexp)   \n\
#define and(lexp, rexp)  (lexp && rexp)  \n\
#define or(lexp, rexp)   (lexp || rexp)  \n\
#define mod(lexp, rexp)  (lexp % rexp)   \n\
#define lt(lexp, rexp)   (lexp < rexp)   \n\
#define gt(lexp, rexp)   (lexp > rexp)   \n\
#define eq(lexp, rexp)   (lexp == rexp)  \n\
#define leq(lexp, rexp)  (lexp <= rexp)  \n\
#define geq(lexp, rexp)  (lexp >= rexp)  \n\
#define band(lexp, rexp) (lexp & rexp)   \n\
#define bor(lexp, rexp)  (lexp | rexp)   \n\
#define bxor(lexp, rexp) (lexp ^ rexp)   \n\
#define ls(lexp, rexp)   (lexp << rexp)  \n\
#define rs(lexp, rexp)   (lexp >> rexp)  \n\
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
    if(!node) {
      error("unable to parse complete input");
      break;
    }
    switch(node->type) {
      case STRUCT_NODE: {
        log("parsed struct");
        struct_emit(node, output);
        break;
      }
      case FUN_NODE: {
        log("parsed function");
        fun_emit(node, output);
        break;
      }
      case STRUCT_DECL_NODE: {
        log("parsed struct forward declaration");
        struct_decl_emit(node, output);
        break;
      }
      case VAR_DEF_NODE: {
        log("parsed variable definition");
        var_def_emit(node, output);
        break;
      }
      case FUN_DECL_NODE: {
        log("parsed function declaration");
        fun_decl_emit(node, output);
        break;
      }
      case VAR_DECL_NODE: {
        log("parsed variable declaration");
        var_decl_emit(node, output);
        break;
      }
      case EOF_NODE: {
        printf("done!\n");
        goto cleanup;
      }
      default:
        panic("parsed undefined node");
    }
    node_free(node);
  }

cleanup:  
  // cleanup
  parser_free(parser);
  output_free(output);
  
  return 0;
}
