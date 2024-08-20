#pragma once
#ifdef AVK_USES_VMA
#include <vma/vk_mem_alloc.h>

namespace avk
{
    namespace vma_utils
    {
	    /** Allocates a custom memory pool for VMA
	     * 
	     * @param aAllocator        a VmaAllocator that will be used to allocate from this pool
	     * @param aPoolCreateInfo   a pointer to a VmaPoolCreateInfo struct that will be used to create the VmaPool
	     * @return 
	     */
	    inline VmaPool alloc_custom_pool(VmaAllocator aAllocator, VmaPoolCreateInfo* const aPoolCreateInfo)
        {
            VmaPool pool;
            VkResult result = vmaCreatePool(aAllocator, aPoolCreateInfo, &pool);
            
            assert(result >= 0);
            return pool;
        }
    }
}

#endif
