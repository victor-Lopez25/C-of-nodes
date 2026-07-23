#ifndef STB_SLICEHM_H
#define STB_SLICEHM_H

#include "stb_ds.h"

#define slicehm_free_func(a, elemsize) stbds_hmfree_func(a, elemsize)
extern void *slicehm_get_key(void *a, size_t elemsize, void *data);
extern void *slicehm_get_key_ts(void *a, size_t elemsize, void *data, ptrdiff_t *temp);
#define slicehm_put_default(a, elemsize) stbds_hmput_default(a, elemsize)
extern void *slicehm_put_key(void *a, size_t elemsize, void *key);
extern void *slicehm_del_key(void *a, size_t elemsize, void *key, size_t keyoffset);

#define slicehm_put(t, k, v) \
  ((t) = slicehm_put_key_wrapper((t), sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k))),   \
    (t)[stbds_temp((t)-1)].key = (k),    \
    (t)[stbds_temp((t)-1)].value = (v))

#define slicehm_puts(t, s) \
  ((t) = slicehm_put_key_wrapper((t), sizeof *(t), &(s).key), \
    (t)[stbds_temp((t)-1)] = (s))

#define slicehm_geti(t,k) \
  ((t) = slicehm_get_key_wrapper((t), sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k))), \
    stbds_temp((t)-1))

#define slicehm_geti_ts(t,k,temp) \
  ((t) = slicehm_get_key_ts_wrapper((t), sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k)), &(temp)), \
    (temp))

#define slicehm_getp(t, k) \
  ((void) slicehm_geti(t,k), &(t)[stbds_temp((t)-1)])

#define slicehm_getp_ts(t, k, temp) \
    ((void) slicehm_geti_ts(t,k,temp), &(t)[temp])

#define slicehm_del(t,k) \
  (((t) = slicehm_del_key_wrapper((t),sizeof *(t), (void*) STBDS_ADDRESSOF((t)->key, (k)), STBDS_OFFSETOF((t),key))), \
    (t) ? stbds_temp((t)-1) : 0)

#define slicehm_default(t, v) \
  ((t) = slicehm_put_default_wrapper((t), sizeof *(t)), \
    (t)[-1].value = (v))

#define slicehm_defaults(t, s) \
  ((t) = slicehm_put_default_wrapper((t), sizeof *(t)), \
    (t)[-1] = (s))

#define slicehm_free(p)        \
  ((void) ((p) != NULL ? slicehm_free_func((p)-1,sizeof*(p)),0 : 0),(p)=NULL)

#define slicehm_gets(t, k)    (*slicehm_getp(t,k))
#define slicehm_get(t, k)     (slicehm_getp(t,k)->value)
#define slicehm_get_ts(t, k, temp)  (slicehm_getp_ts(t,k,temp)->value)
#define slicehm_len(t)        ((t) ? (ptrdiff_t) stbds_header((t)-1)->length-1 : 0)
#define slicehm_lenu(t)       ((t) ?             stbds_header((t)-1)->length-1 : 0)
#define slicehm_getp_null(t,k)  (slicehm_geti(t,k) == -1 ? NULL : &(t)[stbds_temp((t)-1)])

#ifdef __cplusplus
// in C we use implicit assignment from these void*-returning functions to T*.
// in C++ these templates make the same code work
template<class T> static T * slice_arrgrowf_wrapper(T *a, size_t elemsize, size_t addlen, size_t min_cap) {
  return (T*)slice_arrgrowf((void *)a, elemsize, addlen, min_cap);
}
template<class T> static T * slicehm_get_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, int mode) {
  return (T*)slicehm_get_key((void*)a, elemsize, key, keysize, mode);
}
template<class T> static T * slicehm_get_key_ts_wrapper(T *a, size_t elemsize, void *key, size_t keysize, ptrdiff_t *temp, int mode) {
  return (T*)slicehm_get_key_ts((void*)a, elemsize, key, keysize, temp, mode);
}
template<class T> static T * slicehm_put_default_wrapper(T *a, size_t elemsize) {
  return (T*)slicehm_put_default((void *)a, elemsize);
}
template<class T> static T * slicehm_put_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, int mode) {
  return (T*)slicehm_put_key((void*)a, elemsize, key, keysize, mode);
}
template<class T> static T * slicehm_del_key_wrapper(T *a, size_t elemsize, void *key, size_t keysize, size_t keyoffset, int mode){
  return (T*)slicehm_del_key((void*)a, elemsize, key, keysize, keyoffset, mode);
}
template<class T> static T * slice_shmode_func_wrapper(T *, size_t elemsize, int mode) {
  return (T*)slice_shmode_func(elemsize, mode);
}
#else
#define slice_arrgrowf_wrapper            slice_arrgrowf
#define slicehm_get_key_wrapper           slicehm_get_key
#define slicehm_get_key_ts_wrapper        slicehm_get_key_ts
#define slicehm_put_default_wrapper       slicehm_put_default
#define slicehm_put_key_wrapper           slicehm_put_key
#define slicehm_del_key_wrapper           slicehm_del_key
#define slice_shmode_func_wrapper(t,e,m)  slice_shmode_func(e,m)
#endif

