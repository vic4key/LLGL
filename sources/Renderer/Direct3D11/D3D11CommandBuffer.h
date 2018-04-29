/*
 * D3D11CommandBuffer.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2017 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef LLGL_D3D11_COMMAND_BUFFER_H
#define LLGL_D3D11_COMMAND_BUFFER_H


#include <LLGL/CommandBufferExt.h>
#include <cstddef>
#include "../DXCommon/ComPtr.h"
#include "../DXCommon/DXCore.h"
#include <vector>
#include <d3d11.h>
#include <dxgi.h>


namespace LLGL
{


class D3D11StateManager;
class D3D11RenderTarget;

class D3D11CommandBuffer : public CommandBufferExt
{

    public:

        /* ----- Common ----- */

        D3D11CommandBuffer(D3D11StateManager& stateMngr, const ComPtr<ID3D11DeviceContext>& context);

        /* ----- Configuration ----- */

        void SetGraphicsAPIDependentState(const GraphicsAPIDependentStateDescriptor& state) override;

        /* ----- Viewport and Scissor ----- */

        void SetViewport(const Viewport& viewport) override;
        void SetViewports(std::uint32_t numViewports, const Viewport* viewports) override;

        void SetScissor(const Scissor& scissor) override;
        void SetScissors(std::uint32_t numScissors, const Scissor* scissors) override;

        /* ----- Clear ----- */

        void SetClearColor(const ColorRGBAf& color) override;
        void SetClearDepth(float depth) override;
        void SetClearStencil(std::uint32_t stencil) override;

        void Clear(long flags) override;
        void ClearAttachments(std::uint32_t numAttachments, const AttachmentClear* attachments) override;

        /* ----- Input Assembly ------ */

        void SetVertexBuffer(Buffer& buffer) override;
        void SetVertexBufferArray(BufferArray& bufferArray) override;

        void SetIndexBuffer(Buffer& buffer) override;
        
        /* ----- Constant Buffers ------ */

        void SetConstantBuffer(Buffer& buffer, std::uint32_t slot, long shaderStageFlags = ShaderStageFlags::AllStages) override;
        void SetConstantBufferArray(BufferArray& bufferArray, std::uint32_t startSlot, long shaderStageFlags = ShaderStageFlags::AllStages) override;
        
        /* ----- Storage Buffers ------ */

        void SetStorageBuffer(Buffer& buffer, std::uint32_t slot, long shaderStageFlags = ShaderStageFlags::AllStages) override;
        void SetStorageBufferArray(BufferArray& bufferArray, std::uint32_t startSlot, long shaderStageFlags = ShaderStageFlags::AllStages) override;

        /* ----- Stream Output Buffers ------ */

        void SetStreamOutputBuffer(Buffer& buffer) override;
        void SetStreamOutputBufferArray(BufferArray& bufferArray) override;

        void BeginStreamOutput(const PrimitiveType primitiveType) override;
        void EndStreamOutput() override;

        /* ----- Textures ----- */

        void SetTexture(Texture& texture, std::uint32_t slot, long shaderStageFlags = ShaderStageFlags::AllStages) override;
        void SetTextureArray(TextureArray& textureArray, std::uint32_t startSlot, long shaderStageFlags = ShaderStageFlags::AllStages) override;

        /* ----- Sampler States ----- */

        void SetSampler(Sampler& sampler, std::uint32_t slot, long shaderStageFlags = ShaderStageFlags::AllStages) override;
        void SetSamplerArray(SamplerArray& samplerArray, std::uint32_t startSlot, long shaderStageFlags = ShaderStageFlags::AllStages) override;

        /* ----- Render Targets ----- */

        void SetRenderTarget(RenderTarget& renderTarget) override;
        void SetRenderTarget(RenderContext& renderContext) override;

        /* ----- Pipeline States ----- */

        void SetGraphicsPipeline(GraphicsPipeline& graphicsPipeline) override;
        void SetComputePipeline(ComputePipeline& computePipeline) override;

        /* ----- Queries ----- */

        void BeginQuery(Query& query) override;
        void EndQuery(Query& query) override;

        bool QueryResult(Query& query, std::uint64_t& result) override;
        bool QueryPipelineStatisticsResult(Query& query, QueryPipelineStatistics& result) override;

        void BeginRenderCondition(Query& query, const RenderConditionMode mode) override;
        void EndRenderCondition() override;

        /* ----- Drawing ----- */

        void Draw(std::uint32_t numVertices, std::uint32_t firstVertex) override;

        void DrawIndexed(std::uint32_t numVertices, std::uint32_t firstIndex) override;
        void DrawIndexed(std::uint32_t numVertices, std::uint32_t firstIndex, std::int32_t vertexOffset) override;

        void DrawInstanced(std::uint32_t numVertices, std::uint32_t firstVertex, std::uint32_t numInstances) override;
        void DrawInstanced(std::uint32_t numVertices, std::uint32_t firstVertex, std::uint32_t numInstances, std::uint32_t instanceOffset) override;

        void DrawIndexedInstanced(std::uint32_t numVertices, std::uint32_t numInstances, std::uint32_t firstIndex) override;
        void DrawIndexedInstanced(std::uint32_t numVertices, std::uint32_t numInstances, std::uint32_t firstIndex, std::int32_t vertexOffset) override;
        void DrawIndexedInstanced(std::uint32_t numVertices, std::uint32_t numInstances, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t instanceOffset) override;

        /* ----- Compute ----- */

        void Dispatch(std::uint32_t groupSizeX, std::uint32_t groupSizeY, std::uint32_t groupSizeZ) override;

    private:

        struct D3D11FramebufferView
        {
            std::vector<ID3D11RenderTargetView*>    rtvList;
            ID3D11DepthStencilView*                 dsv = nullptr;
        };

        void SubmitFramebufferView();

        void SetConstantBuffersOnStages(UINT startSlot, UINT count, ID3D11Buffer* const* buffers, long shaderStageFlags);
        void SetShaderResourcesOnStages(UINT startSlot, UINT count, ID3D11ShaderResourceView* const* views, long shaderStageFlags);
        void SetSamplersOnStages(UINT startSlot, UINT count, ID3D11SamplerState* const* samplers, long shaderStageFlags);
        void SetUnorderedAccessViewsOnStages(UINT startSlot, UINT count, ID3D11UnorderedAccessView* const* views, const UINT* initialCounts, long shaderStageFlags);

        void ResolveBoundRenderTarget();

        D3D11StateManager&          stateMngr_;
        
        ComPtr<ID3D11DeviceContext> context_;

        D3D11FramebufferView        framebufferView_;

        D3DClearState               clearState_;

        D3D11RenderTarget*          boundRenderTarget_  = nullptr;

};


} // /namespace LLGL


#endif



// ================================================================================
