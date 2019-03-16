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

void stack_inverse(struct stack **this) {
  struct stack_t *prev = *this;
  struct stack_t *curr = *this->next;
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
  X(I8),      "i8")       \
  X(U8),      "u8")       \
  X(I16),     "i16")      \
  X(U16),     "u16")      \
  X(I32),     "i32")      \
  X(U32),     "u32")      \
  X(I64),     "i64")      \
  X(U64),     "u64")      \
  X(F32),     "f32")      \
  X(F64),     "f64")      \
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
  X(PTR_OP,       "->")  \
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
// EMIT_UTIL 
//---------------------------------------

#define EOL "\r\n"

#define emit(out, ...) {     \
  fprintf(out, __VA_ARGS__); \
}

#define emit_line(out, ...) { \
  fprintf(out, __VA_ARGS__);  \
  fprintf(out, EOL);          \
}

//---------------------------------------
// PARSER 
//---------------------------------------

struct parser_t {
  FILE *in;
  FILE *out;
  
  struct lexer_t lexer;
};

// PARSER FUNCTION
// parser: type *parse_type(struct parser_t *parser, int *rcr { 
//           int rc = 0;
//           ...
//           *rcr += rc;
//           return type_new(...);
//         } 
//
// type:   type *type_new(args..);               -- returns new variable
//         void type_free(type *this);           -- destructs type
//         void type_emit(type *this, FILE* out) -- emits type and destructs it

// cosumes and sets next token and return bool
#define is_token(var, tok) \
  (rc++, (var = lexer_next(&parser->lexer))->token_type == tok)

// rewinds all tokens since parser function started
#define parser_fail() {             \
  lexer_rewind(&parser->lexer, rc); \
  return 0;                         \
}
 
// checks, consumes and sets next token. executes on fail on falure
#define set_next_token(var, tok, on_fail) {                   \
  rc++;                                                       \
  if((var = lexer_next(&parser->lexer))->token_type != tok) { \
    on_fail                                                   \
    lexer_rewind(&parser->lexer, rc);                         \
    return 0;                                                 \
  }                                                           \
}

// consumes next token. executes on fail on failure
#define next_token(tok, on_fail) {                    \
  rc++;                                               \
  if(lexer_next(&parser->lexer)->token_type != tok) { \
    on_fail                                           \
    lexer_rewind(&parser->lexer, rc);                 \
    return 0;                                         \
  }                                                   \
}

// checks next token without consuming it
#define token_peek(tok) (lexer_peek(&parser->lexer)->token_type == tok)

void parse(FILE *in, FILE *out) {
  struct parser_t parser;
  parser.in  = in;
  parser.out = out;
  lexer_crt(&parser.lexer, in);
  
  struct token_t *token = 0;
  for(;;) {
    if(!(token = lexer_next(&parser.lexer))) break;
    token_print(token);
  }
}

//---------------------------------------
// AST_TYPES
//---------------------------------------
// TYPE_FORWARD_DECLARATION:
//---------------------------------------
// -- STRUCT ----------------------------
struct struct_t
// -- VARIABLE --------------------------
struct var_t;
// -- FUNCTION --------------------------
struct function_t;
// -- TYPE ------------------------------
struct type_t;
// -- STATEMENT -------------------------
struct stm_t;
struct con_stm_t;
struct loop_stm_t;
struct var_stm_t;
struct arr_type_t;
struct fun_type_t;
// -- EXPRESSION ------------------------
struct exp_t;
struct bin_exp_t;
struct ter_exp_t;
struct cast_exp_t;
struct comp_exp_t;
//---------------------------------------
// TYPE_DECLARATION: 
//---------------------------------------
// -- STRUCT ----------------------------
struct struct_t {
  struct token_t *id;
  struct stack_t *vars; // <var_t>
};
// -- VARIABLE --------------------------
struct var_t {
  struct token_t *id;
  struct type_t  *type;
};
// -- FUNCTION --------------------------
struct function_t {
  struct token_t *id;
  struct type_t  *type;
  struct stack_t *params; // <var_t>
  struct stack_t *stms;   // <stm_t>
};
// -- TYPE ------------------------------
#define PTYPE_LIST                       \
  X(I8_T,   "int8_t")   /* empty      */ \
  X(U8_T,   "uint8_t")  /* empty      */ \
  X(I16_T,  "int16_t")  /* empty      */ \
  X(U16_T,  "uint16_t") /* empty      */ \
  X(I32_T,  "int32_t")  /* empty      */ \
  X(U32_T,  "uint32_t") /* empty      */ \
  X(I64_T,  "int64_t")  /* empty      */ \
  X(U64_T,  "uint64_t") /* empty      */ \
  X(F32_T,  "float")    /* empty      */ \
  X(F64_T   "double")   /* empty      */ \
  X(VOID_T, "void")     /* empty      */ 