#ifdef STB_SLICEHM_IMPLEMENTATION
#include <assert.h>
#include <string.h>

#ifndef SLICEHM_ASSERT
#define SLICEHM_ASSERT_WAS_UNDEFINED
#define SLICEHM_ASSERT(x)   ((void) 0)
#endif

typedef struct {
  void *ptr;
  size_t len;
} slicehm_key_internal;

#ifdef SLICEHM_STATISTICS
#define SLICEHM_STATS(x)   x
size_t slicehm_array_grow;
size_t slicehm_hash_grow;
size_t slicehm_hash_shrink;
size_t slicehm_hash_rebuild;
size_t slicehm_hash_probes;
size_t slicehm_hash_alloc;
size_t slicehm_rehash_probes;
size_t slicehm_rehash_items;
#else
#define SLICEHM_STATS(x)
#endif

#ifdef SLICEHM_INTERNAL_SMALL_BUCKET
#define SLICEHM_BUCKET_LENGTH      4
#else
#define SLICEHM_BUCKET_LENGTH      8
#endif

#define SLICEHM_BUCKET_SHIFT      (SLICEHM_BUCKET_LENGTH == 8 ? 3 : 2)
#define SLICEHM_BUCKET_MASK       (SLICEHM_BUCKET_LENGTH-1)
#define SLICEHM_CACHE_LINE_SIZE   64

#define SLICEHM_ALIGN_FWD(n,a)   (((n) + (a) - 1) & ~((a)-1))

typedef struct
{
   size_t    hash [SLICEHM_BUCKET_LENGTH];
   ptrdiff_t index[SLICEHM_BUCKET_LENGTH];
} slice_hash_bucket; // in 32-bit, this is one 64-byte cache line; in 64-bit, each array is one 64-byte cache line

typedef struct
{
  char * temp_key; // this MUST be the first field of the hash table
  size_t slot_count;
  size_t used_count;
  size_t used_count_threshold;
  size_t used_count_shrink_threshold;
  size_t tombstone_count;
  size_t tombstone_count_threshold;
  size_t seed;
  size_t slot_count_log2;
  stbds_string_arena string;
  slice_hash_bucket *storage; // not a separate allocation, just 64-byte aligned storage after this struct
} slice_hash_index;

#define SLICEHM_INDEX_EMPTY    -1
#define SLICEHM_INDEX_DELETED  -2
#define SLICEHM_INDEX_IN_USE(x)  ((x) >= 0)

#define SLICEHM_HASH_EMPTY      0
#define SLICEHM_HASH_DELETED    1

static size_t slice_hash_seed=0x31415926;

void slice_rand_seed(size_t seed)
{
  slice_hash_seed = seed;
}

#define slicehm_load_32_or_64(var, temp, v32, v64_hi, v64_lo)                                          \
  temp = v64_lo ^ v32, temp <<= 16, temp <<= 16, temp >>= 16, temp >>= 16, /* discard if 32-bit */   \
  var = v64_hi, var <<= 16, var <<= 16,                                    /* discard if 32-bit */   \
  var ^= temp ^ v32

#define SLICEHM_SIZE_T_BITS           ((sizeof (size_t)) * 8)

static size_t slicehm_probe_position(size_t hash, size_t slot_count, size_t slot_log2)
{
  size_t pos;
  STBDS_NOTUSED(slot_log2);
  pos = hash & (slot_count-1);
  #ifdef STBDS_INTERNAL_BUCKET_START
  pos &= ~SLICE_BUCKET_MASK;
  #endif
  return pos;
}

