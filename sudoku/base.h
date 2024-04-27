/* date = April 7th 2024 3:56 pm */

#ifndef C_BASE_API_H
#define C_BASE_API_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// noted: keep this same structure with String and you are fine
struct CString {
    u32 size;
    union {
        char *str;
        void *data;
    };
};

#define KB(n) ((n) * 1024)
#define MB(n) ((n) * KB(1024))
#define GB(n) ((n) * MB(1024))

#ifndef assert
#define assert(exp) if (!(exp)) (*((int*)0) = 0)
#endif
#define array_count(arr) (sizeof(arr) / sizeof(arr[0]))
#define swap(x, y, type) {  type temp = x; x = y; y = temp; }

#ifdef _cplusplus
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#define lerp(v0, v1,  t) ((v0) + (t) * ((v1) - (v0)))

// double circular linked list
// (s)entinal, (n)ode, (t)ype 
// require: need to define next and prev pointer in your struct
#define dll_init(s) (s)->next=s,(s)->prev=s
#define dll_begin(s) (s)->next
#define dll_end(s) (s)->prev
#define dll_is_end(s, n) (n == s)
#define dll_next(n) n->next
#define dll_is_empty(s) ((s)->next) == (s)
#define dll_not_empty(s) ((s)->next) != (s)

#define dll_insert(n, val) (val)->prev =n, (val)->next=(n)->next, (val)->prev->next=val, (val)->next->prev=val
#define dll_insert_back(s, n) dll_insert((s)->prev, n)
#define dll_insert_front(s, n) dll_insert(s, n)
#define dll_remove(n) (n)->prev->next=(n)->next, (n)->next->prev=(n)->prev, (n)->next=(n)->prev=0


static void mem_copy(void *src, void *dest, u64 size) {
    u64 copy_bytes = size; 
    u8 *u8_src = (u8*)src;
    u8 *u8_dest = (u8*)dest;
    while (copy_bytes--) {
        *(u8_dest++) = *(u8_src++);
    }
}

typedef struct String {
    u32 size;
    union {
        char *str;
        void* data;
    };
    u32 mem_size;
} String;

#endif