#define CTYPE_LIST                       \
  X(ID_T,   "")         /* token_t    */ \
  X(ARR_T,  "")         /* arr_type_t */ \
  X(FUN_T,  "")         /* fun_type_t */ \

enum type_type_e {
#define X(tok, v) tok,
PTYPE_LIST
CTYPE_LIST
#undef X
};

struct type_t {
  enum type_type_e type_type;
  void             *type;
  size_t           ref_count;
};

struct arr_type_t {
  struct type_t *type;
  struct exp_t  *size;
};

struct fun_type_t {
  struct type_t  *type;
  struct stack_t *params; // <type_t>
};
// -- STATEMENT -------------------------
#define STM_LIST                            \
  X(CON_STM)     /* stack_t<guard_stm_t> */ \
  X(LOOP_STM)    /* guard_stm_t          */ \
  X(STRUCT_STM)  /* struct_t             */ \
  X(EXP_STM)     /* exp_t                */ \
  X(VAR_STM)     /* var_stm_t            */ \
  X(LST_STM)     /* stack_t<stm_t>       */ 

enum stm_type_e {
#define X(tok) tok,
STM_LIST
#undef X
};

struct stm_t {
  enum stm_type_e stm_type;
  void            *stm;
};

struct guard_stm_t {
  struct exp_t     *con; 
  struct lst_stm_t *stms;
};

struct var_stm_t {
  struct var_t *var;
  struct exp_t *val;
};
// -- EXPRESSION ------------------------
#define PEXP_LIST                                              \
  X(COMP_EXP,    "")    /* (type) { exp, ... } | comp_exp_t */ \
  X(INT_EXP,     "")    /* 10                  | token_t    */ \
  X(FLOAT_EXP,   "")    /* 10.0                | token_t    */ \
  X(STRING_EXP,  "")    /* "str"               | token_t    */ \
  X(ID_EXP,      "")    /* x                   | token_t    */ \
  X(SIZEOF_EXP,  "")    /* sizeof(type)        | type_t     */ \
  X(BRACKET_EXP, "")    /* ( exp )             | exp_t      */ \
  X(CAST_EXP,    "")    /* ( type ) exp        | cast_exp_t */ \
  X(CALL_EXP,    "")    /* exp ( exp, ... )    | stack_t    */ \
  X(ARR_EXP,     "")    /* exp [ exp ]         | bin_exp_t  */ \
  X(TER_EXP,     "")    /* exp ? exp : exp     | ter_exp_t  */ 
#define UEXP_LIST                                              \
  X(INC_EXP,     "++")  /* ++ exp              | exp_t      */ \
  X(DEC_EXP,     "--")  /* -- exp              | exp_t      */ \
  X(POS_EXP,     "+")   /* + exp               | exp_t      */ \
  X(MIN_EXP,     "-")   /* - exp               | exp_t      */ \
  X(NOT_EXP,     "!")   /* ! exp               | exp_t      */ \
  X(BIT_NOT_EXP, "~")   /* ~ exp               | exp_t      */ \
  X(DEREF_EXP,   "*")   /* * exp               | exp_t      */ \
  X(REF_EXP,     "&")   /* & exp               | exp_t      */ 