static size_t slicehm_log2(size_t slot_count)
{
  size_t n=0;
  while (slot_count > 1) {
    slot_count >>= 1;
    ++n;
  }
  return n;
}

static slice_hash_index *slicehm_make_hash_index(size_t slot_count, slice_hash_index *ot)
{
  slice_hash_index *t;
  t = (slice_hash_index *) STBDS_REALLOC(NULL,0,(slot_count >> SLICEHM_BUCKET_SHIFT) * sizeof(slice_hash_bucket) + sizeof(slice_hash_index) + SLICEHM_CACHE_LINE_SIZE-1);
  t->storage = (slice_hash_bucket *) SLICEHM_ALIGN_FWD((size_t) (t+1), SLICEHM_CACHE_LINE_SIZE);
  t->slot_count = slot_count;
  t->slot_count_log2 = slicehm_log2(slot_count);
  t->tombstone_count = 0;
  t->used_count = 0;

  #if 0 // A1
  t->used_count_threshold        = slot_count*12/16; // if 12/16th of table is occupied, grow
  t->tombstone_count_threshold   = slot_count* 2/16; // if tombstones are 2/16th of table, rebuild
  t->used_count_shrink_threshold = slot_count* 4/16; // if table is only 4/16th full, shrink
  #elif 1 // A2
  //t->used_count_threshold        = slot_count*12/16; // if 12/16th of table is occupied, grow
  //t->tombstone_count_threshold   = slot_count* 3/16; // if tombstones are 3/16th of table, rebuild
  //t->used_count_shrink_threshold = slot_count* 4/16; // if table is only 4/16th full, shrink

  // compute without overflowing
  t->used_count_threshold        = slot_count - (slot_count>>2);
  t->tombstone_count_threshold   = (slot_count>>3) + (slot_count>>4);
  t->used_count_shrink_threshold = slot_count >> 2;

  #elif 0 // B1
  t->used_count_threshold        = slot_count*13/16; // if 13/16th of table is occupied, grow
  t->tombstone_count_threshold   = slot_count* 2/16; // if tombstones are 2/16th of table, rebuild
  t->used_count_shrink_threshold = slot_count* 5/16; // if table is only 5/16th full, shrink
  #else // C1
  t->used_count_threshold        = slot_count*14/16; // if 14/16th of table is occupied, grow
  t->tombstone_count_threshold   = slot_count* 2/16; // if tombstones are 2/16th of table, rebuild
  t->used_count_shrink_threshold = slot_count* 6/16; // if table is only 6/16th full, shrink
  #endif
  // Following statistics were measured on a Core i7-6700 @ 4.00Ghz, compiled with clang 7.0.1 -O2
    // Note that the larger tables have high variance as they were run fewer times
  //     A1            A2          B1           C1
  //    0.10ms :     0.10ms :     0.10ms :     0.11ms :      2,000 inserts creating 2K table
  //    0.96ms :     0.95ms :     0.97ms :     1.04ms :     20,000 inserts creating 20K table
  //   14.48ms :    14.46ms :    10.63ms :    11.00ms :    200,000 inserts creating 200K table
  //  195.74ms :   196.35ms :   203.69ms :   214.92ms :  2,000,000 inserts creating 2M table
  // 2193.88ms :  2209.22ms :  2285.54ms :  2437.17ms : 20,000,000 inserts creating 20M table
  //   65.27ms :    53.77ms :    65.33ms :    65.47ms : 500,000 inserts & deletes in 2K table
  //   72.78ms :    62.45ms :    71.95ms :    72.85ms : 500,000 inserts & deletes in 20K table
  //   89.47ms :    77.72ms :    96.49ms :    96.75ms : 500,000 inserts & deletes in 200K table
  //   97.58ms :    98.14ms :    97.18ms :    97.53ms : 500,000 inserts & deletes in 2M table
  //  118.61ms :   119.62ms :   120.16ms :   118.86ms : 500,000 inserts & deletes in 20M table
  //  192.11ms :   194.39ms :   196.38ms :   195.73ms : 500,000 inserts & deletes in 200M table

  if (slot_count <= SLICEHM_BUCKET_LENGTH)
    t->used_count_shrink_threshold = 0;
  // to avoid infinite loop, we need to guarantee that at least one slot is empty and will terminate probes
  STBDS_ASSERT(t->used_count_threshold + t->tombstone_count_threshold < t->slot_count);
  STBDS_STATS(++stbds_hash_alloc);
  if (ot) {
    t->string = ot->string;
    // reuse old seed so we can reuse old hashes so below "copy out old data" doesn't do any hashing
    t->seed = ot->seed;
  } else {
    size_t a,b,temp;
    memset(&t->string, 0, sizeof(t->string));
    t->seed = stbds_hash_seed;
    // LCG
    // in 32-bit, a =          2147001325   b =  715136305
    // in 64-bit, a = 2862933555777941757   b = 3037000493
    slicehm_load_32_or_64(a,temp, 2147001325, 0x27bb2ee6, 0x87b0b0fd);
    slicehm_load_32_or_64(b,temp,  715136305,          0, 0xb504f32d);
    stbds_hash_seed = stbds_hash_seed  * a + b;
  }

  {
    size_t i,j;
    for (i=0; i < slot_count >> SLICEHM_BUCKET_SHIFT; ++i) {
      slice_hash_bucket *b = &t->storage[i];
      for (j=0; j < SLICEHM_BUCKET_LENGTH; ++j)
        b->hash[j] = STBDS_HASH_EMPTY;
      for (j=0; j < SLICEHM_BUCKET_LENGTH; ++j)
        b->index[j] = STBDS_INDEX_EMPTY;
    }
  }

  // copy out the old data, if any
  if (ot) {
    size_t i,j;
    t->used_count = ot->used_count;
    for (i=0; i < ot->slot_count >> SLICEHM_BUCKET_SHIFT; ++i) {
      slice_hash_bucket *ob = &ot->storage[i];
      for (j=0; j < SLICEHM_BUCKET_LENGTH; ++j) {
        if (SLICEHM_INDEX_IN_USE(ob->index[j])) {
          size_t hash = ob->hash[j];
          size_t pos = slicehm_probe_position(hash, t->slot_count, t->slot_count_log2);
          size_t step = SLICEHM_BUCKET_LENGTH;
          SLICEHM_STATS(++stbds_rehash_items);
          for (;;) {
            size_t limit,z;
            slice_hash_bucket *bucket;
            bucket = &t->storage[pos >> SLICEHM_BUCKET_SHIFT];
            SLICEHM_STATS(++stbds_rehash_probes);

            for (z=pos & SLICEHM_BUCKET_MASK; z < SLICEHM_BUCKET_LENGTH; ++z) {
              if (bucket->hash[z] == 0) {
                bucket->hash[z] = hash;
                bucket->index[z] = ob->index[j];
                goto done;
              }
            }

            limit = pos & SLICEHM_BUCKET_MASK;
            for (z = 0; z < limit; ++z) {
              if (bucket->hash[z] == 0) {
                bucket->hash[z] = hash;
                bucket->index[z] = ob->index[j];
                goto done;
              }
            }

            pos += step;                  // quadratic probing
            step += SLICEHM_BUCKET_LENGTH;
            pos &= (t->slot_count-1);
          }
        }
       done:
        ;
      }
    }
  }

  return t;
}

