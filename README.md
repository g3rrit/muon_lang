
![alt text](/logo.png "Muon Logo")

## About
---

This project is intended to be viewed as a simple demonstration for 
a transpiler written in < 2000 lines of c code.
It was originally intended to be just a simple exercise in writing 
a basic **Parser-Combinator**.
Altough it is able to produce valid c code, it is
still far from being usable without a firm understanding of how the
transpiler works.

It is also my first project i would really consider finished.
(for myself and the public eye) 

That said if you're still interessted, i invite you to have a look around.
You can contact me at <gerrit.proessl@gmail.com>.
I would certainly appreciate any feedback :)
> Names that end with _t are reserved by POSIX

## The Muon Language
---

**Muon** is a syntactically simple imperative language.
The transpiler emits **c** code.
It strongly relies on and heavily utilizes constructs
found in the **c language**.
It is only possible to transpile one file programs.
The formal grammar for the language can be found at ./lang.grammar

## Building
---

```sh
$ make
```

## Usage
---

```sh
$ build/comp example.mn output.c
$ gcc output.c
```

## Example:
---

```c
printf() -> void;

foo_t {
  id: *char;
}

main() -> int 
foo: foo_t = (init "Hello Muon");
i: int = 0; 
{
  (printf "%s\n" (get foo id));

loop:
  (printf "i: %d\n" i); 
  (inc i);
  jmp (lt i 10) loop;
  
  ret 0;
}
```

produces ->

```c
#define set(lexp, rexp)  (lexp = rexp)   
#define ref(exp)         (&exp)          
#define deref(exp)       (*exp)          
#define get(lexp, rexp)  (lexp.rexp)     
#define pget(lexp, rexp) (lexp->rexp)    
#define aget(exp, index) (exp[index])    
#define cast(exp, type)  ((type)exp)     
#define size(exp)        (sizeof(exp))   
#define lst(...)         ( __VA_ARGS__ ) 
#define init(...)        { __VA_ARGS__ } 
// UNARY_OPERATORS                       
#define inc(exp)         (exp++)         
#define dec(exp)         (exp--)         
#define pos(exp)         (+exp)          
#define neg(exp)         (-exp)          
#define bnot(exp)        (~exp)          
#define not(exp)         (!exp)          
// BINARY_OPERATORS                      
#define add(lexp, rexp)  (lexp + rexp)   
#define sub(lexp, rexp)  (lexp - rexp)   
#define mul(lexp, rexp)  (lexp * rexp)   
#define div(lexp, rexp)  (lexp / rexp)   
#define and(lexp, rexp)  (lexp && rexp)  
#define or(lexp, rexp)   (lexp || rexp)  
#define mod(lexp, rexp)  (lexp % rexp)   
#define lt(lexp, rexp)   (lexp < rexp)   
#define gt(lexp, rexp)   (lexp > rexp)   
#define eq(lexp, rexp)   (lexp == rexp)  
#define leq(lexp, rexp)  (lexp <= rexp)  
#define geq(lexp, rexp)  (lexp >= rexp)  
#define band(lexp, rexp) (lexp & rexp)   
#define bor(lexp, rexp)  (lexp | rexp)   
#define bxor(lexp, rexp) (lexp ^ rexp)   
#define ls(lexp, rexp)   (lexp << rexp)  
#define rs(lexp, rexp)   (lexp >> rexp)  

void printf();
typedef struct foo_t foo_t;
typedef struct foo_t {
char* id ;
} foo_t;
int main() {
foo_t foo  = init("Hello Muon");
int i  = 0;
printf("%s\n", get(foo, id));
loop:
printf("i: %d\n", i);
inc(i);
if(lt(i, 10)) goto loop;
return 0;
}
```

You can find another example program at ./example.mn.

## Documentation
---

### Types
The languages support four basic forms of types.
* Array Types
* Function Types
* Pointer Types
* Identifier Types

#### Identifier Types
As the transpiler does no typechecking there is no need for 
introducing primative types. All primative types and compound types
(defined by structures) are just **Identifier Types**.

#### Array Types
**Array Types** are for declaring an array of variables.
**Array Types** are declared in the following way:
```c
[type; exp]
```
where **type** describes the type of an element
and **exp** describes the size of the array as an expression.
(Note: the size needs to be a constant expression if the array
is declared outside of a function)