#define BEXP_LIST                                              \
  X(DOT_EX,      ".")   /* exp . exp           | bin_exp_t  */ \
  X(ARROW_EXP,   "->")  /* exp -> exp          | bin_exp_t  */ \
  X(MULT_EXP,    "*")   /* exp * exp           | bin_exp_t  */ \
  X(DIV_EXP,     "/")   /* exp / exp           | bin_exp_t  */ \
  X(MOD_EXP,     "%")   /* exp % exp           | bin_exp_t  */ \
  X(ADD_EXP,     "+")   /* exp + exp           | bin_exp_t  */ \
  X(SUB_EXP,     "-")   /* exp - exp           | bin_exp_t  */ \
  X(LS_EXP,      "<<")  /* exp << exp          | bin_exp_t  */ \
  X(RS_EXP,      ">>")  /* exp >> exp          | bin_exp_t  */ \
  X(LT_EXP,      "<")   /* exp < exp           | bin_exp_t  */ \
  X(GT_EXP,      ">")   /* exp > exp           | bin_exp_t  */ \
  X(LE_EXP,      "<=")  /* exp <= exp          | bin_exp_t  */ \
  X(GE_EXP,      ">=")  /* exp >= exp          | bin_exp_t  */ \
  X(EQ_EXP,      "==")  /* exp == exp          | bin_exp_t  */ \
  X(NE_EXP,      "!=")  /* exp != exp          | bin_exp_t  */ \
  X(BIT_AND_EXP, "&")   /* exp & exp           | bin_exp_t  */ \
  X(BIT_XOR_EXP, "^")   /* exp ^ exp           | bin_exp_t  */ \
  X(BIT_OR_EXP,  "|")   /* exp | exp           | bin_exp_t  */ \
  X(AND_EXP,     "&&")  /* exp && exp          | bin_exp_t  */ \
  X(OR_EXP,      "||")  /* exp || exp          | bin_exp_t  */ \
  X(ASG_EXP,     "=")   /* exp = exp           | bin_exp_t  */ \
  X(ADDA_EXP,    "+=")  /* exp += exp          | bin_exp_t  */ \
  X(SUBA_EXP,    "-=")  /* exp -= exp          | bin_exp_t  */ \
  X(MULTA_EXP,   "*=")  /* exp *= exp          | bin_exp_t  */ \
  X(DIVA_EXP,    "/=")  /* exp /= exp          | bin_exp_t  */ \
  X(MODA_EXP,    "%=")  /* exp %= exp          | bin_exp_t  */ \
  X(LSA_EXP,     "<<=") /* exp <<= exp         | bin_exp_t  */ \
  X(RSA_EXP,     ">>=") /* exp >>= exp         | bin_exp_t  */ \
  X(ANDA_EXP,    "&=")  /* exp &= exp          | bin_exp_t  */ \
  X(XORA_EXP,    "^=")  /* exp ^= exp          | bin_exp_t  */ \
  X(ORA_EXP,     "|=")  /* exp |= exp          | bin_exp_t  */ \
  X(COMMA_EXP,   ",")   /* exp , exp           | bin_exp_t  */ 
  
enum exp_type_e {
#define X(tok, v) tok,
PEXP_LIST
UEXP_LIST
BEXP_LIST
#undef X
};

struct exp_t {
  enum exp_type_e exp_type;
  void            *exp;
};

struct bin_exp_t {
  struct exp_t *f;
  struct exp_t *s;
};

struct ter_exp_t {
  struct exp_t *f;
  struct exp_t *s;
  struct exp_t *t;
};

struct cast_exp_t {
  struct type_t *type;
  struct exp_t  *exp;
};

struct comp_exp_t {
  struct type_t  *type;
  struct stack_t *vals;
};
//---------------------------------------
// FUNCTION_DECLARATION: 
//---------------------------------------
// -- -- STRUCT -------------------------
struct struct_t*   struct_new(struct token_t*, struct stack_t*);
void               struct_free(struct struct_t*);
void               struct_emit(struct struct_t*, FILE*);
struct sturct_t*   parse_struct(struct parser_t*, int*);
// -- -- VARIABLE -----------------------
struct var_t*      var_new(struct token_t*, struct type_t*);
void               var_free(struct var_t*);
void               var_emit(struct var_t*, FILE*);
struct var_t*      parse_var(struct parser_t*, int*);
// -- -- FUNCTION -----------------------
struct function_t* function_new(struct token_t*, struct type_t*, struct stack_t*, struct stack_t*);
void               function_free(struct function_t*);
void               function_emit(struct function_t*, FILE*);
struct function_t* parse_function(struct parser_t*, int*);
// -- -- TYPE ---------------------------
struct type_t*     type_new(enum type_type_e, void*, size_t);
void               type_free(struct type_t*);
void               type_emit(struct type_t*);
void               type_emit_head(struct type_t*, FILE*);
void               type_emit_tail(struct type_t*, FILE*);
struct type_t*     parse_type(struct parser_t*, int*);
// -- -- STATEMENT ----------------------
struct stm_t;
struct con_stm_t;
struct loop_stm_t;
struct var_stm_t;
struct arr_type_t;
struct fun_type_t;
// -- -- EXPRESSION ---------------------
struct exp_t;
struct bin_exp_t;
struct ter_exp_t;
struct cast_exp_t;
struct comp_exp_t;