#define SLICEHM_ROTATE_LEFT(val, n)   (((val) << (n)) | ((val) >> (SLICEHM_SIZE_T_BITS - (n))))
#define SLICEHM_ROTATE_RIGHT(val, n)  (((val) >> (n)) | ((val) << (SLICEHM_SIZE_T_BITS - (n))))

#ifdef SLICEHM_SIPHASH_2_4
#define SLICEHM_SIPHASH_C_ROUNDS 2
#define SLICEHM_SIPHASH_D_ROUNDS 4
typedef int SLICEHM_SIPHASH_2_4_can_only_be_used_in_64_bit_builds[sizeof(size_t) == 8 ? 1 : -1];
#endif

#ifndef SLICEHM_SIPHASH_C_ROUNDS
#define SLICEHM_SIPHASH_C_ROUNDS 1
#endif
#ifndef SLICEHM_SIPHASH_D_ROUNDS
#define SLICEHM_SIPHASH_D_ROUNDS 1
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127) // conditional expression is constant, for do..while(0) and sizeof()==
#endif

#define slicehm_hash_bytes(p, len, seed) stbds_hash_bytes(p, len, seed)

static int slicehm_is_key_equal(void *a, size_t elemsize, void *key, size_t keyoffset, size_t i)
{
  slicehm_key_internal *key1 = (slicehm_key_internal*)key;
  slicehm_key_internal *key2 = (slicehm_key_internal*)((char *)a + elemsize*i + keyoffset);
  if(key1->len != key2->len) {
    return false;
  }
  return 0==memcmp(key1->ptr, key2->ptr, key1->len);
}

