# babymalloc
A dynamic memory allocator in C, inspired by the [CS:APP malloc lab](http://csapp.cs.cmu.edu/3e/labs.html) from the CMU course [Intro to Computer Systems](https://www.cs.cmu.edu/afs/cs/academic/class/15213-f15/www/schedule.html).

### Description
This is a dynamic memory allocator that utilizes simple techniques:
- First-fit placement policy:
  - The allocator searches for the first free block that is large enough to fit the requested size.
- Boundary tags:
  - Each block contains a header and footer that store the size of the block and whether the block is allocated or free.
  - This allows the allocator to traverse the heap in both directions.
- Implicit free list:
  - The allocator uses a simple implicit free list to keep track of free blocks.
  - The allocator traverses the free list using the size of the blocks.
- Immediate coalescing:
  - When a block is freed, the allocator checks if the adjacent blocks (in both sides) are free and coalesces them.
- Splitting:
  - When a free block is selected for an allocation, the allocator checks if the block is large enough to be split into two blocks.
- sbrk:
  - The allocator uses the sbrk system call to request more memory from the kernel when the heap is full.
- Alignment:
  - The allocator aligns the blocks to the word size of the CPU.
- Platform Compatibility:
  - The allocator works on 64-bit, 32-bit and smaller CPUs on UNIX-like systems.