// --  ----------------------------------


// -- STRUCT ----------------------------

struct struct_t *struct_new(struct token_t *id, struct stack_t *vars) {
  struct struct_t *res = malloc(sizeof(struct struct_t));
  res->id   = id;
  res->vars = vars;
  return res;
}

void struct_free(struct struct_t *this) {
  if(!this) return;
  token_free(this->id);
  stack_free(&this->vars, var_free);
  free(this);
}

void struct_emit(struct struct_t *this, FILE *out) {
  emit_line(out, "struct %s {", this->id->str_val);
  stack_for_each(&this->vars, {
    var_emit(obj, out);
    emit_line(out, ";");
  });
  emit_line(out, "};");
  token_free(this->id);
  free(this);
}

struct struct_t *parse_struct(struct parser_t *parser, int *rcr) {
  int rc = 0;
  struct token_t *id = 0;
  struct stack_t *vars = 0;
  struct var_t   *var = 0;

#define VS_F stack_free(&vars, var_free);

  set_next_token(id, ID, {});
  next_token(L_C_B, {});
  while(!token_peek(R_C_B)) {
    if(!(var = var_parse(parser, &rc))) {
      VS_F
      parser_fail();
    }
    stack_push(&vars, var);
  }
  stack_inverse(&vars);
  next_token(R_C_B, { VS_F });

#undef VS_F
  
  *rcr += rc;
  return struct_new(token_copy(id), vars);
};

// -- VARIABLE --------------------------

struct var_new(struct token_t *id, struct type_t *type) {
  struct var_t *res = malloc(sizeof(struct var_t));
  res->id   = id;
  res->type = type;
  return res;
}

void var_free(struct var_t *this) {
  if(!this) return;
  token_free(this->id);
  type_free(this->type);
  free(this);
}

void var_emit(struct var_t *this, FILE *out) {
  type_emit_head(this->type, out);
  emit(out, " %s", this->id->str_val);
  type_emit_tail(this->type, out);
  token_free(this->id);
  free(this);
}

struct var_t *parse_var(struct parser_t *parser, int *rcr) {
  int rc = 0;
  struct token_t *id = 0;
  struct type_t  *type = 0;

  set_next_token(id, ID, {});
  next_token(COLON, {});
  if(!(type = parse_type(parser, &rc))) {
    parser_fail();
  }

  *rcr += rc;
  return var_new(token_copy(id), type); 
}

// -- FUNCTION --------------------------

struct function_t *function_new(struct token_t *id, struct type_t *type, struct stack_t *params, struct stack_t *stms) {
  struct function_t *res = malloc(sizeof(struct function_t));
  res->id     = id;
  res->type   = type;
  res->params = params;
  res->stms   = stms;
  return res;
}

void function_free(struct function_t *this) {
  if(!this) return;
  token_free(this->id);
  type_free(this->type);
  stack_free(this->params, var_free);
  stack_free(this->stms, stm_free);
  free(this);
}

void function_emit(struct function_t *this, FILE *out) {
  type_emit_head(this->type, out);
  emit(out, " %s(", this->id->str_val);
  stack_for_each(&this->params, {
    var_emit(obj, out);
    emit(out, ", ");
  });
  emit(out, ")");
  type_emit_tail(this->type, out);
  stm_lst_emit(this->stms, out);
  token_free(this->id);
  free(this);
}

struct function_t *parse_function(struct parser_t *parser, int *rcr) {
  int rc = 0;
  struct token_t *id = 0;
  struct type_t  *type = 0;
  struct var_t   *var = 0;
  struct stack_t *params = 0;
  struct stm_t   *stm = 0;
  struct stack_t *stms = 0;
 
#define T_F  type_free(type);
#define PS_F stack_free(&params, var_free);
  
  set_next_token(id, ID, {});
  next_token(L_R_B, {});
  while(!token_peek(R_R_B)) {
    if(!(var = parse_var(parser, &rc))) {
      PS_F
      parser_fail();
    }
    stack_push(&params, var);
  }
  stack_inverse(&params);
  next_token(R_R_B, { PS_F });
  
  if(token_peek(ARROW)) {
    next_token(ARROW, { PS_F });
    if(!(type = parse_type(parser, &rc))) {
      PS_F
      parser_fail()
    }
  } else {
    type = type_new(VOID_T, 0);
  }

