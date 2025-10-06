/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "pch.h"

#include "utils/Ref.h"

#include "framework/Game.h"

namespace tractor
{

#ifdef GP_USE_MEM_LEAK_DETECTION
void* trackRef(Ref* ref);
void untrackRef(Ref* ref, void* record);
#endif

Ref::Ref() : _refCount(1)
{
#ifdef GP_USE_MEM_LEAK_DETECTION
    __record = trackRef(this);
#endif
}

Ref::Ref(const Ref& copy) : _refCount(1)
{
#ifdef GP_USE_MEM_LEAK_DETECTION
    __record = trackRef(this);
#endif
}

Ref::~Ref() {}

void Ref::addRef()
{
    assert(_refCount > 0 && _refCount < 1000000);
    ++_refCount;
}

void Ref::release()
{
    assert(_refCount > 0 && _refCount < 1000000);
    if ((--_refCount) <= 0)
    {
#ifdef GP_USE_MEM_LEAK_DETECTION
        untrackRef(this, __record);
#endif
        delete this;
    }
}

#ifdef GP_USE_MEM_LEAK_DETECTION

struct RefAllocationRecord
{
    Ref* ref;
    RefAllocationRecord* next;
    RefAllocationRecord* prev;
};

RefAllocationRecord* __refAllocations = 0;
int __refAllocationCount = 0;

void Ref::printLeaks()
{
    // Dump Ref object memory leaks
    if (__refAllocationCount == 0)
    {
        print("[memory] All Ref objects successfully cleaned up (no leaks detected).\n");
    }
    else
    {
        print("[memory] WARNING: %d Ref objects still active in memory.\n", __refAllocationCount);
        for (RefAllocationRecord* rec = __refAllocations; rec != nullptr; rec = rec->next)
        {
            Ref* ref = rec->ref;
            assert(ref);
            const char* type = typeid(*ref).name();
            print("[memory] LEAK: Ref object '%s' still active with reference count %d.\n",
                  (type ? type : ""),
                  ref->getRefCount());
        }
    }
}

void* trackRef(Ref* ref)
{
    assert(ref);

    // Create memory allocation record.
    RefAllocationRecord* rec = (RefAllocationRecord*)malloc(sizeof(RefAllocationRecord));
    rec->ref = ref;
    rec->next = __refAllocations;
    rec->prev = 0;

    if (__refAllocations) __refAllocations->prev = rec;
    __refAllocations = rec;
    ++__refAllocationCount;

    return rec;
}

void untrackRef(Ref* ref, void* record)
{
    if (!record)
    {
        print("[memory] ERROR: Attempting to free null ref tracking record.\n");
        return;
    }

    RefAllocationRecord* rec = (RefAllocationRecord*)record;
    if (rec->ref != ref)
    {
        print("[memory] CORRUPTION: Attempting to free Ref with invalid ref tracking record.\n");
        return;
    }

    // Link this item out.
    if (__refAllocations == rec) __refAllocations = rec->next;
    if (rec->prev) rec->prev->next = rec->next;
    if (rec->next) rec->next->prev = rec->prev;
    free((void*)rec);
    --__refAllocationCount;
}

#endif

} // namespace tractor
