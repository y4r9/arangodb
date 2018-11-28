#include "malloc-wrapper.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef ARANGODB_HAVE_JEMALLOC
#include "jemalloc/jemalloc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// forwards for the standard library's memory allocation operations
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(1) *__real_malloc(size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE2(1, 2) *__real_calloc(size_t nmemb, size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW __real_free(void* ptr) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(2) *__real_realloc(void* ptr, size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC *__real_memalign(size_t alignment, size_t size) JSMALLOC_CXX_THROW;
int JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_ALLOC_SIZE2(2, 3) __real_posix_memalign(void** memptr, size_t alignment, size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(2) *__real_aligned_alloc(size_t alignment, size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(1) *__real_valloc(size_t size) JSMALLOC_CXX_THROW;
size_t JSMALLOC_ATTRIBUTE_NOTHROW __real_malloc_usable_size(void* ptr) JSMALLOC_CXX_THROW;
int JSMALLOC_ATTRIBUTE_NOTHROW __real_malloc_trim(size_t pad) JSMALLOC_CXX_THROW;

// forwards for the standard library's memory allocation operations
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(1) *jem51_malloc(size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE2(1, 2) *jem51_calloc(size_t nmemb, size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW jem51_free(void* ptr) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(2) *jem51_realloc(void* ptr, size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC *jem51_memalign(size_t alignment, size_t size) JSMALLOC_CXX_THROW;
int JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_ALLOC_SIZE2(2, 3) jem51_posix_memalign(void** memptr, size_t alignment, size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(2) *jem51_aligned_alloc(size_t alignment, size_t size) JSMALLOC_CXX_THROW;
void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(1) *jem51_valloc(size_t size) JSMALLOC_CXX_THROW;
size_t JSMALLOC_ATTRIBUTE_NOTHROW jem51_malloc_usable_size(void* ptr) JSMALLOC_CXX_THROW;


// function pointers in use for actual memory allocation operations
static void*  (*runtime_malloc)(size_t) = __real_malloc;
static void*  (*runtime_calloc)(size_t, size_t) = __real_calloc;
static void   (*runtime_free)(void*) = __real_free;
static void*  (*runtime_realloc)(void*, size_t) = __real_realloc;
static void*  (*runtime_memalign)(size_t, size_t) = __real_memalign;
static int    (*runtime_posix_memalign)(void**, size_t, size_t) = __real_posix_memalign;
static void*  (*runtime_aligned_alloc)(size_t, size_t) = __real_aligned_alloc;
static void*  (*runtime_valloc)(size_t) = __real_valloc;
static size_t (*runtime_malloc_usable_size)(void*) = __real_malloc_usable_size;
static int    (*runtime_malloc_trim)(size_t) = __real_malloc_trim;


static int arangodb_shim_malloc_trim(size_t pad) {
  return 0;
}

static void arangodb_malloc_initialize() __attribute__((constructor));
static void arangodb_malloc_shutdown() __attribute__((destructor));

extern bool malloc_init();

static void arangodb_malloc_initialize() {
  runtime_malloc = __real_malloc;
  runtime_calloc = __real_calloc;
  runtime_realloc = __real_realloc;
  runtime_free = __real_free;
  runtime_valloc = __real_valloc;
  runtime_memalign = __real_memalign;
  runtime_posix_memalign = __real_posix_memalign;
  runtime_aligned_alloc = __real_aligned_alloc;
  runtime_valloc = __real_valloc;
  runtime_malloc_usable_size = __real_malloc_usable_size;
  runtime_malloc_trim = __real_malloc_trim;

#ifdef ARANGODB_HAVE_JEMALLOC
  char const* p = getenv("ARANGODB_MALLOC_LIB");

  if (p != nullptr && strcmp(p, "jemalloc") == 0) {
    jem51_malloc(0);
    runtime_malloc = jem51_malloc;
    runtime_calloc = jem51_calloc;
    runtime_realloc = jem51_realloc;
    runtime_free = jem51_free;
    runtime_valloc = jem51_valloc;
    runtime_memalign = jem51_memalign;
    runtime_posix_memalign = jem51_posix_memalign;
    runtime_aligned_alloc = jem51_aligned_alloc;
    runtime_valloc = jem51_valloc;
    runtime_malloc_usable_size = jem51_malloc_usable_size;
    runtime_malloc_trim = arangodb_shim_malloc_trim;
  }
#endif
}

static void arangodb_malloc_shutdown() {
  runtime_malloc = __real_malloc;
  runtime_calloc = __real_calloc;
  runtime_realloc = __real_realloc;
  runtime_free = __real_free;
  runtime_valloc = __real_valloc;
  runtime_memalign = __real_memalign;
  runtime_posix_memalign = __real_posix_memalign;
  runtime_aligned_alloc = __real_aligned_alloc;
  runtime_valloc = __real_valloc;
  runtime_malloc_usable_size = __real_malloc_usable_size;
  runtime_malloc_trim = __real_malloc_trim;
}

/// public functions
/// the C library memory management functions we will override

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(1) *__wrap_malloc(size_t size) JSMALLOC_CXX_THROW {
  void* p = runtime_malloc(size);
//printf("malloc %llu -> %p\n", (unsigned long long) size, p);
return p;
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE2(1, 2) *__wrap_calloc(size_t nmemb, size_t size) JSMALLOC_CXX_THROW {
  void* p  = runtime_calloc(nmemb, size);
//printf("calloc %llu -> %p\n", (unsigned long long) (nmemb * size), p);
return p;
}

void JSMALLOC_ATTRIBUTE_NOTHROW __wrap_free(void* ptr) JSMALLOC_CXX_THROW {
//printf("free %p\n", ptr);
  runtime_free(ptr);
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(2) *__wrap_realloc(void* ptr, size_t size) JSMALLOC_CXX_THROW {
  void* p = runtime_realloc(ptr, size);
//printf("realloc %p %llu -> %p\n", ptr, (unsigned long long) size, p);
return p;
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC *__wrap_memalign(size_t alignment, size_t size) JSMALLOC_CXX_THROW {
//printf("memalign\n");
  return runtime_memalign(alignment, size);
}

int JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_ALLOC_SIZE2(2, 3) __wrap_posix_memalign(void** memptr, size_t alignment, size_t size) JSMALLOC_CXX_THROW {
//printf("posix_memalign\n");
  return runtime_posix_memalign(memptr, alignment, size);
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(2) *__wrap_aligned_alloc(size_t alignment, size_t size) JSMALLOC_CXX_THROW {
//printf("aligned_alloc\n");
  return runtime_aligned_alloc(alignment, size);
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(1) *__wrap_valloc(size_t size) JSMALLOC_CXX_THROW {
//printf("valloc\n");
  return runtime_valloc(size);
}

size_t JSMALLOC_ATTRIBUTE_NOTHROW __wrap_malloc_usable_size(void* ptr) JSMALLOC_CXX_THROW {
//printf("malloc_usable_size\n");
  return runtime_malloc_usable_size(ptr);
}

int JSMALLOC_ATTRIBUTE_NOTHROW __wrap_malloc_trim(size_t pad) JSMALLOC_CXX_THROW {
//printf("malloc_trim\n");
  return runtime_malloc_trim(pad);
}



/*

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(1) *malloc(size_t size) JSMALLOC_CXX_THROW {
  void* p = runtime_malloc(size);
printf("malloc %llu -> %p\n", (unsigned long long) size, p);
return p;
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE2(1, 2) *calloc(size_t nmemb, size_t size) JSMALLOC_CXX_THROW {
  void* p  = runtime_calloc(nmemb, size);
printf("calloc %llu -> %p\n", (unsigned long long) (nmemb * size), p);
return p;
}

void JSMALLOC_ATTRIBUTE_NOTHROW free(void* ptr) JSMALLOC_CXX_THROW {
printf("free %p\n", ptr);
  runtime_free(ptr);
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(2) *realloc(void* ptr, size_t size) JSMALLOC_CXX_THROW {
  void* p = runtime_realloc(ptr, size);
printf("realloc %p %llu -> %p\n", ptr, (unsigned long long) size, p);
return p;
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC *memalign(size_t alignment, size_t size) JSMALLOC_CXX_THROW {
printf("memalign\n");
  return runtime_memalign(alignment, size);
}

int JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_ALLOC_SIZE2(2, 3) posix_memalign(void** memptr, size_t alignment, size_t size) JSMALLOC_CXX_THROW {
printf("posix_memalign\n");
  return runtime_posix_memalign(memptr, alignment, size);
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(2) *aligned_alloc(size_t alignment, size_t size) JSMALLOC_CXX_THROW {
printf("aligned_alloc\n");
  return runtime_aligned_alloc(alignment, size);
}

void JSMALLOC_ATTRIBUTE_NOTHROW JSMALLOC_ATTRIBUTE_MALLOC JSMALLOC_ATTRIBUTE_ALLOC_SIZE1(1) *valloc(size_t size) JSMALLOC_CXX_THROW {
printf("valloc\n");
  return runtime_valloc(size);
}

size_t JSMALLOC_ATTRIBUTE_NOTHROW malloc_usable_size(void* ptr) JSMALLOC_CXX_THROW {
printf("malloc_usable_size\n");
  return runtime_malloc_usable_size(ptr);
}

int JSMALLOC_ATTRIBUTE_NOTHROW malloc_trim(size_t pad) JSMALLOC_CXX_THROW {
printf("malloc_trim\n");
  return runtime_malloc_trim(pad);
}

*/







#ifdef __cplusplus
}
#endif