#define SLICEHM_HASH_TO_ARR(x,elemsize) ((char*) (x) - (elemsize))
#define SLICEHM_ARR_TO_HASH(x,elemsize) ((char*) (x) + (elemsize))

#define slice_hash_table(a)  ((slice_hash_index *) stbds_header(a)->hash_table)

static ptrdiff_t slicehm_find_slot(void *a, size_t elemsize, void *key, size_t keyoffset)
{
  void *raw_a = SLICEHM_HASH_TO_ARR(a,elemsize);
  slice_hash_index *table = slice_hash_table(raw_a);
  // void *data; size_t len; (key is always this structure)
  slicehm_key_internal *keyslice = (slicehm_key_internal*)key;
  size_t hash = slicehm_hash_bytes(keyslice->ptr, keyslice->len, table->seed);
  size_t step = SLICEHM_BUCKET_LENGTH;
  size_t limit,i;
  size_t pos;
  slice_hash_bucket *bucket;

  if (hash < 2) hash += 2; // stored hash values are forbidden from being 0, so we can detect empty slots

  pos = slicehm_probe_position(hash, table->slot_count, table->slot_count_log2);

  for (;;) {
    SLICEHM_STATS(++stbds_hash_probes);
    bucket = &table->storage[pos >> SLICEHM_BUCKET_SHIFT];

    // start searching from pos to end of bucket, this should help performance on small hash tables that fit in cache
    for (i=pos & SLICEHM_BUCKET_MASK; i < SLICEHM_BUCKET_LENGTH; ++i) {
      if (bucket->hash[i] == hash) {
        if (slicehm_is_key_equal(a, elemsize, key, keyoffset, bucket->index[i])) {
          return (pos & ~SLICEHM_BUCKET_MASK)+i;
        }
      } else if (bucket->hash[i] == STBDS_HASH_EMPTY) {
        return -1;
      }
    }

    // search from beginning of bucket to pos
    limit = pos & SLICEHM_BUCKET_MASK;
    for (i = 0; i < limit; ++i) {
      if (bucket->hash[i] == hash) {
        if (slicehm_is_key_equal(a, elemsize, key, keyoffset, bucket->index[i])) {
          return (pos & ~SLICEHM_BUCKET_MASK)+i;
        }
      } else if (bucket->hash[i] == STBDS_HASH_EMPTY) {
        return -1;
      }
    }

    // quadratic probing
    pos += step;
    step += SLICEHM_BUCKET_LENGTH;
    pos &= (table->slot_count-1);
  }
  /* NOTREACHED */
}

// slicehm_free_func is the same as stbds_hmfree_func

void * slicehm_get_key_ts(void *a, size_t elemsize, void *key, ptrdiff_t *temp)
{
  size_t keyoffset = 0;
  if (a == NULL) {
    // make it non-empty so we can return a temp
    a = stbds_arrgrowf(0, elemsize, 0, 1);
    stbds_header(a)->length += 1;
    memset(a, 0, elemsize);
    *temp = SLICEHM_INDEX_EMPTY;
    // adjust a to point after the default element
    return SLICEHM_ARR_TO_HASH(a,elemsize);
  } else {
    slice_hash_index *table;
    void *raw_a = SLICEHM_HASH_TO_ARR(a,elemsize);
    // adjust a to point to the default element
    table = (slice_hash_index *) stbds_header(raw_a)->hash_table;
    if (table == 0) {
      *temp = -1;
    } else {
      ptrdiff_t slot = slicehm_find_slot(a, elemsize, key, keyoffset);
      if (slot < 0) {
        *temp = SLICEHM_INDEX_EMPTY;
      } else {
        slice_hash_bucket *b = &table->storage[slot >> SLICEHM_BUCKET_SHIFT];
        *temp = b->index[slot & SLICEHM_BUCKET_MASK];
      }
    }
    return a;
  }
}

