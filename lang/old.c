#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/* -------- DEFINITIONS -------- */

#define MAX_STR_LEN 1024

#define error(...) {             \
  fprintf(stderr, "|Error| - "); \
  fprintf(stderr, __VA_ARGS__);  \
}

#define log(...) {               \
  fprintf(stdout, "|Log| - ");  \
  fprintf(stdout, __VA_ARGS__); \
  fprintf(stdout, "\n");        \
}

void panic(char *msg) {
  fprintf(stderr, "|Unexpected Error| - %s\n", msg);
  exit(-1);
}

//---------------------------------------
// UTILITY 
//---------------------------------------

char *str_new(char *str) {
  char *res = malloc(strlen(str) + 1);
  strcpy(res, str);
  return res;
}

//---------------------------------------
// STACK 
//---------------------------------------

struct stack_t {
  void           *obj;
  struct stack_t *next;
};

struct stack_t *stack_new(void *obj) {
  struct stack_t *res = malloc(sizeof(struct stack_t));
  res->obj  = obj;
  res->next = 0;
  return res;
}

void stack_push(struct stack_t **this, void *obj) {
  if(!*this) {
    *this = stack_new(obj);
    return;
  }
  struct stack_t *s = malloc(sizeof(struct stack_t));
  s->obj  = obj;
  s->next = *this;
  *this = s;
}

void *stack_pop(struct stack_t **this) {
  if(!*this) return 0;
  struct stack_t *f = *this;
  void *res = (*this)->obj;
  *this = (*this)->next;
  free(f);
  return res;
}

void stack_inverse(struct stack_t **this) {
  struct stack_t *prev = *this;
  struct stack_t *curr = (*this)->next;
  prev->next = 0;
  for(; curr; curr = curr->next) {
    curr->next = prev;
    prev = curr;
  }
  *this = prev;
}

#define stack_free(stack, free_fun) {                          \
  for(void *obj = 0; (obj = stack_pop(stack)); free_fun(obj)); \
}

// this macro also destroyes the stack
#define stack_for_each(stack, block) {                    \
  for(void *obj = 0; (obj = stack_pop(stack));) { block } \
}

//---------------------------------------
// TOKEN
//---------------------------------------

#define KEYWORD_LIST      \
  X(BREAK,    "break")    \
  X(CASE,     "case")     \
  X(CONST,    "const")    \
  X(CONTINUE, "continue") \
  X(ELSE,     "else")     \
  X(ELIF,     "elif")     \
  X(IF,       "if")       \
  X(WHILE,    "while")    \
  X(SIZEOF,   "sizeof")   \
  X(I8,       "i8")       \
  X(U8,       "u8")       \
  X(I16,      "i16")      \
  X(U16,      "u16")      \
  X(I32,      "i32")      \
  X(U32,      "u32")      \
  X(I64,      "i64")      \
  X(U64,      "u64")      \
  X(F32,      "f32")      \
  X(F64,      "f64")      \
  X(VOID,     "void") 

#define OP_LIST          \
  X(ELLIPSIS,     "...") \
  X(RIGHT_ASSIGN, ">>=") \
  X(LEFT_ASSIGN,  "<<=") \
  X(ADD_ASSIGN,   "+=")  \
  X(SUB_ASSIGN,   "-=")  \
  X(MUL_ASSIGN,   "*=")  \
  X(DIV_ASSIGN,   "/=")  \
  X(MOD_ASSIGN,   "%=")  \
  X(AND_ASSIGN,   "&=")  \
  X(XOR_ASSIGN,   "^=")  \
  X(OR_ASSIGN,    "|=")  \
  X(RIGHT_OP,     ">>")  \
  X(LEFT_OP,      "<<")  \
  X(INC_OP,       "++")  \
  X(DEC_OP,       "--")  \
  X(ARROW,        "->")  \
  X(AND_OP,       "&&")  \
  X(OR_OP,        "||")  \
  X(LE_OP,        "<=")  \
  X(GE_OP,        ">=")  \
  X(EQ_OP,        "==")  \
  X(NE_OP,        "!=")  \
  X(SEMICOLON,    ";")   \
  X(L_C_B,        "{")   \
  X(R_C_B,        "}")   \
  X(L_R_B,        "(")   \
  X(R_R_B,        ")")   \
  X(L_S_B,        "[")   \
  X(R_S_B,        "]")   \
  X(COMMA,        ",")   \
  X(COLON,        ":")   \
  X(EQUALS,       "=")   \
  X(DOT,          ".")   \
  X(AND,          "&")   \
  X(NOT,          "!")   \
  X(BIT_NOT,      "~")   \
  X(PLUS,         "+")   \
  X(MINUS,        "-")   \
  X(ASTERIX,      "*")   \
  X(DIV,          "/")   \
  X(MOD,          "%")   \
  X(LESS,         "<")   \
  X(GREATER,      ">")   \
  X(BIT_XOR,      "^")   \
  X(BIT_OR,       "|")   \
  X(QUESTION,     "?")

