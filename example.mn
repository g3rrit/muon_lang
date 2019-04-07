// THIS IS A LINE_COMMENT

/* THIS 
   IS 
   A 
   MUTLILINE 
   // COMMENT 
*/

// structure declaration
foo_t {
  id : *char;
  val : int;
}

// forward decalaration of a structure
loo_t;

// variable declaration
extern loo: loo_t;

// variable declaration
gfoo : *foo_t = (init 0);

// function declaration
bar((*foo_t) -> void, *foo_t) -> void;

// function definition
foo_print(this : *foo_t) -> void {
  (printf "foo id[%s] val[%i]\n" (pget this id) (pget this val));
}

// function definition
main () -> int 
foo : foo_t = (init 0); 
t : int = 0; 
i : int = 0; {
  (set (get foo id) "name");
  (set (get foo val) 10);
  (bar foo_print (ref foo));
label:
  (set i (add i 1));
  (printf "i = %d\n" i);
  jmp (lt i 10) label; 

  ret 0;
}

// function definition
bar (print : (*foo_t) -> void, foo : *foo_t) -> void {
  (print foo);
}

