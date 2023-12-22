// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_HEAP_NEW_SPACES_INL_H_
#define V8_HEAP_NEW_SPACES_INL_H_

#include "src/base/sanitizer/msan.h"
#include "src/common/globals.h"
#include "src/heap/heap.h"
#include "src/heap/new-spaces.h"
#include "src/heap/paged-spaces-inl.h"
#include "src/heap/spaces-inl.h"
#include "src/objects/objects-inl.h"
#include "src/objects/tagged-impl.h"
#include "src/objects/tagged.h"

namespace v8 {
namespace internal {

// -----------------------------------------------------------------------------
// SemiSpace

bool SemiSpace::Contains(Tagged<HeapObject> o) const {
  BasicMemoryChunk* memory_chunk = BasicMemoryChunk::FromHeapObject(o);
  if (memory_chunk->IsLargePage()) return false;
  return id_ == kToSpace ? memory_chunk->IsToPage()
                         : memory_chunk->IsFromPage();
}

bool SemiSpace::Contains(Tagged<Object> o) const {
  return IsHeapObject(o) && Contains(HeapObject::cast(o));
}

template <typename T>
inline bool SemiSpace::Contains(Tagged<T> o) const {
  static_assert(kTaggedCanConvertToRawObjects);
  return Contains(*o);
}

bool SemiSpace::ContainsSlow(Address a) const {
  for (const Page* p : *this) {
    if (p == BasicMemoryChunk::FromAddress(a)) return true;
  }
  return false;
}

// --------------------------------------------------------------------------
// NewSpace

bool NewSpace::Contains(Tagged<Object> o) const {
  return IsHeapObject(o) && Contains(HeapObject::cast(o));
}

bool NewSpace::Contains(Tagged<HeapObject> o) const {
  return BasicMemoryChunk::FromHeapObject(o)->InNewSpace();
}

V8_WARN_UNUSED_RESULT inline AllocationResult NewSpace::AllocateRawSynchronized(
    int size_in_bytes, AllocationAlignment alignment, AllocationOrigin origin) {
  base::MutexGuard guard(&mutex_);
  return AllocateRaw(size_in_bytes, alignment, origin);
}

// -----------------------------------------------------------------------------
// SemiSpaceObjectIterator

SemiSpaceObjectIterator::SemiSpaceObjectIterator(const SemiSpaceNewSpace* space)
    : current_(space->first_allocatable_address()) {}

Tagged<HeapObject> SemiSpaceObjectIterator::Next() {
  while (true) {
    if (Page::IsAlignedToPageSize(current_)) {
      Page* page = Page::FromAllocationAreaAddress(current_);
      page = page->next_page();
      if (page == nullptr) return Tagged<HeapObject>();
      current_ = page->area_start();
    }
    Tagged<HeapObject> object = HeapObject::FromAddress(current_);
    current_ += ALIGN_TO_ALLOCATION_ALIGNMENT(object->Size());
    if (!IsFreeSpaceOrFiller(object)) return object;
  }
}

}  // namespace internal
}  // namespace v8

#endif  // V8_HEAP_NEW_SPACES_INL_H_
