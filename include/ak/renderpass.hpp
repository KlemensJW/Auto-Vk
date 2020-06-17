#pragma once

namespace ak
{
	/** A class which represents a Vulkan renderpass and all its attachments,
	 *	subpasses and subpass dependencies
	 */
	class renderpass_t
	{
		struct subpass_data
		{
			subpass_data() = default;
			subpass_data(subpass_data&&) noexcept = default;
			subpass_data(const subpass_data&) = delete;
			subpass_data& operator=(subpass_data&&) noexcept = default;
			subpass_data& operator=(const subpass_data&) = delete;
			~subpass_data() = default;
			
			// Ordered list of input attachments
			std::vector<vk::AttachmentReference> mOrderedInputAttachmentRefs;

			// The ordered list of color attachments (ordered by shader-location).
			std::vector<vk::AttachmentReference> mOrderedColorAttachmentRefs;

			// The ordered list of depth attachments. Actually, only one or zero are supported.
			std::vector<vk::AttachmentReference> mOrderedDepthStencilAttachmentRefs;

			// The ordered list of attachments that shall be resolved.
			// The length of this list must be zero or the same length as the color attachments.
			std::vector<vk::AttachmentReference> mOrderedResolveAttachmentRefs;

			// The list of attachments that are to be preserved
			std::vector<uint32_t> mPreserveAttachments;
		};
		
	public:
		renderpass_t() = default;
		renderpass_t(renderpass_t&&) noexcept = default;
		renderpass_t(const renderpass_t&) = delete;
		renderpass_t& operator=(renderpass_t&&) noexcept = default;
		renderpass_t& operator=(const renderpass_t&) = delete;
		~renderpass_t() = default;

		/** Create a renderpass from a given set of attachments.
		 *	Also, create default subpass dependencies (which are overly cautious and potentially sync more than required.)
		 *	To specify custom subpass dependencies, pass a callback to the second parameter!
		 *	@param	aAttachments				Attachments of the renderpass to be created
		 *	@param	aSync						Callback of type void(renderpass_sync&) that is invoked for external subpass dependencies (before and after),
		 *										and also between each of the subpasses. Modify the passed `renderpass_sync&` in order to set custom
		 *										synchronization parameters.
		 *	@param	aAlterConfigBeforeCreation	Use it to alter the renderpass_t configuration before it is actually being created.
		 */
		static ak::owning_resource<renderpass_t> create(std::vector<ak::attachment> aAttachments, std::function<void(renderpass_sync&)> aSync = {}, std::function<void(renderpass_t&)> aAlterConfigBeforeCreation = {});

		const auto& attachment_descriptions() const { return mAttachmentDescriptions; }
		const auto& clear_values() const { return mClearValues; }

		const auto& subpasses() const { return mSubpasses; }
		const auto& subpass_dependencies() const { return mSubpassDependencies; }

		auto& attachment_descriptions() { return mAttachmentDescriptions; }
		auto& subpasses() { return mSubpasses; }
		auto& subpass_dependencies() { return mSubpassDependencies; }

		bool is_input_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;
		bool is_color_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;
		bool is_depth_stencil_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;
		bool is_resolve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;
		bool is_preserve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const;

		const std::vector<vk::AttachmentReference>& input_attachments_for_subpass(uint32_t aSubpassId);
		const std::vector<vk::AttachmentReference>& color_attachments_for_subpass(uint32_t aSubpassId);
		const std::vector<vk::AttachmentReference>& depth_stencil_attachments_for_subpass(uint32_t aSubpassId);
		const std::vector<vk::AttachmentReference>& resolve_attachments_for_subpass(uint32_t aSubpassId);
		const std::vector<uint32_t>& preserve_attachments_for_subpass(uint32_t aSubpassId);

		const auto& handle() const { return mRenderPass.get(); }

	private:
		// All the attachments to this renderpass
		std::vector<vk::AttachmentDescription> mAttachmentDescriptions;

		// All the clear values
		std::vector<vk::ClearValue> mClearValues;

		// Subpass data
		std::vector<subpass_data> mSubpassData;

		// Subpass descriptions
		std::vector<vk::SubpassDescription> mSubpasses;

		// Dependencies between internal and external subpasses
		std::vector<vk::SubpassDependency> mSubpassDependencies;

		// The native handle
		vk::UniqueRenderPass mRenderPass;
	};

	using renderpass = ak::owning_resource<renderpass_t>;
	
}