// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define MAX_SEARCH_RANGE 10

#define HASH(bno) ((bno) % NBUCKET)

struct {
  struct buf buf[NBUF];

  // bitmap[i] is set to 1 indicates that buf[i] is being used
  uint bitmap[NBUF];
} bcache;

struct bucket {
  struct spinlock lock;
  struct buf head;
} buckets[NBUCKET];

void
binit(void)
{
  // init bufs
  for (int i = 0; i < NBUF; i++) {
    bcache.bitmap[i] = 0;
    struct buf *b = bcache.buf + i;
    initsleeplock(&b->lock, "buffer");
  }

  // init buckets
  for (int i = 0; i < NBUCKET; i++) {
    struct bucket *bucket = buckets + i;
    initlock(&bucket->lock, "bcache bucket");
    struct buf *head = &bucket->head;
    head->next = head;
    head->prev = head;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint bid = HASH(blockno);
  struct bucket *bucket = buckets + bid;
  acquire(&bucket->lock);
  struct buf *head = &bucket->head;

  // Is the block already cached?
  for(b = head->next; b != head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bucket->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // search the bitmap to find a free buf
  uint free_idx = -1;
  for (uint i = 0; i < MAX_SEARCH_RANGE && free_idx == -1; i++) {
    for (uint j = 0; j < NBUF; j++) {
      if (__sync_lock_test_and_set(bcache.bitmap + j, 1) != 0) {
        continue;
      }
      free_idx = j;
      break;
    }
  }
  if (free_idx == -1) {
    panic("bget: no buffers");
  }

  b = bcache.buf + free_idx;
  if (b->refcnt != 0) {
    panic("bget: freelist invariant destroyed");
  }
  b->dev = dev;
  b->blockno = blockno;
  b->valid = 0;
  b->refcnt = 1;

  b->next = head->next;
  b->prev = head;
  b->next->prev = b;
  b->prev->next = b;
  release(&bucket->lock);
  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint bid = HASH(b->blockno);
  struct bucket *bucket = buckets + bid;
  acquire(&bucket->lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    uint bidx = ((uint64)b - (uint64)bcache.buf) / sizeof(struct buf);
    __sync_lock_test_and_set(bcache.bitmap + bidx, 0);
  }
  
  release(&bucket->lock);
}

void
bpin(struct buf *b) {
  uint bid = HASH(b->blockno);
  struct bucket *bucket = buckets + bid;
  acquire(&bucket->lock);
  b->refcnt++;
  release(&bucket->lock);
}

void
bunpin(struct buf *b) {
  uint bid = HASH(b->blockno);
  struct bucket *bucket = buckets + bid;
  acquire(&bucket->lock);
  b->refcnt--;
  release(&bucket->lock);
}


