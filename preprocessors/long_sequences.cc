/**
 * @file long_sequences.cc
 * @author Pekka Mikkola <pjmikkol@cs.helsinki.fi>
 *
 * @section LICENSE
 *
 * This file is part of bwtc.
 *
 * bwtc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bwtc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bwtc.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 *
 * Implementation for detecting and replacing long repetitive sequences.
 */

#include "../globaldefs.h"
#include "long_sequences.h"
#include "longsequences.h"
#include "sequence_detector-inl.h"
#include "sequence_detector.h"
#include <algorithm>
#include <cassert>
#include <vector>

namespace bwtc {
namespace long_sequences {


/** Forms buckets, names positions in buckets to point
 *  pos_ordered vector (chunks)
 */
void FormBucketsAndNamePositions(std::vector<uint32> *names,
                                 std::vector<chunk> *chunks,
                                 std::vector<bucket_struct> *buckets)
{
  uint32 pos_in_chunks = 0;
  std::vector<uint32>& cum_pos = *names;
  std::vector<chunk>& buckets_ = *buckets;
  for(std::vector<chunk>::iterator it=chunks->begin(); it != chunks->end();
      ++it)
  {
    if(cum_pos[it->hash_value] != error_val)
      buckets_[cum_pos[it->hash_value]++] =
          bucket_struct(pos_in_chunks++, 37);
  }
}

uint32 CompressPositionOrdered(const std::vector<uint32>& bucket_starts,
                               std::vector<chunk> *pos_ordered)
{
  uint32 i = 0;
  std::vector<chunk>& chunks = *pos_ordered;
  while(i < chunks.size() && bucket_starts[chunks[i].hash_value] != error_val)
    ++i;
  uint32 next = i++;
  while(i < chunks.size()) {
    if(bucket_starts[chunks[i].hash_value] != error_val) 
      chunks[next++] = chunks[i];
    ++i;
  }
  return next;
}

/** Sorts entrys with the same hash value into ame bucket. At the same
 *  time renames the positions (in buckets) to point into corresponding
 *  entry of chunks-table. Also drops unused values at chunks. */
void SortIntoBuckets(std::vector<uint32> *hash_values,
                     std::vector<chunk> *chunks,
                     std::vector<bucket_struct> *buckets)
{
  uint32 total = 0;
  // Calculate cumulative frequencies into hash table
  for(std::vector<uint32>::iterator it = hash_values->begin();
      it != hash_values->end(); ++it)
  {
    if(*it > 1) {
      uint32 t = *it; *it = total; total += t;
    }
    else *it = error_val;
  }
  buckets->resize(total);
  FormBucketsAndNamePositions(hash_values, chunks, buckets);
  uint32 check_val = CompressPositionOrdered(*hash_values, chunks);
  assert(check_val == total);
  chunks->resize(total);
}

/* Following functions are used for packing additional information to
 * buckets-array. Namely they mark the first elements of the bucket.
 * TODO: write them as methods of bucket_struct
 */
uint32 GetPosition(const bucket_struct *b) {
  return b->position & 0x7FFFFFFF;
}

void SetBucketBeginFlag(bucket_struct *b) {
  b->position |= 0x80000000;
}

bool StartOfBucket(const bucket_struct *b) {
  return b->position & 0x80000000;
}

int StrEq(uint32 pos1, uint32 pos2, uint32 str_len, byte *from) {
  uint32 i = 0;
  while(i < str_len - 1 && from[pos1 + i] == from[pos2 + i]) ++i;
  return from[pos1 + i] - from[pos2 + i];
}

const int INSERTION_SORT_LIMIT = 10;

void InsertionSort(bucket_struct *begin, bucket_struct *end,
                   const std::vector<chunk>& pos_ordered,
                   byte *from, uint32 str_len)
{
  assert(end - begin > 0);
  SetBucketBeginFlag(begin);
  for(bucket_struct *i = begin + 1; i < end; ++i) {
    bucket_struct *j = i;
    bucket_struct val = *j;
    int cmp = 0;
    while(j > begin && (cmp = StrEq(pos_ordered[GetPosition(&val)].position,
                                    pos_ordered[GetPosition(j-1)].position,
                                    str_len, from)) < 0) {
      *j = *(j-1);
      --j;
    }
    *j = val;
    if(cmp > 0 || j == begin) SetBucketBeginFlag(j);
  }
}


void StringSort(bucket_struct *begin, bucket_struct *end,
                const std::vector<chunk>& pos_ordered,
                byte *from, uint32 str_len)
{
  int len = end - begin;
  if (len <= INSERTION_SORT_LIMIT) {
    if(len > 1) InsertionSort(begin, end, pos_ordered, from, str_len);
    else if (len == 1) SetBucketBeginFlag(begin);
    return;
  }
  bucket_struct *pivot = end-1;
  // First put values lesser than pivot in place 
  bucket_struct *i = begin - 1, *j = begin;
  for(; j < end - 1; ++j) {
    if(StrEq(pos_ordered[GetPosition(pivot)].position,
             pos_ordered[GetPosition(j)].position,str_len, from) > 0) {
      ++i;
      std::swap(*i, *j);
    }
  }
  StringSort(begin, i+1, pos_ordered, from, str_len);
  bucket_struct *sec = j = i + 1;
  for(; j < end - 1; ++j) {
    if(StrEq(pos_ordered[GetPosition(pivot)].position,
             pos_ordered[GetPosition(j)].position,str_len, from) == 0) {
      ++i;
      std::swap(*i, *j);
    }
  }
  ++i; std::swap(*i,*j); ++i;
  SetBucketBeginFlag(sec);
  StringSort(i, end, pos_ordered, from, str_len);
}
  

/* If pos is valid returns pos */
uint32 NextValidPos(const std::vector<uint32>& bucket_starts, uint32 pos) {
  while(pos < bucket_starts.size() && bucket_starts[pos] == error_val) ++pos;
  return pos;
}

void SortBuckets(byte *from, uint32 win_length,
                 const std::vector<uint32>& bucket_starts,
                 std::vector<bucket_struct> *buckets,
                 const std::vector<chunk>& pos_ordered)
{
  uint32 lo = 0; //NextValidPos(bucket_starts, 0);
  uint32 hi = NextValidPos(bucket_starts, 0);
  StringSort(&(*buckets)[0], &(*buckets)[bucket_starts[hi]],
             pos_ordered, from, win_length);
  lo = hi++;
  hi = NextValidPos(bucket_starts, hi);
  while(hi < bucket_starts.size()) {
    StringSort(&(*buckets)[bucket_starts[lo]], &(*buckets)[bucket_starts[hi]],
               pos_ordered, from, win_length);
    lo = hi++;
    hi = NextValidPos(bucket_starts, hi);
  }
}

#if 0
void HandleBuckets(byte *from, uint32 win_size,
                   std::vector<bucket_struct> *buckets,
                   std::vector<chunk> *pos_ordered)
{
  std::vector<chunk>& positions = *pos_ordered;
  std::vector<bucket_struct>& b = *buckets;
  uint32 lo = 0;
  assert(StartOfBucket(&b[lo]));
  while(lo < buckets->size()) {
    positions[GetPosition(&b[lo])].position = lo;
    uint32 hi = lo + 1;
    while(hi < buckets->size() && !StartOfBucket(&b[hi])) {
      positions[GetPosition(&b[hi])].position = lo;
      ++hi;
    }
    //HandleSingleBucket([lo.hi))
    lo = hi++;
  }
  
}
#endif

uint64 CompressSequences(byte *from, uint32 length, uint32 window_size) {
  assert(length < error_val);
  uint32 freqs[256] = {0};
  std::vector<chunk> pos_ordered;
  std::vector<uint32> *hash_counts = new std::vector<uint32>();
  /* Decide suggested size of hash table more intelligently */
  uint32 size_recommendation = length/(2*window_size);
  SequenceDetector<Hasher> seq_det(from, size_recommendation, freqs,
                                   &pos_ordered, hash_counts, window_size);
  seq_det.ScanAndStore(length);
  std::vector<bucket_struct> buckets;
  SortIntoBuckets(hash_counts, &pos_ordered, &buckets);
  assert(buckets.size() == pos_ordered.size());
  assert(buckets.size() < (Max<uint32>::max & 0x7FFFFFFF));
  SortBuckets(from, window_size, *hash_counts, &buckets, pos_ordered);
  delete hash_counts;
  //HandleBuckets(from, window_size, &buckets, &pos_ordered);

}

} // namespace long_sequences
} // namespace bwtc