enum token_type_e {
  ID,
  INTEGER,
  FLOAT,
  STRING,

#define X(tok, val) tok, 
KEYWORD_LIST
OP_LIST
#undef X
};

struct token_t {
  enum token_type_e token_type;
  union {
    char   *str_val;
    int    int_val;
    double float_val;
  };
};

struct token_t *token_new(enum token_type_e type) {
  struct token_t *res = malloc(sizeof(struct token_t));
  res->token_type = type;
  res->str_val    = 0;
  res->int_val    = 0;
  res->float_val  = 0;
  return res;
}

void token_free(struct token_t *this) {
  if(!this) return;
  if(this->token_type == ID || this->token_type == STRING) {
    free(this->str_val);
  }
  free(this);
}

struct token_t *token_new_str(enum token_type_e type, char *str) {
  struct token_t *res = token_new(type);
  res->str_val = str;
  return res;
}

struct token_t *token_new_int(enum token_type_e type, int val) {
  struct token_t *res = token_new(type);
  res->int_val = val;
  return res;
}

struct token_t *token_new_float(enum token_type_e type, float val) {
  struct token_t *res = token_new(type);
  res->float_val = val;
  return res;
}

struct token_t *token_copy(struct token_t *this) {
  struct token_t *res = malloc(sizeof(struct token_t));
  memcpy(res, this, sizeof(struct token_t));
  if(this->token_type == ID || this->token_type == STRING) {
    res->str_val = malloc(strlen(this->str_val));
    strcpy(res->str_val, this->str_val);
  }
  return res;
}

void token_print(struct token_t *this) {
  printf("TOKEN|\n");
#define X(tok, val) if(tok == this->token_type) printf("type: %s\n", val);
  OP_LIST
  KEYWORD_LIST
#undef X
  switch(this->token_type) {
    case ID: 
      printf("type: id\nval: %s\n", this->str_val);
      break;
    case STRING: 
      printf("type: string\nval: %s\n", this->str_val);
      break;
    case INTEGER: 
      printf("type: integer\nval: %d\n", this->int_val);
      break;
    case FLOAT: 
      printf("type: float\nval: %f\n", this->float_val);
      break;
    default:
      break;
  }
}
 
//---------------------------------------
// LEXER
//---------------------------------------

struct lexer_t {
  FILE *in;
  
  struct token_t *current;
  
  // all read tokens are stored on stack
  struct stack_t *stack;
  // all rewinded tokens are stored on from
  struct stack_t *from;
};

// -- LEXER_UTIL ------------------------

char char_next(FILE *file) {
  int c = fgetc(file);
  if(c == EOF) {
    fclose(file);
    panic("end of file");
  }
  return (char)c;
}

char char_peek(FILE *file) {
  int c = fgetc(file);
  fseek(file, (long int)-1, SEEK_CUR);
  return (char)c;
}

void char_unget(FILE *file) {
  fseek(file, (long int)-1, SEEK_CUR);
}

int char_skip(FILE *file, char *set) {
  char c = 0;
  int count = -1;
  do { 
    c = char_next(file);
    count++;
    if(c == EOF) return -1;
  } while(strchr(set, c));
  char_unget(file);
  return count;
}

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

#define char_move(_in) (rc++, char_next(_in))

#define char_fail() {                 \
  fseek(in, (long int)-rc, SEEK_CUR); \
  return (struct token_t*)0;          \
}

// -- LEXER_FUNCTIONS -------------------

struct token_t *lexer_identifier(FILE *in) {
  int rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  if(!is_alpha(buffer[0] = char_move(in))) char_fail();
  size_t i = 1;
  for(;is_alpha_num(buffer[i] = char_move(in)); i++) {
    if(i >= MAX_STR_LEN) panic("identifier string too long");
  }
  char_unget(in);
  buffer[i] = 0;
  return token_new_str(ID, str_new(buffer));
}

struct token_t *lexer_integer(FILE *in) {
  int rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  size_t i = 0;
  if(!is_num(char_peek(in))) char_fail();
  for(;is_num(buffer[i] = char_move(in)); i++) {
    if(i >= MAX_STR_LEN) panic("integer string too long");
  }
  if(strchr(".f", buffer[i])) char_fail(); // TODO: what if: 123fid
  char_unget(in);
  return token_new_int(INTEGER, strtol(buffer, 0, 10));
}

struct token_t *lexer_float(FILE *in) {
  int rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  size_t i = 0;
  if(!is_num(char_peek(in))) char_fail();
  for(; is_num(buffer[i] = char_move(in)); i++) {
    if(i >= MAX_STR_LEN) panic("float string too long");
  }
  if(buffer[i] == 'f') {
    return token_new_float(FLOAT, strtod(buffer, 0));
  } else if(buffer[i] == '.') {
    for(i++; is_num(buffer[i] = char_move(in)); i++) {
      if(i >= MAX_STR_LEN) panic("float string too long");
    }
    char_unget(in);
    return token_new_float(FLOAT, strtod(buffer, 0));
  }
  char_fail();
}

