#include "ss7.h"
#include "m3ua.h"
#include <unistd.h>

int ss7_decode(struct ss7_ctx *ctx, const uint8_t *buf, size_t size)
{
	/* TODO: Handle errors properly */
#ifdef SS7_USE_ARENA
	if(ctx && !ctx->mem_arena) {
		ctx->mem_arena = malloc(sizeof(*ctx->mem_arena));
		if(!ctx->mem_arena || arena_init(ctx->mem_arena, 0))
			return -1;
	}
#endif
	if(!ctx->m3ua)
		ctx->m3ua = SS7_CALLOC(ctx, 1, sizeof(struct m3ua_msg));
	if(m3ua_msg_decode(ctx, buf, size)) {
#ifdef SS7_USE_ARENA
		if(ctx->mem_arena)
			arena_reset(ctx->mem_arena);
		else
			m3ua_msg_free(ctx->m3ua);
#else
		m3ua_msg_free(ctx->m3ua);
#endif
		ctx->m3ua = NULL;
		return -1;
	}
	return 0;
}
int ss7_json_print(const struct ss7_ctx *ctx, char *buf, size_t rem)
{
	if (!list_empty(&ctx->m3ua->list))
		return m3ua_msg_json_fmt(ctx->m3ua, buf, rem);
	return 0;
}
void ss7_free(struct ss7_ctx *ctx)
{
	if (ctx) {
#ifdef SS7_USE_ARENA
		if(ctx->mem_arena) {
			arena_destroy(ctx->mem_arena);
			free(ctx->mem_arena);
			free(ctx);
			return;
		}
#endif
		m3ua_msg_free(ctx->m3ua);
		free(ctx);
	}
}
