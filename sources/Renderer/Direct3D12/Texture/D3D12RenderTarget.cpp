/*
 * D3D12RenderTarget.cpp
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015-2018 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#include "D3D12RenderTarget.h"
#include "D3D12Texture.h"
#include "../D3D12Device.h"
#include "../D3D12Types.h"
#include "../Command/D3D12CommandContext.h"
#include "../../DXCommon/DXTypes.h"
#include "../../CheckedCast.h"


namespace LLGL
{


D3D12RenderTarget::D3D12RenderTarget(D3D12Device& device, const RenderTargetDescriptor& desc) :
    resolution_ { desc.resolution }
{
    CreateDescriptorHeaps(device, desc);
    CreateAttachments(device.GetNative(), desc);
}

Extent2D D3D12RenderTarget::GetResolution() const
{
    return resolution_;
}

std::uint32_t D3D12RenderTarget::GetNumColorAttachments() const
{
    return static_cast<std::uint32_t>(colorFormats_.size());
}

bool D3D12RenderTarget::HasDepthAttachment() const
{
    return (dsvDescHeap_.Get() != nullptr);
}

bool D3D12RenderTarget::HasStencilAttachment() const
{
    return (dsvDescHeap_.Get() != nullptr && DXTypes::HasStencilComponent(depthStencilFormat_));
}

const RenderPass* D3D12RenderTarget::GetRenderPass() const
{
    return nullptr; // dummy
}

void D3D12RenderTarget::TransitionToOutputMerger(D3D12CommandContext& commandContext)
{
    for (auto& resource : colorBuffers_)
        commandContext.TransitionResource(*resource, D3D12_RESOURCE_STATE_RENDER_TARGET, false);

    if (depthStencil_ != nullptr)
        commandContext.TransitionResource(*depthStencil_, D3D12_RESOURCE_STATE_DEPTH_WRITE, false);

    commandContext.FlushResourceBarrieres();
}

void D3D12RenderTarget::ResolveRenderTarget(D3D12CommandContext& commandContext)
{
    for (auto& resource : colorBuffers_)
        commandContext.TransitionResource(*resource, resource->usageState, false);

    if (depthStencil_ != nullptr)
        commandContext.TransitionResource(*depthStencil_, depthStencil_->usageState, false);

    commandContext.FlushResourceBarrieres();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderTarget::GetCPUDescriptorHandleForRTV() const
{
    if (rtvDescHeap_)
        return rtvDescHeap_->GetCPUDescriptorHandleForHeapStart();
    else
        return {};
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12RenderTarget::GetCPUDescriptorHandleForDSV() const
{
    /*if (dsvDescHeap_)
        return dsvDescHeap_->GetCPUDescriptorHandleForHeapStart();
    else*/
        return {};
}

bool D3D12RenderTarget::HasMultiSampling() const
{
    return false; //TODO
}


/*
 * ======= Private: =======
 */

void D3D12RenderTarget::CreateDescriptorHeaps(D3D12Device& device, const RenderTargetDescriptor& desc)
{
    /* Determine number of resource views */
    colorFormats_.reserve(desc.attachments.size());

    for (const auto& attachment : desc.attachments)
    {
        if (auto texture = attachment.texture)
        {
            /* Get texture format */
            auto& textureD3D = LLGL_CAST(D3D12Texture&, *texture);
            auto format = textureD3D.GetFormat();

            /* Store color or depth-stencil format */
            if (attachment.type == AttachmentType::Color)
                colorFormats_.push_back(format);
            else
                depthStencilFormat_ = format;
        }
        else
        {
            switch (attachment.type)
            {
                case AttachmentType::Color:
                    throw std::invalid_argument("cannot have color attachment in render target without a valid texture");
                case AttachmentType::Depth:
                    depthStencilFormat_ = DXGI_FORMAT_D32_FLOAT;
                    break;
                case AttachmentType::DepthStencil:
                case AttachmentType::Stencil:
                    depthStencilFormat_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    break;
            }
        }
    }

    /* Create RTV descriptor heap */
    if (!colorFormats_.empty())
    {
        colorBuffers_.reserve(colorFormats_.size());
        auto numRenderTargets = static_cast<UINT>(colorFormats_.size());

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
        {
            heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            heapDesc.NumDescriptors = (HasMultiSampling() ? numRenderTargets * 2 : numRenderTargets);
            heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            heapDesc.NodeMask       = 0;
        }
        rtvDescHeap_ = device.CreateDXDescriptorHeap(heapDesc);
    }

    /* Create DSV descriptor heap */
    if (depthStencilFormat_ != DXGI_FORMAT_UNKNOWN)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
        {
            heapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            heapDesc.NumDescriptors = 1;
            heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            heapDesc.NodeMask       = 0;
        }
        dsvDescHeap_ = device.CreateDXDescriptorHeap(heapDesc);
    }
}

