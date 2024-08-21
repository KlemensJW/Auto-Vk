#pragma once
#include "avk/avk.hpp"

namespace avk
{
	/**	Class handling the lifetime of one VMA allocation.
	 *	Also provides some convenience methods.
	 */
	template <typename T>
	struct vma_handle
	{
		/** Construct emptyness */
		vma_handle() : mAllocator{nullptr}, mCreateInfo{}, mAllocation{nullptr}, mAllocationInfo{}, mResource{nullptr}
		{ }

		/** Initialize with VMA structs and the already created resource. */
		vma_handle(VmaAllocator aAllocator, VmaAllocationCreateInfo aAllocInfo, VmaAllocation aAlloc, T aResource)
			: mAllocator{ aAllocator }
			, mCreateInfo{ std::move(aAllocInfo) }
			, mAllocation{ std::move(aAlloc) }
			, mResource{ std::move(aResource) }
		{
			vmaGetAllocationInfo(mAllocator, mAllocation, &mAllocationInfo);
		}

		/**	Create VmaAllocator, VmaAllocationCreateInfo, and VmaAllocation internally.
		 *	This is only implemented for certain types via template specialization: vk::Buffer, vk::Image
		 */
		template <typename C>
		vma_handle(VmaAllocator aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const C& aResourceCreateInfo);

		/**	Create VmaAllocator, VmaAllocationCreateInfo, and VmaAllocation internally. Uses a custom memory pool
		 *	This is only implemented for certain types via template specialization: vk::Buffer, vk::Image
		 */
		template <typename C>
		vma_handle(VmaAllocator aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const C& aResourceCreateInfo, VmaPool aPool);
		
		/** Move-construct a vma_handle */
		vma_handle(vma_handle&& aOther) noexcept : mAllocator{nullptr}, mCreateInfo{}, mAllocation{nullptr}, mAllocationInfo{}, mResource{nullptr}
		{
			std::swap(mAllocator,      aOther.mAllocator);
			std::swap(mCreateInfo,     aOther.mCreateInfo);
			std::swap(mAllocation,     aOther.mAllocation);
			std::swap(mAllocationInfo, aOther.mAllocationInfo);
			std::swap(mResource,       aOther.mResource);
		}

		vma_handle(const vma_handle& aOther) = delete;
		
		/** Move-assign a vma_handle */
		vma_handle& operator=(vma_handle&& aOther) noexcept
		{
			std::swap(mAllocator,      aOther.mAllocator);
			std::swap(mCreateInfo,     aOther.mCreateInfo);
			std::swap(mAllocation,     aOther.mAllocation);
			std::swap(mAllocationInfo, aOther.mAllocationInfo);
			std::swap(mResource,       aOther.mResource);
			return *this;
		}

		vma_handle& operator=(const vma_handle& aOther) = delete;

		/** Destroy the resource and free the allocation
		 *	This is only implemented for certain types via template specialization: vk::Buffer, vk::Image
		 *	That also means that this type is only usable with certain resource types.
		 */
		~vma_handle();

		/** Get the allocator that was used to allocate this resource */
		auto allocator() const
		{
			return mAllocator;
		}
		
		/** Get the config a.k.a. create-info that was used to allocate this resource. */
		auto create_info() const
		{
			return mCreateInfo;
		}

		/** Get the VMA-handle of this resource's allocation. */
		auto allocation() const
		{
			return mAllocation;
		}

		/** Get the resource handle. */
		T resource() const
		{
			return mResource;
		}

		/** Get the memory properties from the allocation */
		vk::MemoryPropertyFlags memory_properties() const
		{
			VkMemoryPropertyFlags result;
			vmaGetMemoryTypeProperties(mAllocator, mAllocationInfo.memoryType, &result);
			return vk::MemoryPropertyFlags{ result };
		}

		std::tuple<vk::DeviceMemory, vk::DeviceSize> device_memory_and_offset() const
		{
			return {mAllocationInfo.deviceMemory, mAllocationInfo.offset};
		}

