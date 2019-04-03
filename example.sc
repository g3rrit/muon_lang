foo_t;

test () -> void 
i : int = 0;
y : to = 0; { }

foo_t {
  id : *char;
  val : int;
}

gfoo : *foo_t = (init 0);

bar((*foo_t) -> void, *foo_t) -> void;

foo_print(this : *foo_t) -> void {
  (printf "foo id[%s] val[%i]\n" (pget this id) (pget this val));
}

main () -> int 
foo : foo_t = (init 0); {
  (set (get foo id) "name");
  (set (get foo val) 10);
  (bar foo_print (ref foo));
  ret 0;
}

bar (print : (*foo_t) -> void, foo : *foo_t) -> void {
  (print foo);
}
bar (print : (*foo_t) -> void, foo : *foo_t) -> void {
  (print foo);
}