void D3D12RenderTarget::CreateAttachments(ID3D12Device* device, const RenderTargetDescriptor& desc)
{
    /* Get CPU descriptor heap start for RTVs */
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandle;
    if (rtvDescHeap_)
    {
        cpuDescHandle   = rtvDescHeap_->GetCPUDescriptorHandleForHeapStart();
        rtvDescSize_    = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    /* Create all attachments */
    for (const auto& attachment : desc.attachments)
    {
        if (auto texture = attachment.texture)
        {
            auto& textureD3D = LLGL_CAST(D3D12Texture&, *texture);
            if (attachment.type == AttachmentType::Color)
            {
                CreateSubresourceRTV(
                    device,
                    textureD3D.GetResource(),
                    textureD3D.GetFormat(),
                    textureD3D.GetType(),
                    attachment.mipLevel,
                    attachment.arrayLayer,
                    cpuDescHandle
                );
                cpuDescHandle.ptr += rtvDescSize_;
            }
            else
            {
                CreateSubresourceDSV(
                    device,
                    textureD3D.GetResource(),
                    textureD3D.GetFormat(),
                    textureD3D.GetType(),
                    attachment.mipLevel,
                    attachment.arrayLayer
                );
            }
        }
        else
        {
            //TODO...
        }
    }
}

void D3D12RenderTarget::CreateSubresourceRTV(
    ID3D12Device*                       device,
    D3D12Resource&                      resource,
    DXGI_FORMAT                         format,
    const TextureType                   type,
    UINT                                mipLevel,
    UINT                                arrayLayer,
    const D3D12_CPU_DESCRIPTOR_HANDLE&  cpuDescHandle)
{
    /* Initialize D3D12 RTV descriptor */
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = format;

    switch (type)
    {
        case TextureType::Texture1D:
            rtvDesc.ViewDimension                       = D3D12_RTV_DIMENSION_TEXTURE1D;
            rtvDesc.Texture1D.MipSlice                  = mipLevel;
            break;

        case TextureType::Texture2D:
            rtvDesc.ViewDimension                       = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice                  = mipLevel;
            rtvDesc.Texture2D.PlaneSlice                = 0;
            break;

        case TextureType::Texture3D:
            rtvDesc.ViewDimension                       = D3D12_RTV_DIMENSION_TEXTURE3D;
            rtvDesc.Texture3D.MipSlice                  = mipLevel;
            rtvDesc.Texture3D.FirstWSlice               = arrayLayer;
            rtvDesc.Texture3D.WSize                     = 1;
            break;

        case TextureType::Texture2DArray:
        case TextureType::TextureCube:
        case TextureType::TextureCubeArray:
            rtvDesc.ViewDimension                       = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture2DArray.MipSlice             = mipLevel;
            rtvDesc.Texture2DArray.FirstArraySlice      = arrayLayer;
            rtvDesc.Texture2DArray.ArraySize            = 1;
            rtvDesc.Texture2DArray.PlaneSlice           = 0;
            break;

        case TextureType::Texture1DArray:
            rtvDesc.ViewDimension                       = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtvDesc.Texture1DArray.MipSlice             = mipLevel;
            rtvDesc.Texture1DArray.FirstArraySlice      = arrayLayer;
            rtvDesc.Texture1DArray.ArraySize            = 1;
            break;

        case TextureType::Texture2DMS:
            rtvDesc.ViewDimension                       = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            break;

        case TextureType::Texture2DMSArray:
            rtvDesc.ViewDimension                       = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            rtvDesc.Texture2DMSArray.FirstArraySlice    = arrayLayer;
            rtvDesc.Texture2DMSArray.ArraySize          = 1;
            break;

    }

    /* Create RTV and store reference to resource */
    device->CreateRenderTargetView(resource.native.Get(), &rtvDesc, cpuDescHandle);
    colorBuffers_.push_back(&resource);
}

void D3D12RenderTarget::CreateSubresourceDSV(
    ID3D12Device*       device,
    D3D12Resource&      resource,
    DXGI_FORMAT         format,
    const TextureType   type,
    UINT                mipLevel,
    UINT                arrayLayer)
{
    /* Initialize D3D12 RTV descriptor */
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Format  = format;
    dsvDesc.Flags   = D3D12_DSV_FLAG_NONE;

    switch (type)
    {
        case TextureType::Texture1D:
            dsvDesc.ViewDimension                       = D3D12_DSV_DIMENSION_TEXTURE1D;
            dsvDesc.Texture1D.MipSlice                  = mipLevel;
            break;

        case TextureType::Texture2D:
            dsvDesc.ViewDimension                       = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice                  = mipLevel;
            break;

        case TextureType::Texture3D:
        case TextureType::TextureCube:
        case TextureType::TextureCubeArray:
        case TextureType::Texture2DArray:
            dsvDesc.ViewDimension                       = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsvDesc.Texture2DArray.MipSlice             = mipLevel;
            dsvDesc.Texture2DArray.FirstArraySlice      = arrayLayer;
            dsvDesc.Texture2DArray.ArraySize            = 1;
            break;

        case TextureType::Texture1DArray:
            dsvDesc.ViewDimension                       = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
            dsvDesc.Texture1DArray.MipSlice             = mipLevel;
            dsvDesc.Texture1DArray.FirstArraySlice      = arrayLayer;
            dsvDesc.Texture1DArray.ArraySize            = mipLevel;
            break;

        case TextureType::Texture2DMS:
            dsvDesc.ViewDimension                       = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            break;

        case TextureType::Texture2DMSArray:
            dsvDesc.ViewDimension                       = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            dsvDesc.Texture2DMSArray.FirstArraySlice    = arrayLayer;
            dsvDesc.Texture2DMSArray.ArraySize          = 1;
            break;
    }

    /* Create DSV and store reference to resource */
    device->CreateDepthStencilView(resource.native.Get(), &dsvDesc, dsvDescHeap_->GetCPUDescriptorHandleForHeapStart());
    depthStencil_ = &resource;
}


} // /namespace LLGL



// ================================================================================
