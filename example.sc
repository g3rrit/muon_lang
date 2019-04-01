main () -> int {
  (foo 34);
}

foo_t {
  id : *char;
  val : int;
}

bar (print : (*foo_t) -> void, foo : *foo_t) -> void {
  (print foo);
}

foo_print(this : *foo_t) -> void {
  (printf "foo id[%s] val[%i]\n" this->id this->val);
}

