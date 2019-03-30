main () -> u8 {
  (add 1 2);
  jmp label;
label:
  ret 0;
}

foo_t {
 i : u8;
 n: u8;
}

