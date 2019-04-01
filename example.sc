foo_t {
  id : *char;
  val : int;
}

bar (print : (*foo_t) -> void, foo : *foo_t) -> void {
  (print foo);
}

foo_print(this : *foo_t) -> void {
  (printf "foo id[%s] val[%i]\n" (pget this id) (pget this val));
}

main () -> int 
foo : foo_t = 0; {
  (set (get foo id) "name");
  (set (get foo val) 10);
  (bar foo_print (ref foo));
  ret 0;
}