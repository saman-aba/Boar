#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__
#include "hlist.h"
#include "hash.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static inline int fls(unsigned long word)
{
	return 64 - __builtin_clzl(word);
}

static __always_inline __attribute__((const)) int __ilog2_u32(uint32_t n)
{
	return fls(n) - 1;
}

static __always_inline __attribute__((const)) int __ilog2_u64(uint64_t n)
{
	return fls(n) - 1;
}

#define ilog2(n) \
( 					\
	__builtin_constant_p(n) ?	\
	((n) < 2 ? 0 :			\
	63 - __builtin_clzll(n)) :	\
	(sizeof(n) <= 4) ?		\
	__ilog2_u32(n) :		\
	__ilog2_u64(n)			\
)


#define HASHTABLE(name, bits) 						\
	struct hlist_head name[1 << (bits)] = 				\
			{ [0 ... ((1 << (bits)) - 1)] = HLIST_HEAD_INIT } 

// I should define read-mostly section for read-mostly hashtable

#define HASHTABLE_DECLARE(name, bits)					\
	struct hlist_head name[1 << (bits)]

#define HASH_SIZE(name) (ARRAY_SIZE(name))
#define HASH_BITS(name) ilog2(HASH_SIZE(name))

#define hash_min(val, bits)						\
	(sizeof(val) <= 4 ? hash_32(val, bits) : hash_long(val, bits))

static inline void __hash_init(struct hlist_head *ht, unsigned int sz)
{
	unsigned int i;
	for(i = 0; i < sz; i++)
		INIT_HLIST_HEAD(&ht[i]);
	
}

#define hash_init(ht) __hash_init(ht, HASH_SIZE(ht))

#define hash_add(ht, node, key)						\
	hlist_add_head(node, &ht[hash_min(key, HASH_BITS(ht))])

//#define hash_add_rcu(ht, node, key)

static inline bool hash_hashed(struct hlist_node *node)
{
	return !hlist_unhashed(node);
}

static inline bool __hash_empty(struct hlist_head *ht, unsigned int sz)
{
	unsigned int i;
	for(i = 0; i < sz; i++)
		if(!hlist_empty(&ht[i]))
			return false;
	return true;
}

#define hash_empty(ht) __hash_empty(ht, HASH_SIZE(ht))

static inline void hash_del(struct hlist_node *node)
{
	hlist_del_init(node);
}

//static inline void hash_del_rcu(struct hlist_node *node)

#define hash_for_each(name, bkt, obj, member)				\
	for((bkt) = 0, obj = NULL; 					\
			obj == NULL && (bkt) < HASH_SIZE(name);		\
			(bkt)++)					\
		hlist_for_each_entry(obj, &name[bkt], member)

//#define hash_for_each_rcu(name, bkt, obj, member)

#define hash_for_each_safe(name, bkt, tmp, obj, member)                 \
	for ((bkt) = 0, obj = NULL; 					\
		obj == NULL && (bkt) < HASH_SIZE(name);			\
		(bkt)++)						\
		hlist_for_each_entry_safe(obj, tmp, &name[bkt], member)

#define hash_for_each_possible(name, obj, member, key)			\
	hlist_for_each_entry(obj, &name[hash_min(key, HASH_BITS(name))],\
			member)						\
		
#define hash_for_each_possible_rcu(name, obj, member, key, cond...)

#define hash_for_each_possible_rcu_notrace(name, obj, member, key)	\
		hlist_for_each_entry_rcu_notrace(obj, 			\
				&name[hash_min(key, HASH_BITS(name))],	\
				member)

#define hash_for_each_possible_safe(name, obj, tmp, member, key)	\
		hlist_for_each_entry_safe(obj, tmp,			\
				&name[hash_min(key, HASH_BITS(name))],	\
				member)
	
#endif //__HASHTABLE_H__
