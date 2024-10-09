
struct Memory_Arena {
    size_t  size;
    size_t  used;
    u8     *base;
};

void ArenaInit(Memory_Arena *arena, size_t size void *base) {
    arena->size = size;
    arena->used = 0;
    arena->base = (u8 *)base;
}

void *ArenaAlloc(Memory_Arena *arena, size_t size) {
    void *result = NULL;
    if (arena->used + size <= arena->size) {
        result = arena->base + arena->used;
        arena->used += size;
    } else {
        printf("No room in the arena for this thing babycakes!");
        ASSERT(0);
    }
    return result;
}

void ArenaClear(Memory_Arena *arena) {
    ArenaInit(arena, arena->size, arena->base);
}

void ArenaFree(Memory_Arena *arena) {
    free(arena->base);
}
