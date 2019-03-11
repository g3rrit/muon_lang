#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define error(...) {             \
  fprintf(stderr, "|Error| - "); \
  fprintf(stderr, __VA_ARGS__);  \
}

void panic(char *msg) {
  fprintf(stderr, "|Unexpected Error| - %s\n", msg);
  exit(-1);
}

//---------------------------------------
// UTILITY 
//---------------------------------------

void emit(FILE *out, char *str) {
  fprintf(out, "%s", str);
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
  if(!this) return 0;
  struct stack_t *f = *this;
  void *res = (*this)->obj;
  *this = (*this)->next;
  free(f);
  return res;
}

//---------------------------------------
// OBJ
//---------------------------------------

//---------------------------------------
// TOKEN
//---------------------------------------

enum token_type_e {
  ID,
  INTEGER,
  FLOAT,
  IF,
  WHILE,
  VOID
};

struct token_t {
  enum token_type_e token_type;
  union {
    char   *str_val;
    int    int_val;
    double float_val;
  };
};
 
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

/* --------  LEXER_UTIL -------- */

char char_next(FILE *file) {
  int c  = fgetc(file);
  if(c == EOF) {
    fclose(file);
    panic("end of file");
  }
  return (char)c;
}

char char_peek(FILE *file) {
  int c = fgetc(file);
  fseek(int, (long int)-1, SEEK_CUR);
  return (char)c;
}

#define char_move() (rc++, char_next())

#define char_fail() {                 \
  fseek(in, (long int)-rc, SEEK_CUR); \
  return (struct token_t*)0;          \
}

#define char_peek()

// checks if char is 0 .. 9
#define is_num(_c) (_c >= '0' && _c <= '9')

// checks if char is a .. z | A .. Z | _
#define is_alpha(_c) ((_c >= 'a' && _c <= 'z') || (_c >= 'A' && _c <= 'Z') || _c == '_')

// checks if char is a .. z | A .. Z | 0 .. 9 | _
#define is_alpha_num(_c) ((_c >= 'a' && _c <= 'z') || (_c >= 'A' && _c <= 'Z') || (_c >= '0' && _c <= '9') || _c == '_')

/* --------  LEXER_FUNCTIONS -------- */

struct token_t *lexer_if(FILE *in) {
  int rc = 0;
  int c  = 0;
  if(char_move() != 'i') char_fail();
  if(char_move() != 'f') char_fail();
  if(!is_alphanum(char_peek())) char_fail();
  //return new token
}

struct token_t *lexer_id(FILE *in) {
}

/* --------  --------------- -------- */

//array of lexer functions that get executed in order
struct token_t *(*token_lexers[])(FILE*) = {
  lexer_if, 
  lexer_id,
  0
};

struct lexer_t *lexer_new(FILE *in) {
  struct lexer_t *res = malloc(sizeof(struct lexer_t));
  res->in      = in;
  res->current = 0;
  res->stack   = 0;
  res->from    = 0;
  return res;
}

struct token_t *lexer_current(struct lexer_t *this) {
  return this->current;
}

struct token_t *lexer_next(struct lexer_t *this) {
  struct token_t *res = 0;
  // if token is already on from stack
  // return it immediately
  if(res = stack_pop(&this->from)) {
    stack_push(&this->stack, res);
    this->current = res;
    return res;
  }

  //otherwise parse all tokens in order
  struct token_t *(*token_lexer)(FILE*) = 0;
  for(size_t i = 0; token_lexer = token_lexers[i]; i++) {
    if(res = token_lexer(this->in)) {
      stack_push(&this->stack, res);
      this->current = res;
      return res;
    }
  }
  return (struct token_t*)0;
}

//---------------------------------------
// PARSER 
//---------------------------------------

struct parser_t {
  FILE *in;
  FILE *out;
  
  struct token_t *token;
};

void parse(FILE *in, FILE *out) {
}

//---------------------------------------
// MAIN PROGRAMM 
//---------------------------------------

int main(void) {
  printf("| Starting SC-Lang Compiler |\n");
  printf("| Author:  Gerrit Proessl   |\n");
  printf("| Version: 0.0.1            |\n");
 
  return 0;
}
