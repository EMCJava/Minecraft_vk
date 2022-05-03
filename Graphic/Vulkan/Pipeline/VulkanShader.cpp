//
// Created by loys on 18/1/2022.
//

#include "VulkanShader.hpp"
#include <Utility/Logger.hpp>

#include <cassert>
#include <fstream>
#include <shaderc/shaderc.hpp>

#define READ_PATH( path_prefix )                                                                 \
    std::vector<char> path_prefix##_buffer;                                                      \
    {                                                                                            \
        std::ifstream path_prefix##file( path_prefix##_path, std::ios::ate | std::ios::binary ); \
        assert( path_prefix##file );                                                             \
        path_prefix##_buffer.resize( path_prefix##file.tellg( ) );                               \
        path_prefix##file.seekg( 0 );                                                            \
        path_prefix##file.read( path_prefix##_buffer.data( ), path_prefix##_buffer.size( ) );    \
        path_prefix##file.close( );                                                              \
    }

bool
VulkanShader::InitGLSLBinary( const vk::Device& device, const std::string& vertex_binary_path, const std::string& fragment_binary_path )
{
    READ_PATH( vertex_binary )
    READ_PATH( fragment_binary )

    m_vkVertex_shader_module   = InitGLSLCode( device, vertex_binary_buffer );
    m_vkFragment_shader_module = InitGLSLCode( device, fragment_binary_buffer );

    return true;
}

bool
VulkanShader::InitGLSLFile( const vk::Device& device, const std::string& vertex_file_path, const std::string& fragment_binary_path )
{
    std::stringstream vertex_sstr, fragment_sstr;

    vertex_sstr << std::ifstream( vertex_file_path ).rdbuf( );
    fragment_sstr << std::ifstream( fragment_binary_path ).rdbuf( );

    return InitGLSLString( device, vertex_sstr.str( ), fragment_sstr.str( ) );
}

bool
VulkanShader::InitGLSLString( const vk::Device& device, const std::string& vertexShader, const std::string& fragmentShader )
{
    shaderc::Compiler       compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel( shaderc_optimization_level_performance );

    shaderc::SpvCompilationResult vertShaderModule =
        compiler.CompileGlslToSpv( vertexShader, shaderc_glsl_vertex_shader, "vertex shader", options );
    if ( vertShaderModule.GetCompilationStatus( ) != shaderc_compilation_status_success )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eError, vertShaderModule.GetErrorMessage( ) );
        throw std::runtime_error( "vertex shader compilation failed !" );
    }

    shaderc::SpvCompilationResult fragShaderModule = compiler.CompileGlslToSpv(
        fragmentShader, shaderc_glsl_fragment_shader, "fragment shader", options );
    if ( fragShaderModule.GetCompilationStatus( ) != shaderc_compilation_status_success )
    {
        Logger::getInstance( ).LogLine( Logger::LogType::eError, fragShaderModule.GetErrorMessage( ) );
        throw std::runtime_error( "fragment shader compilation failed !" );
    }

    m_vkVertex_shader_module   = InitGLSLCode( device, std::vector<uint32_t> { vertShaderModule.cbegin( ), vertShaderModule.cend( ) } );
    m_vkFragment_shader_module = InitGLSLCode( device, std::vector<uint32_t> { fragShaderModule.cbegin( ), fragShaderModule.cend( ) } );

    return true;
}

vk::UniqueShaderModule
VulkanShader::InitGLSLCode( const vk::Device& device, const std::vector<uint32_t>& code )
{
    vk::ShaderModuleCreateInfo createInfo { { },
                                            code.size( ) * sizeof( uint32_t ),
                                            code.data( ) };

    return device.createShaderModuleUnique( createInfo );
}

vk::UniqueShaderModule
VulkanShader::InitGLSLCode( const vk::Device& device, const std::vector<char>& code )
{
    vk::ShaderModuleCreateInfo createInfo { { },
                                            code.size( ) * sizeof( char ),
                                            reinterpret_cast<const uint32_t*>( code.data( ) ) };

    return device.createShaderModuleUnique( createInfo );
}
