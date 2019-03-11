//---------------------------------------
// LIST 
//---------------------------------------

struct list_t {
  void          *obj
  struct list_t *next;
};

struct list_t *list_new(void *obj) {
  struct list_t *res = malloc(sizeof(struct list_t));
  res->obj = obj;
  res->next = 0;
  return res;
}

void list_add(struct list_t *this, void *obj) {
  for(;this->next; this = this->next);
  this->next = list_next(obj);
}

void *list_at(struct list_t *this, size_t pos) {
  for(size_t i = 0; i < pos; i++) {
    if(!this->next) return (void*)0;
    this = this->next;
  }
  return this->obj;
}

void *list_remove(struct list_t **this, size_t pos) {
  for(size_t i = 0; i < pos; i++) {
    if(!(*this)->next) return (void*)0;
    this = &(*this)->next;
  }
  void *res = (*this)->obj;
  struct list_t *f = *this;
  *this = this->next;
  free(f);
  return res;
}

void list_delete(struct list_t *this) {
  struct list_t *f = 0;
  while(this) {
    f = this;
    this = this->next;
    free(f);
  }
}

