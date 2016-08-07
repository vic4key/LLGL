/*
 * ShaderProgram.h
 * 
 * This file is part of the "LLGL" project (Copyright (c) 2015 by Lukas Hermanns)
 * See "LICENSE.txt" for license information.
 */

#ifndef __LLGL_SHADER_PROGRAM_H__
#define __LLGL_SHADER_PROGRAM_H__


#include "Export.h"
#include "VertexShader.h"
#include "FragmentShader.h"
#include "GeometryShader.h"
#include "TessControlShader.h"
#include "TessEvaluationShader.h"
#include "ComputeShader.h"
#include "VertexAttribute.h"
#include "ConstantBuffer.h"
#include <string>
#include <vector>


namespace LLGL
{


//! Shader program interface.
class LLGL_EXPORT ShaderProgram
{

    public:

        virtual ~ShaderProgram()
        {
        }

        virtual void AttachShader( VertexShader&         vertexShader         ) = 0;
        virtual void AttachShader( FragmentShader&       fragmentShader       ) = 0;
        virtual void AttachShader( GeometryShader&       geometryShader       ) = 0;
        virtual void AttachShader( TessControlShader&    tessControlShader    ) = 0;
        virtual void AttachShader( TessEvaluationShader& tessEvaluationShader ) = 0;
        virtual void AttachShader( ComputeShader&        computeShader        ) = 0;

        /**
        \brief Links all attached shaders to the final shader program.
        \return True on success, otherwise "QueryInfoLog" can be used to query the reason for failure.
        \remarks Each attached shader must be compiled first!
        \see QueryInfoLog
        */
        virtual bool LinkShaders() = 0;

        //! Returns the information log after the shader linkage.
        virtual std::string QueryInfoLog() = 0;

        //! Returns a list of vertex attributes, which describe all vertex attributes within this shader program.
        virtual std::vector<VertexAttribute> QueryVertexAttributes() const = 0;

        //! Returns a list of constant buffer descriptors, which describe all constant buffers (also "Uniform Buffer Object") within this shader program.
        virtual std::vector<ConstantBufferDescriptor> QueryConstantBuffers() const = 0;

        /**
        \brief Binds the specified vertex attributes to this shader program.
        \param[in] vertexAttribs Specifies the vertex attributes.
        \param[in] ignoreUnusedAttributes Specifies whether to ignore unused vertex attributes.
        If this is true, no exception is thrown if a vertex attribute could not be found.
        This option might be useful, when a shader removes vertex attributes for optimization. By default false.
        \remarks This is only required for a shader program, which has an attached vertex shader,
        and it can only be used after the shaders have already been linked.
        \see AttachShader(VertexShader&)
        \see LinkShaders
        \throws std::invalid_argument If the name of an vertex attribute is invalid or the maximal number of available vertex attributes is exceeded.
        \throws std::runtime_error If this function is called before the shaders have been successfully linked.
        */
        virtual void BindVertexAttributes(const std::vector<VertexAttribute>& vertexAttribs, bool ignoreUnusedAttributes = false) = 0;

        /**
        \brief Binds the specified constant buffer to this shader.
        \param[in] name Specifies the name of the constant buffer within this shader.
        \param[in] bindingIndex Specifies the binding index. This index must match the index which will be used for "RenderContext::BindConstantBuffer".
        \see QueryConstantBuffers
        \see RenderContext::BindConstantBuffer
        */
        virtual void BindConstantBuffer(const std::string& name, unsigned int bindingIndex) = 0;

};


} // /namespace LLGL


#endif



// ================================================================================