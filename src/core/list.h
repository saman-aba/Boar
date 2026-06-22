#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>
#include <stddef.h>

#ifndef LIST_POISON1
#define LIST_POISON1 ( ( void *) 0xdeadbeef)
#endif
#ifndef LIST_POISON2
#define LIST_POISON2 ( ( void *) 0xfeedface)
#endif

struct list_head {
	struct list_head *next, *prev;
};

/* Circular doubly linked list */
#define LIST_HEAD_INIT( name) { &(name), &( name)}

#define list_head(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD( struct list_head *list)
{
	list->next = list;
	list->prev = list;
}


static inline __attribute__((__always_inline__)) bool _list_add_validate(struct list_head *new, 
		struct list_head *prev,
		struct list_head *next)
{
	/* TODO: Slowpath add must be performed */
	if(__builtin_expect((next->prev == prev &&
			prev->next == next &&
			new != prev &&
			new != next), 1))
		return true;
	return false;

}

static inline __attribute__((__always_inline__)) bool _list_delete_entry_validate(struct list_head *entry)
{
	struct list_head *prev = entry->prev;
	struct list_head *next = entry->next;
	if(__builtin_expect((prev->next == entry && next->prev == entry), 1))
		return true;
	return false;
}

static inline void _list_add(struct list_head *new,
				struct list_head *prev,
				struct list_head *next)
{
	if( !_list_add_validate(new, prev, next))
		return;

	next->prev = new;
	new->next = next;
	new->prev = prev;
	/* TODO: ensure that memory is written once, How(?) */
	prev->next = new;
}

/**
 * list_add
 * Suitable for stacks.
 */
static inline void list_add( struct list_head *new, struct list_head *head)
{
	_list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	_list_add(new, head->prev, head);
}

static inline void _list_delete(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	*(volatile struct list_head **)(&prev->next) = next;
}

static inline void _list_delete_clearprev(struct list_head *entry)
{
	_list_delete(entry->prev, entry->next);
	entry->prev = NULL;
}
static inline void _list_delete_entry(struct list_head *entry)
{
	if( !_list_delete_entry_validate(entry))
		return;
	_list_delete(entry->prev, entry->next);
}

static inline void list_delete( struct list_head *entry)
{
	_list_delete_entry( entry);
	entry->next = NULL; //LIST_POISON1;
	entry->prev = NULL; //LIST_POISON2;
}

static inline void list_replace(struct list_head *old, struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void list_replace_init(struct list_head *old, struct list_head *new)
{
	list_replace(old, new);
	INIT_LIST_HEAD(old);
}

static inline void list_swap(struct list_head *entry1, struct list_head *entry2)
{
	struct list_head *pos = entry2->prev;

	list_delete(entry2);
	list_replace(entry1, entry2);
	if(pos == entry1)
		pos = entry2;
	list_add(entry1, pos);
}

static inline void list_delete_init(struct list_head *entry)
{
	_list_delete_entry(entry);
	INIT_LIST_HEAD(entry);
}

static inline void list_bulk_move_tail(struct list_head *head, 
		struct list_head *first, 
		struct list_head *last)
{
	first->prev->next = last->next;
	last->next->prev = first->prev;

	head->prev->next = first;
	first->prev = head->prev;

	last->next = head;
	head->prev = last;
}

static inline int list_is_first(const struct list_head *list, 
		const struct list_head *head)
{
	return list->prev == head;
}

static inline int list_is_last(const struct list_head *list, 
		const struct list_head *head)
{
	return list->next == head;
}

static inline int list_is_head(const struct list_head *list, 
		const struct list_head *head)
{
	return list == head;
}

static inline int list_empty(const struct list_head *head)
{
	/* TODO: Ensure head->next read once. */
	if(!(head->next))
		return 1;
	return head->next == head;
}
		
#define list_entry(ptr, type, member) 				\
	CONTAINER_OF( ptr, type, member)

#define list_first_entry(ptr, type, member) 			\
	list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) 			\
	list_entry((ptr)->prev, type, member)
#define list_entry_is_head(pos, head, member)                           \
         list_is_head(&pos->member, (head))

#define list_first_entry_or_null(ptr, type, member) ({		\
	struct list_head *head_ = (ptr); 			\
	struct list_head *pos_ = head_->next);			\
	pos_ != head_ ? list_entry(pos_, type, member) : NULL;	\
})

#define list_next_entry(pos, member)				\
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_next_entry_circular(pos, head, member)		\
	(list_is_last(&(pos)->member, head)?			\
	list_last_entry(head, typeof(*(pos)), member) : last_prev_entry(pos, member))

#define list_foreach( pos, head) \
	for ( pos = (head)->next; !list_is_head(pos, (head)); pos = pos->next) 

#define list_foreach_safe(pos, n, head) for (pos = (head)->next, n = pos->next; \
             !list_is_head(pos, (head)); \
             pos = n, n = pos->next)

#define list_foreach_entry_safe(pos, n, head, member)                  \
        for (pos = list_first_entry(head, typeof(*pos), member),        \
                n = list_next_entry(pos, member);                       \
             !list_entry_is_head(pos, head, member);                    \
             pos = n, n = list_next_entry(n, member))

	
#define OFFSETOF( type, member) ( ( size_t) &( ( type *) 0)->member)

#define CONTAINER_OF( ptr, type, member) ({ 			\
	const typeof( ( ( type *) 0)->member) *__mptr = (ptr); 	\
	( type *)( ( char *)__mptr - OFFSETOF( type, member));})


#endif // __LIST_H__