struct token_t *lexer_string(FILE *in) {
  int rc = 0;
  char buffer[MAX_STR_LEN] = { 0 };
  if(char_move(in) != '\"') char_fail();
  size_t i = 0;
  for(;is_str(buffer[i] = char_move(in)); i++) {
    if(i >= MAX_STR_LEN) panic("string string too long");
    if(buffer[i] == '\"' && (i == 0 || buffer[i - 1] != '\\')) break;
  }
  if(buffer[i] != '\"') panic("string not properly ended (missing \")");
  buffer[i] = 0;
  return token_new_str(STRING, str_new(buffer));
}

struct token_t *lexer_keyword(FILE *in, int is_op, char *key, enum token_type_e type) {
  int rc = 0;
  for(size_t i = 0; key[i]; i++) {
    if(char_move(in) != key[i]) char_fail();
  }
  if(!is_op && is_alpha_num(char_peek(in))) char_fail();
  return token_new(type);
}

// --  ----------------------------------

void lexer_crt(struct lexer_t *this, FILE *in) {
  this->in      = in;
  this->current = 0;
  this->stack   = 0;
  this->from    = 0;
}

struct token_t *lexer_current(struct lexer_t *this) {
  return this->current;
}

// returns 0 if no token can be found anymore
struct token_t *lexer_next(struct lexer_t *this) {
  struct token_t *res = 0;
#define set_res() {              \
  stack_push(&this->stack, res); \
  this->current = res;           \
  return res;                    \
}
  
  // return 0 on eof
  if(char_peek(this->in) == EOF) return (struct token_t*)0;

  // if token is already on from stack
  // return it immediately
  if((res = stack_pop(&this->from))) set_res();
  
  // ignore all whitespaces, newlines, tabs, etc.
  char *skip_set = " \n\r\t";
  if(char_skip(this->in, skip_set) == -1) return (struct token_t*)0;

  // parser all tokens in order 
  // operators -> keywords -> identifier -> integer -> float -> string
#define X(tok, val) if((res = lexer_keyword(this->in, 1, val, tok))) set_res();
  OP_LIST
#undef X
#define X(tok, val) if((res = lexer_keyword(this->in, 0, val, tok))) set_res();
  KEYWORD_LIST
#undef X
  if((res = lexer_identifier(this->in))) set_res();
  if((res = lexer_integer(this->in)))    set_res();
  if((res = lexer_float(this->in)))      set_res();
  if((res = lexer_string(this->in)))     set_res();

  return (struct token_t*)0;
}

// consumes the next token
void lexer_consume(struct lexer_t *this) {
  lexer_next(this);
  struct token_t *token = stack_pop(&this->stack);
  if(!token) panic("token missing from stack");
  token_free(token);
}

// resets n tokens from stack to from
void lexer_rewind(struct lexer_t *this, size_t n) {
  void *obj = 0;
  for(size_t i = 0; i < n; i++) {
    obj = stack_pop(&this->stack);
    if(!obj) return;
    stack_push(&this->from, obj);
  }
}

struct token_t *lexer_peek(struct lexer_t *this) {
  struct token_t *res = lexer_next(this);
  lexer_rewind(this, 1);
  return res;
}

void lexer_clear(struct lexer_t *this) {
  stack_free(&this->stack, token_free);
}

//---------------------------------------
// PARSER 
//---------------------------------------

struct parser_t {
  struct lexer_t;
};

//---------------------------------------
// PARSER_TYPES
//---------------------------------------

//---------------------------------------
// EMIT 
//---------------------------------------

#define EOL "\r\n"

#define emit(out, str) {   \
  fprintf(out, "%s", str); \
}

#define femit(out, ...) {    \
  fprintf(out, __VA_ARGS__); \
}

#define emit_line(out, ...) { \
  fprintf(out, __VA_ARGS__);  \
  fprintf(out, EOL);          \
}



//---------------------------------------
// MAIN_PROGRAMM 
//---------------------------------------

int main(int argc, char **argv) {
  printf("+---------------------------+\n");
  printf("| Starting SC-Lang Compiler |\n");
  printf("| Author:  Gerrit Proessl   |\n");
  printf("| Version: 0.0.1            |\n");
  printf("+---------------------------+\n");
  
  FILE *in = 0;
  FILE *out = stdout;
  
  if(argc >= 3) {
    out = fopen(argv[2], "w");
    if(!out) panic("unable to open output file");
  }
 
  if(argc >= 2) {
    in = fopen(argv[1], "r");
    if(!in) panic("unable to open input file");
    parse(in, stdout);
    fclose(in);
    if(out != stdout) fclose(out);
  } else {
    panic("no input or output file specified");
  }

  return 0;
}
