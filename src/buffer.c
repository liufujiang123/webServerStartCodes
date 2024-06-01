#include "buffer.h"

void init_dynamic_buffer(dynamic_buffer *db)
{
  db->capacity = DEFAULT_CAPACITY;
  db->current_size = 0;
  db->buf = (char *)malloc(sizeof(char) * db->capacity);
  memset(db->buf, 0, DEFAULT_CAPACITY);
}

void memset_dynamic_buffer(dynamic_buffer *db)
{
  memset(db->buf, 0, db->capacity);
  db->current_size = 0;
}
void free_dynamic_buffer(dynamic_buffer *db)
{
  if (db == NULL)
  {
    printf("ERROR!!! FREEd before\n");
    return;
  }
  db->capacity = 0;
  db->current_size = 0;
  if (strlen(db->buf))
    free(db->buf);
  free(db);
}
void append_dynamic_buffer(dynamic_buffer *db, char *src, size_t len)
{
  size_t new_size = db->current_size + len;
  if (new_size > db->capacity)
  {
    while (new_size > db->capacity)
    {
      db->capacity += DEFAULT_CAPACITY;
    }
    db->buf = (char *)realloc(db->buf, db->capacity);
    memset(db->buf + db->current_size, 0, len);
  }
  memcpy(db->buf + db->current_size, src, len);
  db->current_size = new_size;
}

void print_dynamic_buffer(dynamic_buffer *db)
{
}
void reset_dynamic_buffer(dynamic_buffer *db)
{
  db->capacity = DEFAULT_CAPACITY;
  db->buf = (char *)realloc(db->buf, db->capacity);
  memset_dynamic_buffer(db);
}
void update_dynamic_buffer(dynamic_buffer *db, char *src)
{
  db->current_size -= (src - db->buf);
  db->buf = src;
  while (db->capacity - DEFAULT_CAPACITY > db->current_size)
  {
    db->capacity -= DEFAULT_CAPACITY;
  }
}