
struct Memory_Arena {
    size_t  size;
    size_t  used;
    u8     *base;
};

void ArenaInit(Memeory_Arena *arena, size_t size void *base) {
    arena->size = size;
    arena->used = 0;
    arena->base = (u8 *)base;
}
