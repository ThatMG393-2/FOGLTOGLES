#pragma once

#include "shaderc/shaderc.hpp"
#include "spirv_glsl.hpp"
#include "utils/log.h"

#include <stdexcept>

inline void upgradeTo330(shaderc_shader_kind kind, std::string& src) {
    LOGI("Upgrading shader to GLSL 330");

    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_opengl, 330);
    options.SetForcedVersionProfile(330, shaderc_profile_core);

    options.SetAutoMapLocations(true);
    options.SetAutoBindUniforms(true);
    options.SetAutoSampledTextures(true);

    shaderc::Compiler spirvCompiler;
    shaderc::SpvCompilationResult module = spirvCompiler.CompileGlslToSpv(
        src, kind, 
        "shader_upgto330", options
    );

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error("Shader upgrade error: " + module.GetErrorMessage());
    }

    spirv_cross::CompilerGLSL glslCompiler({ module.cbegin(), module.cend() });
    spirv_cross::CompilerGLSL::Options glslOptions;
    glslOptions.version = 330;  // Output GLSL 330
    glslOptions.es = false;     // Not targeting OpenGL ES
    glslOptions.vulkan_semantics = false;
    glslCompiler.set_common_options(glslOptions);

    src = glslCompiler.compile();

    LOGI("Upgrade successful!");
}