void * slicehm_get_key(void *a, size_t elemsize, void *key)
{
  ptrdiff_t temp;
  void *p = slicehm_get_key_ts(a, elemsize, key, &temp);
  stbds_temp(SLICEHM_HASH_TO_ARR(p,elemsize)) = temp;
  return p;
}

// slicehm_put_default is the same as stbds_hmput_default

void *slicehm_put_key(void *a, size_t elemsize, void *key)
{
  size_t keyoffset=0;
  void *raw_a;
  slice_hash_index *table;

  if (a == NULL) {
    a = stbds_arrgrowf(0, elemsize, 0, 1);
    memset(a, 0, elemsize);
    stbds_header(a)->length += 1;
    // adjust a to point AFTER the default element
    a = SLICEHM_ARR_TO_HASH(a,elemsize);
  }

  // adjust a to point to the default element
  raw_a = a;
  a = SLICEHM_HASH_TO_ARR(a,elemsize);

  table = (slice_hash_index *) stbds_header(a)->hash_table;

  if (table == NULL || table->used_count >= table->used_count_threshold) {
    slice_hash_index *nt;
    size_t slot_count;

    slot_count = (table == NULL) ? SLICEHM_BUCKET_LENGTH : table->slot_count*2;
    nt = slicehm_make_hash_index(slot_count, table);
    if (table)
      STBDS_FREE(NULL, table);
    else
      nt->string.mode = 0;
    stbds_header(a)->hash_table = table = nt;
    SLICEHM_STATS(++stbds_hash_grow);
  }

  // we iterate hash table explicitly because we want to track if we saw a tombstone
  {
    slicehm_key_internal *keyslice = (slicehm_key_internal*)key;
    size_t hash = slicehm_hash_bytes(keyslice->ptr, keyslice->len, table->seed);
    size_t step = SLICEHM_BUCKET_LENGTH;
    size_t pos;
    ptrdiff_t tombstone = -1;
    slice_hash_bucket *bucket;

    // stored hash values are forbidden from being 0, so we can detect empty slots to early out quickly
    if (hash < 2) hash += 2;

    pos = slicehm_probe_position(hash, table->slot_count, table->slot_count_log2);

    for (;;) {
      size_t limit, i;
      SLICEHM_STATS(++stbds_hash_probes);
      bucket = &table->storage[pos >> SLICEHM_BUCKET_SHIFT];

      // start searching from pos to end of bucket
      for (i=pos & SLICEHM_BUCKET_MASK; i < SLICEHM_BUCKET_LENGTH; ++i) {
        if (bucket->hash[i] == hash) {
          if (slicehm_is_key_equal(raw_a, elemsize, key, keyoffset, bucket->index[i])) {
            stbds_temp(a) = bucket->index[i];
            return SLICEHM_ARR_TO_HASH(a,elemsize);
          }
        } else if (bucket->hash[i] == 0) {
          pos = (pos & ~SLICEHM_BUCKET_MASK) + i;
          goto found_empty_slot;
        } else if (tombstone < 0) {
          if (bucket->index[i] == SLICEHM_INDEX_DELETED)
            tombstone = (ptrdiff_t) ((pos & ~SLICEHM_BUCKET_MASK) + i);
        }
      }

      // search from beginning of bucket to pos
      limit = pos & SLICEHM_BUCKET_MASK;
      for (i = 0; i < limit; ++i) {
        if (bucket->hash[i] == hash) {
          if (slicehm_is_key_equal(raw_a, elemsize, key, keyoffset, bucket->index[i])) {
            stbds_temp(a) = bucket->index[i];
            return SLICEHM_ARR_TO_HASH(a,elemsize);
          }
        } else if (bucket->hash[i] == 0) {
          pos = (pos & ~SLICEHM_BUCKET_MASK) + i;
          goto found_empty_slot;
        } else if (tombstone < 0) {
          if (bucket->index[i] == SLICEHM_INDEX_DELETED)
            tombstone = (ptrdiff_t) ((pos & ~SLICEHM_BUCKET_MASK) + i);
        }
      }

      // quadratic probing
      pos += step;
      step += SLICEHM_BUCKET_LENGTH;
      pos &= (table->slot_count-1);
    }
   found_empty_slot:
    if (tombstone >= 0) {
      pos = tombstone;
      --table->tombstone_count;
    }
    ++table->used_count;

    {
      ptrdiff_t i = (ptrdiff_t) stbds_arrlen(a);
      // we want to do stbds_arraddn(1), but we can't use the macros since we don't have something of the right type
      if ((size_t) i+1 > stbds_arrcap(a))
        *(void **) &a = stbds_arrgrowf(a, elemsize, 1, 0);
      raw_a = SLICEHM_ARR_TO_HASH(a,elemsize);

      SLICEHM_ASSERT((size_t) i+1 <= stbds_arrcap(a));
      stbds_header(a)->length = i+1;
      bucket = &table->storage[pos >> SLICEHM_BUCKET_SHIFT];
      bucket->hash[pos & SLICEHM_BUCKET_MASK] = hash;
      bucket->index[pos & SLICEHM_BUCKET_MASK] = i-1;
      stbds_temp(a) = i-1;

      switch (table->string.mode) {
         case STBDS_SH_STRDUP:  stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = stbds_strdup((char*) key); break;
         case STBDS_SH_ARENA:   stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = stbds_stralloc(&table->string, (char*)key); break;
         case STBDS_SH_DEFAULT: stbds_temp_key(a) = *(char **) ((char *) a + elemsize*i) = (char *) key; break;
         default:                memcpy((char *) a + elemsize*i, key, sizeof(void*) + sizeof(size_t)); break;
      }
    }
    return SLICEHM_ARR_TO_HASH(a,elemsize);
  }
}