  if(!(stms = parse_stm_lst(parser, &rc))) {
    PS_F T_F
    parser_fail();
  }
  /*
  next_token(L_C_B, { PS_F T_F });
  while(!token_peek(R_C_B)) {
    if(!(stm = parse_stm(parser, &rc))) {
      PS_F T_F SS_F
      parser_fail();
    }
    stack_push(&stms, stm);
  }
  next_token(R_C_B, { PS_F T_F SS_F });
  */
 
#undef T_F
#undef PF_F
  
  *rcr += rc;
  return function_new(token_copy(id), type, params, stms);
}

// -- TYPE ------------------------------

struct type_t *type_new(enum type_type_e type_type, void* type, size_t ref_count) {
  struct type_t *res = malloc(sizeof(struct type_t));
  res->type_type = type_type;
  res->type      = type;
  res->ref_count = ref_count;
  return res;
}

void type_free(struct type_t *this) {
  if(!this) return;
  switch(this->type_type) {
    case ID_T: token_free(this->type); break;
    case ARR_T: {
      struct arr_type_t *arr_type = this->type;
      type_free(arr_type->type);
      exp_free(arr_type->size);
      free(arr_type);
      break;
    }
    case FUN_T: {
      struct fun_type_t *fun_type = this->type;
      type_free(fun_type->type);
      stack_free(&fun_type->params, var_free);
      free(fun_type);
      break;
    }
    default: break;
  }
  free(this);
}

void type_emit(struct type_t *this, FILE* out) {
  type_emit_head(this, out);
  type_emit_tail(this, out);
}

void type_emit_head(struct type_t *this, FILE *out) {
  switch(this->type_type) {
#define X(tok, v) case tok: emit(out, v); break;
PTYPE_LIST
#undef X
    case ID_T: 
      emit(out, "%s ", ((struct token_t*)this->type)->str_val, out); 
      token_free(this->type);
      break;
    case ARR_T:
      type_emit_head(((struct arr_type_t*)this->type)->type, out);
      break;
    case FUN_T: {
      struct fun_type_t *fun_type = this->type;
      type_emit(fun_type->type, out);
      emit(out, "(*");
    }
  }
}

void type_emit_tail(struct type_t* this, FILE *out) {
  switch(this->type_type) {
    case ARR_T: {
      struct arr_type_t *arr_type = this->type
      type_emit_tail(arr_type->type, out);
      emit(out, "[");
      exp_emit(arr_type->size, out);
      emit(out, "] ");
    }
    case FUN_T: {
      struct fun_type_t *fun_type = this->type;
      emit(out, ")(");
      stack_for_each(&fun_type->params, {
        type_emit(obj, out);
        if(fun_type->next) emit(out, ", ");
      });
      emit(out, ")");
    }
    default: break;
  }
  for(;this->ref_count;this->ref_count--) {
    emit(out, "*");
  }
  free(this);
}

struct type_t *parse_type(struct parser_t *parser, int *rcr) {
  int rc = 0;
  int token_t *token = lexer_next(&parser->lexer);
  enum type_type_e type_type;
  void *type = 0;
  size_t ref_count = 0;

#define AT_F  free(arr_type);
#define TAT_F type_free(arr_type->type);
#define AE_F  exp_free(arr_type->size);
#define FS_F  stack_free(&function_type->params, type_free);
#define FT_F  free(function_type);
#define FTT_F type_free(function_type->type);

  rc++;
  switch(token->token_type) {
    case I8:   type_type = I8_T; break;
    case U8:   type_type = U8_T; break;
    case I16:  type_type = I16_T; break;
    case U16:  type_type = U16_T; break;
    case I32:  type_type = I32_T; break;
    case U32:  type_type = U32_T; break;
    case I64:  type_type = I64_T; break;
    case U64:  type_type = U64_T; break;
    case F32:  type_type = F32_T; break;
    case f64:  type_type = F64_T; break;
    case VOID: type_type = VOID_T; break;
    case ID: 
      type_type = ID_T;
      type = token_copy(token);
      break;
    case L_S_B: { // ARRAY_TYPE
      struct arr_type_t *arr_type = malloc(sizeof(struct arr_type_t));
      if(!(arr_type->type = parse_type(parser, &rc))) {
        AT_F
        parser_fail();
      }
      next_token(SEMICOLON, { type_free(arr_type->type); free(arr_type); });
      if(!(arr_type->size = parse_type(parser, &rc))) {
        TAT_F
        AT_F
        parser_fail();
      }
      next_token(R_S_B, { TAT_F AE_F AT_F });
      type = arr_type;
      break;
    }
    case L_R_B: { // FUNCTION_TYPE
      struct function_type_t *function_type = malloc(sizeof(struct function_type_t));
      struct type_t *fptype = 0;
      while(!token_peek(R_R_B)) {
        if(!(fptype = parse_type(parser, &rc))) {
          FS_F FT_F
          parser_fail();
        }
        stack_push(&function_type->params, fptype);
        if(token_peek(R_R_B)) break;
      }
      next_token(R_R_B, { FS_F FT_F });
      if(token_peek(ARROW)) {
        token_next(ARROW; { panic("bug next token should be ->"); });
        if(!(fptype = parse_type(parser, &rc))) {
          FS_F FT_F
          parser_fail();
        }
        function_type->type = fptype;
      } else {
        function_type->type = type_new(VOID_T, 0, 0);
      }

      type = function_type;
      break;
    }
    default:
      parser_fail();
  }
  
