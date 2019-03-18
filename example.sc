
foo_t {
  i : i32;
  c : u8*;
};

foo : foo_t;

foo_name (foo : foo_t*) -> u8* {
  return foo->c;
}

main () -> i32 {
  printf("Test Program\n");
  printf("FOO NAME: %s\n", foo_name(&foo));
  
  return 0;
}