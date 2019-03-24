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
// OUTPUT 
//---------------------------------------

typedef struct output_t {
  FILE *file;
} output_t;

output_t *output_new(FILE *file) {
  output_t *res = alloc(sizeof(output_t));
  res->file = file;
  return res;
}

void output_free(output_t *this) {
  if(!this) return;
  if(!fclose(this->file) == EOF) error("unable to close output stream");
  free(this);
}

void emit(output_t *this, char *str) {
  fprintf(this->file, "%s", str);
}

void emit_line(output_t *this, char *str) {
  fprintf(this->file, "%s\n", str);
}

void femit(output_t *this, char *fmt, ...) {
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

node_t *node_stack_fold(stack_t *stack) {
  return node_new(STACK_NODE, stack, (free_f)node_stack_free);
}

//---------------------------------------
// PRIMATIVE_NODE_TYPES
//---------------------------------------

// -- STRING ----------------------------

typedef void (*free_f)(void*);

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
  femit(out, "%lf", this->val);
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
      node_t *(*fold)(stack_t*); // -- AND 
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
} comb_t;

comb_t *comb_new() {
  comb_t *res = alloc(sizeof(comb_t));
  memset(res, 0, sizeof(comb_t));
  return res;
}

void comb_free(comb_t *this) {
  if(!this) return;
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
        stack_free(&stack, node_free);
        input_rewind(input, rc);
        rc = 0;
        break;
      }
      stack_push(&stack, res);
    }
    stack_inverse(&stack);
    res = this->fold(stack);
  } else if(this->type == COMB_OPT) {
    if(!(res = comb_parse(input, this->elem, &rc))) {
      input_rewind(input, rc); // prob not necessary
      rc = 0;
    } else {
      stack_push(&stack, res);
      while(comb_parse(input, this->sep, &rc)) {
        if(!(res = comb_parse(input, this->elem, &rc))) {
          stack_free(&stack, node_free);
          input_rewind(input, rc);
          rc = 0;
          break;
        }
        stack_push(&stack, res);
      }
      if(this->sl && !comb_parse(input, this->sep, &rc)) {
        stack_free(&stack, node_free);
        input_rewind(input, rc);
        rc = 0;
      }
      stack_inverse(&stack);
      res = node_new(STACK_NODE, stack, (free_f)node_stack_free);
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
  return (node_t*)0;  \
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
  input_rewind(input, 0);
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

comb_t *match_and(comb_t *this, stack_t *stack, node_t *(*fold)(stack_t*)) {
  if(this->type != COMB_NONE) error("combinator already defined");
  this->type = COMB_AND;
  this->stack = stack;
  if(!fold) {
    fold = node_stack_fold;
  }
  this->fold  = fold;
  return this;
}

comb_t *comb_add(comb_t *this, stack_t *stack) {
  if(this->type == COMB_JUST) panic("unable to add to \'JUST\' combinator");
  stack_inverse(&this->stack);
  for(void *obj = stack_pop(&stack); obj; (obj = stack_pop(&stack))) {
    stack_push(&this->stack, obj);
  }
  stack_inverse(&this->stack);
  return this;
}

comb_t *match_list(comb_t *this, comb_t *elem, comb_t *sep, int sl) {
  if(this->type != COMB_NONE) error("combinator already defined");
  this->type = COMB_OPT;
  this->elem = elem;
  this->sep  = sep;
  this->sl   = sl;
  return this;
}

//---------------------------------------
//---------------------------------------

// -- UTIL ------------------------------

void strl_emit(str_t *this, output_t *out) {
  emit(out, "\"");
  str_emit(this, out);
  emit(out, "\"");
}

void charl_emit(char_t *this, output_t *out) {
  emit(out, "'");
  char_emit(this, out);
  emit(out, "'");
}

void stack_emit(stack_t *this, output_t *out, void (*emit_f)(void*, output_t*)) {
  for(void *obj = stack_next(&this); (obj = stack_next(&this))) {
    emit_f(obj, out);
  }
}

void char_emit(char_t *this, output_t *out) {
  femit(out, "%c",  this->val);
}

void int_emit(int_t *this, output_t *out) {
  femit(out, "%d", this->val);
}

void str_emit(str_t *this, output_t *out) {
  emit(out, this->val);
}

//---------------------------------------
// CUSTOM_NODE_TYPES
//---------------------------------------
// -- DECLARATIONS ----------------------
struct struct_t;
struct var_t;
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
#define STRUCT_NODE 13
#define VAR_NODE 14

#define L_C_B_NODE 50
#define R_C_B_NODE 51
#define L_R_B_NODE 52
#define R_R_B_NODE 53
#define L_S_B_NODE 54
#define R_S_B_NODE 55

#define COLON_NODE 56
#define SEMICOLON_NODE 57

// -- VARIABLE --------------------------

typedef struct var_t {
  str_t  *id;
  node_t *type;
} var_node_t;

void var_free(var_t *this) {
  if(!this) return;
  str_free(this->id);
  node_free(this->type);
  free(this);
}

void var_emit(var_t *this, output_t *out) {
  // TODO
  emit(out, "int ");
  str_emit(this->id, out);
}

node_t *var_fold(stack_t *stack) {
  var_t *res = alloc(sizeof(var_t));
  res->id = node_unwrap(stack_pop(&stack)); // id
  node_free(stack_pop(&stack));             // :
  res->type = stack_pop(&stack);            // type
  return node_new(VAR_NODE, res, (free_f)var_free);
}

// -- STRUCT ----------------------------

typedef struct struct_t {
  str_t   *id;
  stack_t *vars;
} struct_t;

void struct_free(struct_t *this) {
  if(!this) return;
  str_free(this->id);
  stack_free(&this->vars, var_free);
  free(this);
}

node_t *struct_fold(stack_t *stack) {
  struct_t *res = alloc(sizeof(stack_t));
  res->id = node_unwrap(stack_pop(&stack));
  res->vars = node_unwrap(stack_pop(&stack));
  return node_new(STRUCT_NODE, res, (free_f)stack_free);
}

void var_member_emit(node_t *this, output_t *out) {
  var_emit(this->node, out);
  emit(out, ";");
}

void struct_emit(struct_t *this, output_t *out) {
  emit(out, "struct ");
  str_emit(this->id, out);
  emit_line(out, "{");
  stack_emit(this->vars, out, var_member_emit);
  emit(out, "};");
}

// --  ----------------------------------

comb_t *base_comb() {
  comb_t *struct_comb   = comb_new();
  comb_t *var_comb      = comb_new();
  comb_t *var_list_comb = comb_new();
  comb_t *type_comb     = comb_new();

  // operators
  comb_t *l_c_b_o     = match_op("{", L_C_B_NODE);
  comb_t *r_c_b_o     = match_op("}", R_C_B_NODE);
  comb_t *colon_o     = match_op(":", COLON_NODE);
  comb_t *semicolon_o = match_op(";", SEMICOLON_NODE);
  
  // keywords
  comb_t *u8_k = match_key("u8", U8_NODE);
  
  type_comb     = match_or(type_comb, stack_from(match_id(), u8_k, 0));
  var_comb      = match_and(var_comb, stack_from(match_id(), colon_o, type_comb, 0), var_fold);
  var_list_comb = match_opt(var_list_comb, var_comb, semicolon_o, 1);
  struct_comb   = match_and(struct_comb, stack_from(match_id(), l_c_b_o, var_list_comb, r_c_b_o, 0), struct_fold);
  
  return struct_comb;
}

//---------------------------------------
//---------------------------------------


int main(int argc, char **argv) {
  if(argc < 2) panic("no input file specified");
  FILE *file = fopen(argv[1], "r");
  if(!file) panic("unable to open input file");
  
  parser_t *parser = parser_new(input_new(file), base_comb());
  struct_t *s = parse(parser);
  
  printf("finished parsing\n");
  
  parser_free(parser);
  
  return 0;
}