  struct token_t *temp = 0;
  for(;is_token(temp, ASTERIX); ref_count++);
  lexer_rewind(&parser->lexer, 1);
  rc--;

#undef AT_F
#undef TAT_F
#undef AE_F
#undef FS_F
#undef FT_F
#undef FTT_F
  
  *rcr += rc;
  return type_new(type_type, type, ref_count);
}

// -- STATEMENT -------------------------

struct stm_t *stack_new(enum stm_type_e stm_type, void *stm) {
  struct stm_t *res = malloc(sizeof(struct stm_t));
  res->stm_type = stm_type;
  res->stm      = stm;
  return res;
}

void stm_free(struct stm_t *this) {
  if(!this) return;
  switch(this->stm_type) {
    case CON_STM:
      con_stm_free(this->stm);
      break;
    case LOOP_STM:
      loop_stm_free(this->stm);
      break;
    case STRUCT_STM:
      struct_free(this->stm);
      break;
    case EXP_STM:
      exp_free(this->stm);
      break;
    case VAR_STM:
      var_stm_free(this->stm);
      break;
    case LST_STM:
      stack_free(&this->stm, stm_free);
      break;
  }
  free(this);
}

void stm_lst_emit(struct stack_t *this, FILE *out) {
  emit_line(out, "{");
  stack_for_each(&this->stms, {
    stm_emit(obj, out);
    emit_line(out, "");
  });
  emit_line(out, "}");
}

void stm_emit(struct stm_t *this, FILE *out) {
  swtich(this->stm_type) {
    case CON_STM: {
      struct con_stm_t *con_stm = stack_pop(&this->stm);
      emit(out, "if (");
      if(!con_stm->con) panic("invalid else statement");
      exp_emit(con_stm->con, out);
      emit(out, ")");
      stm_lst_emit(con_stm->stms, out);
      free(con_stm);
      while(this->stm) {
        con_stm = stack_pop(&this->stm);
        if(con_stm->con) {
          emit(out, "else if (");
          exp_emit(con_stm->con, out);
          emit(out, ")");
        } else {
          emit(out, "else");
        }
        stm_lst_emit(con_stm->stms, out);
        free(con_stm);
      }
      break;
    }
    case LOOP_STM: {
      struct loop_stm_t *loop_stm = this->stm;
      emit("while (");
      exp_emit(loop_stm->con, out);
      emit(") ");
      stm_lst_emit(loop_stm->stms, out);
      break;
    }
    case STRUCT_STM:
      struct_emit(this->stm, out);
      break;
    case EXP_STM: 
      exp_emit(this->stm, out);
      emit_line(out, ";");
      break;
    case VAR_STM:
      var_emit(((struct var_stm_t*)this->stm)->var, out);
      emit(out, " = ");
      exp_emit(((struct var_stm_t*)this->stm)->val, out);
      emit_line(out, ";");
      free(this->stm);
    case LST_STM:
      lst_stm_emit(this->stm, out);
  }
  free(this);
}

struct con_stm_t *con_stm_new(struct exp_t *con, struct stack_t *stms) {
  struct con_stm_t *res = malloc(sizeof(struct con_stm_t));
  res->con  = con;
  res->stms = stms;
  return res;
}

void con_stm_free(struct stack_t *this) {
  if(!this) return;
  exp_free(this->con);
  stack_free(&this->stms, stm_free);
  free(this);
}


// -- EXPRESSION ------------------------

//---------------------------------------
// MAIN PROGRAMM 
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
