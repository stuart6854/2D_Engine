#pragma once

#include "types.hpp"

namespace app::core
{
    struct AllocationMetrics
    {
        sizet TotalAllocated = 0;
        sizet TotalFreed = 0;

        sizet CurrentUsage() const
        {
            return TotalAllocated - TotalFreed;
        }
    };

    auto GetAllocationMetrics() -> const AllocationMetrics&;
}

void* operator new(sizet size);
void operator delete(void* memory, sizet size);