/*
 * D3D11Texture.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_D3D11_TEXTURE_H__
#define __LLGL_D3D11_TEXTURE_H__


#include <LLGL/Texture.h>
#include <d3d11.h>
#include "../../ComPtr.h"


namespace LLGL
{


union D3D11HardwareTexture
{
    D3D11HardwareTexture() :
        resource( nullptr )
    {
    }
    ~D3D11HardwareTexture()
    {
    }

    ComPtr<ID3D11Resource>  resource;
    ComPtr<ID3D11Texture1D> tex1D;
    ComPtr<ID3D11Texture2D> tex2D;
    ComPtr<ID3D11Texture3D> tex3D;
};


class D3D11Texture : public Texture
{

    public:

        D3D11Texture(const D3D11Texture&) = delete;
        D3D11Texture& operator = (const D3D11Texture&) = delete;

        D3D11Texture() = default;

        Gs::Vector3i QueryMipLevelSize(int mipLevel) const override;

        /* ----- Extended internal functions ---- */

        void CreateTexture1D(ID3D11Device* device, const D3D11_TEXTURE1D_DESC& desc, const D3D11_SUBRESOURCE_DATA* initialData = nullptr);
        void CreateTexture2D(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc, const D3D11_SUBRESOURCE_DATA* initialData = nullptr);
        void CreateTexture3D(ID3D11Device* device, const D3D11_TEXTURE3D_DESC& desc, const D3D11_SUBRESOURCE_DATA* initialData = nullptr);

        inline const D3D11HardwareTexture& GetHardwareTexture() const
        {
            return hardwareTexture_;
        }

        // Returns the shader-resource-view (SRV) of the hardware texture object.
        inline ID3D11ShaderResourceView* GetSRV() const
        {
            return srv_.Get();
        }

    private:

        void CreateSRV(ID3D11Device* device);

        D3D11HardwareTexture                hardwareTexture_;
        ComPtr<ID3D11ShaderResourceView>    srv_;

};


} // /namespace LLGL


#endif



// ================================================================================