		/**	Map the memory in order to write data into, or read data from it.
		 *	If data shall be read from it and the memory is not host coherent, an invalidate-instruction will be issued.
		 *
		 *	Hint: Consider using avk::scoped_mapping instead of calling this method directly.
		 *
		 *	@param	aAccess		Specify your intent: Are you going to read from the memory, or write into it, or both?
		 *	@return	Pointer to the mapped memory.
		 */
		void* map_memory(mapping_access aAccess) const
		{
			const auto memProps = memory_properties();
			assert(has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)); // => Allocation ended up in mappable memory. You can map it and access it directly.

			void* mappedData;
		    auto result = vmaMapMemory(mAllocator, mAllocation, &mappedData);
			assert(result >= 0);
			
			if (has_flag(aAccess, mapping_access::read) && !has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCoherent)) {
				result = vmaInvalidateAllocation(mAllocator, mAllocation, 0, VK_WHOLE_SIZE);
				assert(result >= 0);
			}
			
			return mappedData;
		}

		/**	Unmap memory that has been mapped before via mem_handle::map_memory.
		 *	If data shall be written to it and the memory is not host coherent, a flush-instruction will be issued.
		 *
		 *	Hint: Consider using avk::scoped_mapping instead of calling this method directly.
		 *
		 *	@param	aAccess		Specify your intent: Are you going to read from the memory, or write into it, or both?
		 */
		void unmap_memory(mapping_access aAccess) const
		{
			const auto memProps = memory_properties();
			assert(has_flag(memProps, vk::MemoryPropertyFlagBits::eHostVisible)); // => Allocation ended up in mappable memory. You can map it and access it directly.

			if (has_flag(aAccess, mapping_access::write) && !has_flag(memProps, vk::MemoryPropertyFlagBits::eHostCoherent)) {
				VkResult result = vmaFlushAllocation(mAllocator, mAllocation, 0, VK_WHOLE_SIZE);
				assert(result >= 0);
			}
			
		    vmaUnmapMemory(mAllocator, mAllocation);
		}

		VmaAllocator mAllocator;
		VmaAllocationCreateInfo mCreateInfo;
		VmaAllocation mAllocation;
		VmaAllocationInfo mAllocationInfo;
		T mResource;
	};

	// Fail if not used with either vk::Buffer or vk::Image
	template <typename T>
	template <typename C>
	vma_handle<T>::vma_handle(VmaAllocator aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const C& aResourceCreateInfo)
	{
		throw avk::runtime_error(std::string("VMA allocation not implemented for type ") + typeid(T).name());
	}

	template<typename T>
	template<typename C>
	inline vma_handle<T>::vma_handle(VmaAllocator aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const C& aResourceCreateInfo, VmaPool aPool)
	{
		throw avk::runtime_error(std::string("VMA allocation not implemented for type ") + typeid(T).name());
	}

	// Constructor's template specialization for vk::Buffer
	template <>
	template <>
	inline vma_handle<vk::Buffer>::vma_handle(VmaAllocator aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const vk::BufferCreateInfo& aResourceCreateInfo)
		: mAllocator{ aAllocator }
		, mCreateInfo{}, mAllocation{nullptr}, mAllocationInfo{}
	{
		mCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(aMemPropFlags);
		mCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;

		VkBuffer buffer;
		auto result = vmaCreateBuffer(aAllocator, &static_cast<const VkBufferCreateInfo&>(aResourceCreateInfo), &mCreateInfo, &buffer, &mAllocation, &mAllocationInfo);
		assert(result >= 0);
		mResource = buffer;
	}

	// Constructor's template specialization for vk::Buffer
	template <>
	template <>
	inline vma_handle<vk::Buffer>::vma_handle(VmaAllocator aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const vk::BufferCreateInfo& aResourceCreateInfo, VmaPool aPool)
		: mAllocator{ aAllocator }
		, mCreateInfo{}, mAllocation{ nullptr }, mAllocationInfo{}
	{
		mCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(aMemPropFlags);
		mCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;
		mCreateInfo.pool = aPool;

		VkBuffer buffer;
		auto result = vmaCreateBuffer(aAllocator, &static_cast<const VkBufferCreateInfo&>(aResourceCreateInfo), &mCreateInfo, &buffer, &mAllocation, &mAllocationInfo);
		assert(result >= 0);
		mResource = buffer;
	}
	
	// Constructor's template specialization for vk::Image
	template <>
	template <>
	inline vma_handle<vk::Image>::vma_handle(VmaAllocator aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const vk::ImageCreateInfo& aResourceCreateInfo)
		: mAllocator{ aAllocator }
		, mCreateInfo{}, mAllocation{nullptr}, mAllocationInfo{}
	{
		mCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(aMemPropFlags);
		mCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;

		VkImage image;
		auto result = vmaCreateImage(aAllocator, &static_cast<const VkImageCreateInfo&>(aResourceCreateInfo), &mCreateInfo, &image, &mAllocation, &mAllocationInfo);
		assert(result >= 0);
		mResource = image;
	}

	// Constructor's template specialization for vk::Image
	template <>
	template <>
	inline vma_handle<vk::Image>::vma_handle(VmaAllocator aAllocator, vk::MemoryPropertyFlags aMemPropFlags, const vk::ImageCreateInfo& aResourceCreateInfo, VmaPool aPool)
		: mAllocator{ aAllocator }
		, mCreateInfo{}, mAllocation{nullptr}, mAllocationInfo{}
	{
		mCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(aMemPropFlags);
		mCreateInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;
		mCreateInfo.pool = aPool;

		VkImage image;
		auto result = vmaCreateImage(aAllocator, &static_cast<const VkImageCreateInfo&>(aResourceCreateInfo), &mCreateInfo, &image, &mAllocation, &mAllocationInfo);
		assert(result >= 0);
		mResource = image;
	}

	// Fail if not used with either vk::Buffer or vk::Image
	template <typename T>
	vma_handle<T>::~vma_handle()
	{
		throw avk::runtime_error(std::string("VMA allocation not implemented for type ") + typeid(T).name());
	}

	// Destructor's template specialization for vk::Buffer
	template <>
	inline vma_handle<vk::Buffer>::~vma_handle()
	{
		if (static_cast<bool>(mResource)) {
			vmaDestroyBuffer(mAllocator, static_cast<VkBuffer>(mResource), mAllocation);
			mAllocator = nullptr;
			mCreateInfo = {};
			mAllocation = nullptr;
			mAllocationInfo = {};
			mResource = nullptr;
		}
	}

	// Destructor's template specialzation for vk::Image:
	template <>
	inline vma_handle<vk::Image>::~vma_handle()
	{
		if (static_cast<bool>(mResource)) {
			vmaDestroyImage(mAllocator, static_cast<VkImage>(mResource), mAllocation);
			mAllocator = nullptr;
			mCreateInfo = {};
			mAllocation = nullptr;
			mAllocationInfo = {};
			mResource = nullptr;
		}
	}

	inline VmaPool alloc_custom_pool(VmaAllocator aAllocator, const VmaPoolCreateInfo* aPoolCreateInfo)
	{
		/*
		VmaPoolCreateInfo pool_create_info = {};
		pool_create_info.memoryTypeIndex = ;
		pool_create_info.flags = ;
		pool_create_info.blockSize = 0; // default
		pool_create_info.minBlockCount = 0; // default
		pool_create_info.maxBlockCount = 0;	// default
		pool_create_info.priority = 0; // ignored unless VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT was used during creation of the VmaAllocator object
		pool_create_info.minAllocationAlignment = 0; // default, though it is mentioned that i might need different alignment for interop with OpenGL so maybe also for interop with CUDA
		pool_create_info.pMemoryAllocateNext = aAllocate_info;
		*/

		VmaPool pool;
		vmaCreatePool(aAllocator, aPoolCreateInfo, &pool);
		return pool;
	}

}
