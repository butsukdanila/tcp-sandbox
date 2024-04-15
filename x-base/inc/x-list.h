#ifndef __X_LIST_H__
#define __X_LIST_H__

#include "x-types.h"
#include "x-misc.h"

struct list {
  struct list *next;
  struct list *prev;
};
typedef struct list list_t;

#define list_root(_self) (list_t) { (_self), (_self) }

static inline void
list_init(list_t *root) {
  root->next = root;
  root->prev = root;
}

static inline void
list_link(list_t *prev, list_t *next) {
  next->prev = prev;
  prev->next = next;
}

static inline void
list_insert(list_t *node, list_t *prev, list_t *next) {
  list_link(node, next);
  list_link(prev, node);
}

static inline void
list_unlink(list_t *node) {
  list_link(node->prev, node->next);
}

static inline void
list_insert_head(list_t *node, list_t *root) {
  list_insert(node, root, root->next);
}

static inline void
list_insert_tail(list_t *node, list_t *root) {
  list_insert(node, root->prev, root);
}

#define list_foreach(_node, _root) \
  for (_node  = (_root)->next; \
       _node != (_root); \
       _node  = (_node)->next)

#define list_data(_node, _type, _member) \
  container_of(_node, _type, _member)

#define list_head_data(_root, _type, _member) \
  list_data((_root)->next, _type, _member)

#define list_tail_data(_root, _type, _member) \
  list_data((_root)->prev, _type, _member)

#define list_next_data(_data, _member) \
  list_data((_data)->_member.next, typeof(*(_data)), _member)

#define list_prev_data(_data, _member) \
  list_data((_data)->_member.prev, typeof(*(_data)), _member)

#define list_foreach_data(_data, _root, _member) \
  for (_data = list_head_data(_root, typeof(*_data), _member); \
      &_data->_member != (_root); \
       _data = list_next_data(_data, _member))

#define list_foreach_data_safe(_data, _temp, _head, _member) \
  for (_data = list_head_data(_head, typeof(*_data), _member), \
       _temp = list_next_data(_data, _member); \
      &_data->_member != (_head); \
       _data = _temp, \
       _temp = list_next_data(_temp, _member))

static inline size_t
list_size(list_t *root) {
  size_t  size = 0;
  list_t *node = null;
  list_foreach(node, root) {
    size++;
  }
  return size;
}

#endif//__X_LIST_H__