#include "./hw4-library/memlib.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "./hw4-library/mm.h"

struct memory_region{
  size_t * start;
  size_t * end;
};

struct memory_region global_mem;
struct memory_region stack_mem;

void walk_region_and_mark(void* start, void* end);

// PROVIDED BY US (DO NOT CHANGE)
// ------------------------------
// grabbing the address and size of the global memory region from proc 
void init_global_range(){
  int next = 0;
  char file[100];
  char * line=NULL;
  size_t n=0;
  size_t read_bytes=0;
  size_t start, end;


  sprintf(file, "/proc/%d/maps", getpid());
  FILE * mapfile  = fopen(file, "r");
  if (mapfile==NULL){
    perror("opening maps file failed\n");
    exit(-1);
  }

  int counter=0;
  while ((read_bytes = getline(&line, &n, mapfile)) != -1) {
    // .data of hw4 executable
    if (strstr(line, "hw4") != NULL && strstr(line,"rw-p")){
      sscanf(line, "%lx-%lx", &start, &end);
      global_mem.start = (void*) start;
      global_mem.end = (void*) end;
      break;
    }

  }
  fclose(mapfile);
}

// marking related operations

int is_marked(unsigned int * chunk) {
  return ((*chunk) & 0x2) > 0;
}

void mark(unsigned int * chunk) {
  (*chunk)|=0x2;
}

void clear_mark(unsigned int * chunk) {
  (*chunk)&=(~0x2);
}

// chunk related operations

#define chunk_size(c)  ((*((unsigned int *)c))& ~(unsigned int)7 ) 
void* next_chunk(void* c) { 
  if(chunk_size(c) == 0) {
    fprintf(stderr,"Panic, chunk is of zero size.\n");
  }
  if((c+chunk_size(c)) < mem_heap_hi())
    return ((void*)c+chunk_size(c));
  else 
    return 0;
}

int in_use(void *c) { 
  return *(unsigned int *)(c) & 0x1;
}

// FOR YOU TO IMPLEMENT
// --------------------
// the actual collection code
void sweep() {
  void* chunk = mem_heap_lo();

  while (chunk < mem_heap_hi() && chunk_size(chunk) > 0) {
	  if (is_marked(chunk))
		  clear_mark(chunk);
	  else if (in_use(chunk))
		  mm_free(chunk + 4);
	  chunk = next_chunk(chunk);
  }
  // TODO
  //Use mm_free to free memory

}

// determine if what "looks" like a pointer actually points to an 
// in use block in the heap. if so, return a pointer to its header 
void * is_pointer(void * ptr) {
	if (!ptr)
		return NULL;
	void* deref = *(void**)ptr;
	if (deref >= mem_heap_lo() && deref < mem_heap_hi()) {
		void* cur = mem_heap_lo();
		void* next = next_chunk(cur);
		while (next < mem_heap_hi() && chunk_size(next) > 0) {
			if (next > deref && cur <= deref)
				break;
			cur = next;
			next = next_chunk(next);
		}
		if (in_use(cur))
			return cur;
	}
	return NULL;
}

// walk through memory specified by function arguments and mark
// each chunk
void walk_region_and_mark(void* start, void* end) {
  //fprintf(stderr, "walking %p - %p\n", start, end);
  // TODO
	while (start < end - 8) {
		void* chunk = is_pointer(start);
		if (chunk != NULL && !is_marked(chunk)) {
			mark(chunk);
			walk_region_and_mark(chunk + 4, next_chunk(chunk));
		}
		start += 8;
	}
}

// PROVIDED BY US (DO NOT CHANGE)
// ------------------------------
// standard initialization 

void init_gc() {
  size_t stack_var;
  init_global_range();
  // since the heap grows down, the end is found first
  stack_mem.end=(size_t *)&stack_var;
}

void gc() {
  size_t stack_var;
  // grows down, so start is a lower address
  stack_mem.start=(size_t *)&stack_var;
  // walk memory regions
  walk_region_and_mark(global_mem.start,global_mem.end);
  walk_region_and_mark(stack_mem.start,stack_mem.end);
  sweep();
}