#### Function Types
**Function types** are for declaring a pointer to a function.
They are declared in the following way:
```c
(param_type_1, param_type_2, ...) -> return_type
```
where **param_type_n** is the type of the n-th parameter of the 
function and **return_type** is the return type of the function.

#### Pointer Type
**Pointer Types** are for declaring pointers to a variable.
They are declared in the following way:
```c
*type
```
where **type** is the type of the variable to pointer
is pointing to.

### Structures

```c
id {
  var_id: type;
  ...
}
```

Structures pretty much act like typedefined c-structs.
**id** is the identifier of the structure and 
inside the curly brackets are the variables the structure holds.

### Functions

```c
function_id (var_id: type, ...) -> return_type 
local_var: type = exp;
...
{
  statement;
}
```

A **Function** starts with its **identifier** followed by its **parameters** and **return type**.
In front of the **function body** all variables that will be used additionally to the paramters
get defined and initiated.

### Statements

There are four kinds of **Statements**.
* Expression Statement
* Label Statement
* Jump Statement
* Return Statement

All **Statements** are terminated by a semicolon.

#### Expression Statement
Just a **Expression** terminated by a semicolon.

#### Label Statement
```c
label:
```
A **Label Statement** marks a line of code where one can jump 
to with the **Jump Statement**.

#### Jump Statement
**Conditional Jump Statement:**
```c
jmp exp label;
```
**Jump Statement:**
```c
jmp label;
```
Continues execution at the specified label if the condition is met or if no
condition is specified.

#### Return Statement
```c
ret exp;
```
Returns the value of **exp** from the current function.

### Expressions

There are sic types of **Expressions**
* Integer Expression
* Identifier Expression
* String Expression 
* Float Expression
* Char Expression
* Call Expression

#### Integer Expression
Just a integer literal `10` `2`

#### Identifier Expression
Just a identifier `var` `foo`

#### String Expression
Just a ltring literal `"Hello"` `"World"`

#### Char Expression
Just a char literal `'c'` `'\n'`

#### Call Expression
```c
(call_id param_1 param_2 ...)
```
A **Call Expression** ist mostly just a 
function call, where the **call_id** is is the first
element in a list of expressions followed by its parameters,
all inside round brackets.

Certain functionality which dont directly corrospond to 
function calls in c are also exposed through call expressions 
by some predefined c macros. 
These macros are just prepended to the output. 

### Call Macro List:

* set(lexp, rexp)   -> (lexp = rexp)   
* ref(exp)          -> (&exp)          
* deref(exp)        -> (*exp)          
* get(lexp, rexp)   -> (lexp.rexp)     
* pget(lexp, rexp)  -> (lexp->rexp)    
* aget(exp, index) (exp -> [index])    
* cast(exp, type)   -> ((type)exp)     
* size(exp)        (sizeof -> (exp))   
* lst(...)          -> ( \_\_VA_ARGS\_\_ ) 
* init(...)         -> { \_\_VA_ARGS\_\_ } 

**UNARY OPERATORS**
* inc(exp)          -> (exp++)         
* dec(exp)          -> (exp--)         
* pos(exp)          -> (+exp)          
* neg(exp)          -> (-exp)          
* bnot(exp)         -> (~exp)          
* not(exp)          -> (!exp)          

**BINARY_OPERATORS**
* add(lexp, rexp)   -> (lexp + rexp)   
* sub(lexp, rexp)   -> (lexp - rexp)   
* mul(lexp, rexp)   -> (lexp * rexp)   
* div(lexp, rexp)   -> (lexp / rexp)   
* and(lexp, rexp)   -> (lexp && rexp)  
* or(lexp, rexp)    -> (lexp || rexp)  
* mod(lexp, rexp)   -> (lexp % rexp)   
* lt(lexp, rexp)    -> (lexp < rexp)   
* gt(lexp, rexp)    -> (lexp > rexp)   
* eq(lexp, rexp)    -> (lexp == rexp)  
* leq(lexp, rexp)   -> (lexp <= rexp)  
* geq(lexp, rexp)   -> (lexp >= rexp)  
* band(lexp, rexp)  -> (lexp & rexp)   
* bor(lexp, rexp)   -> (lexp | rexp)   
* bxor(lexp, rexp)  -> (lexp ^ rexp)   
* ls(lexp, rexp)    -> (lexp << rexp)  
* rs(lexp, rexp)    -> (lexp >> rexp)  




