#ifndef __SS7_H__
#define __SS7_H__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include <stdlib.h>
#include <string.h>

struct arena;
struct m3ua_msg;
struct sccp_msg;
typedef struct tcap_tcmessage tcap_msg;
typedef struct tcap_tcmessage tcap_tc_message_t;
struct map_ctx;

struct ss7_ctx {

	struct m3ua_msg *m3ua;
	struct sccp_msg *sccp;
	struct tcap_msg *tcap;
	struct map_ctx	*map;

	struct arena *mem_arena;
};

int ss7_decode(struct ss7_ctx *ctx, const uint8_t *buf, size_t size);
int ss7_json_print(const struct ss7_ctx *ctx, char *buf, size_t rem);
void ss7_free(struct ss7_ctx *ctx);


int ss7_decode_init();

static inline void *ss7_alloc(struct ss7_ctx *ctx, size_t size)
{
#ifdef SS7_USE_ARENA
	if(ctx && ctx->mem_arena)
		return arena_alloc(ctx->mem_arena, size);
#endif
	return malloc(size);
}

static inline void *ss7_calloc(struct ss7_ctx *ctx, size_t nb, size_t size)
{
#ifdef SS7_USE_ARENA
	if(ctx && ctx->mem_arena)
		return arena_calloc(ctx->mem_arena, nb, size);
#endif
	return calloc(nb, size);
}

static inline void ss7_dealloc(struct ss7_ctx *ctx, void *ptr)
{
#ifdef SS7_USE_ARENA
	if(ctx && ctx->mem_arena)
		return;
#endif
	free(ptr);
}

static inline void *ss7_realloc_array(struct ss7_ctx *ctx, void *ptr,
		size_t old_count, size_t new_count, size_t elem_size)
{
#ifdef SS7_USE_ARENA
	if(ctx && ctx->mem_arena) {
		void *next;
		if(elem_size && new_count > SIZE_MAX / elem_size)
			return NULL;
		next = arena_alloc(ctx->mem_arena, new_count * elem_size);
		if(next && ptr && old_count)
			memcpy(next, ptr, old_count * elem_size);
		return next;
	}
#endif
	(void)old_count;
	return realloc(ptr, new_count * elem_size);
}

#define SS7_ALLOC(ctx, size) ss7_alloc((struct ss7_ctx *)(ctx), (size))
#define SS7_CALLOC(ctx, nb, size) ss7_calloc((struct ss7_ctx *)(ctx), (nb), (size))
#define SS7_FREE(ctx, ptr) ss7_dealloc((struct ss7_ctx *)(ctx), (ptr))
#define SS7_REALLOC_ARRAY(ctx, ptr, old_count, new_count, type) \
	ss7_realloc_array((struct ss7_ctx *)(ctx), (ptr), (old_count), \
			(new_count), sizeof(type))

#endif //__SS7_H__
