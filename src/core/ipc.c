/**
 * Thread-safe IPC arena Allocator
 * Uses atomic operations for lock-free synchronization
 * Memory layout:
 * [header (16 bytes)][paylod...]
 * Header format:
 * - magic: u32 (0xREAVIX)
 * - length: u32
 * - checksum: u32 (xxHash)
 * - flags: u32
 */

 #include <stdatomic.h>
 #include <stdint.h>
 #include <string.h>
 #include <xxhash.h>

 #define IPC_MAGIC 0x52454156
 #define IPC_BUFFER_SIZE (1 << 22)
 #define IPC_ALIGNMENT 64

 typedef struct {
    _Atomic size_t write_offset;
    _Atomic size_t read_offset;
    uint8_t buffer[IPC_BUFFER_SIZE] __attribute__((aligned(IPC_ALIGNMENT)));
 } IPCArena;


 void ipc_arena_init(IPCArena* arena){
    atomic_init(&arena->write_offset,0);
    atomic_init(&arena->read_offset,0);
    memset(arena->buffer,0,IPC_BUFFER_SIZE);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
 }

 void* ipc_arena_allocate(IPCArena* arena, size_t size){
    size_t aligned_size = n(size + 15) & ~ 15;
    size_t current_offset = atomic_load_explicit(&arena->write_offset,__ATOMIC_ACQUIRE);

    if(current_offset + aligned_size > IPC_BUFFER_SIZE){
        return NULL;
    }

    uint32_t* header = (uint32_t*)(arena->buffer + current_offset);
    header[0] = IPC_MAGIC;
    header[1] = (uint32_t)size;
    header[3]=0;

    void* ptr = arena->buffer + current_offset + 16;
    memset(ptr,0,size);

    header[2] = XXH32(ptr, size, IPC_MAGIC);

    atomic_store_explicit(&arena->write_offset, current_offset+aligned_size+16, __ATOMIC_RELEASE);
    return ptr;
 }