void * slicehm_del_key(void *a, size_t elemsize, void *key, size_t keyoffset)
{
  if (a == NULL) {
    return 0;
  } else {
    slice_hash_index *table;
    void *raw_a = SLICEHM_HASH_TO_ARR(a,elemsize);
    table = (slice_hash_index *) stbds_header(raw_a)->hash_table;
    stbds_temp(raw_a) = 0;
    if (table == 0) {
      return a;
    } else {
      ptrdiff_t slot;
      slot = slicehm_find_slot(a, elemsize, key, keyoffset);
      if (slot < 0)
        return a;
      else {
        slice_hash_bucket *b = &table->storage[slot >> STBDS_BUCKET_SHIFT];
        int i = slot & STBDS_BUCKET_MASK;
        ptrdiff_t old_index = b->index[i];
        ptrdiff_t final_index = (ptrdiff_t) stbds_arrlen(raw_a)-1-1; // minus one for the raw_a vs a, and minus one for 'last'
        SLICEHM_ASSERT(slot < (ptrdiff_t) table->slot_count);
        --table->used_count;
        ++table->tombstone_count;
        stbds_temp(raw_a) = 1;
        SLICEHM_ASSERT(table->used_count >= 0);
        //SLICEHM_ASSERT(table->tombstone_count < table->slot_count/4);
        b->hash[i] = SLICEHM_HASH_DELETED;
        b->index[i] = SLICEHM_INDEX_DELETED;

        // if indices are the same, memcpy is a no-op, but back-pointer-fixup will fail, so skip
        if (old_index != final_index) {
          // swap delete
          memmove((char*) a + elemsize*old_index, (char*) a + elemsize*final_index, elemsize);

          // now find the slot for the last element
          slot = slicehm_find_slot(a, elemsize,  (char* ) a+elemsize*old_index + keyoffset, keyoffset);
          SLICEHM_ASSERT(slot >= 0);
          b = &table->storage[slot >> SLICEHM_BUCKET_SHIFT];
          i = slot & SLICEHM_BUCKET_MASK;
          SLICEHM_ASSERT(b->index[i] == final_index);
          b->index[i] = old_index;
        }
        stbds_header(raw_a)->length -= 1;

        if (table->used_count < table->used_count_shrink_threshold && table->slot_count > STBDS_BUCKET_LENGTH) {
          stbds_header(raw_a)->hash_table = slicehm_make_hash_index(table->slot_count>>1, table);
          STBDS_FREE(NULL, table);
          SLICEHM_STATS(++stbds_hash_shrink);
        } else if (table->tombstone_count > table->tombstone_count_threshold) {
          stbds_header(raw_a)->hash_table = slicehm_make_hash_index(table->slot_count   , table);
          STBDS_FREE(NULL, table);
          SLICEHM_STATS(++stbds_hash_rebuild);
        }

        return a;
      }
    }
  }
  /* NOTREACHED */
}


#endif
#endif // STB_SLICEHM_H


/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2019 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
