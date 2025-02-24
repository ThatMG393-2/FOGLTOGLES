#pragma once

#include "es/utils.h"
#include "shaderc/shaderc.h"
#include "shaderc/shaderc.hpp"
#include "spirv_glsl.hpp"
#include "../utils.h"
#include "utils/log.h"

#include <GLES3/gl31.h>
#include <cstdlib>
#include <regex>
#include <stdexcept>
#include <string>

using namespace std::string_literals;

namespace ESUtils {
    inline void combineSources(GLsizei count, const GLchar *const* sources, const GLint* length, std::string& destination) {
        for (GLsizei i = 0; i < count; i++) {
            if (sources[i]) {
               if (length && length[i] > 0) {
                     destination.append(sources[i], length[i]);
               } else {
                   destination.append(sources[i]);
               }
           }
       }
    }

    inline shaderc_shader_kind getKindFromShader(GLuint& shader) {
        GLint shaderType;
        glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);
        
        switch (shaderType) {
            case GL_FRAGMENT_SHADER:
                return shaderc_fragment_shader;
            case GL_VERTEX_SHADER:
                return shaderc_vertex_shader;
            case GL_COMPUTE_SHADER:
                if (version.first == 3 && version.second >= 1) return shaderc_compute_shader;
                throw std::runtime_error("You need OpenGL ES 3.1 or newer for compute shaders!");
            
            default:
                LOGI("%u", shader);
                throw std::runtime_error("Received an unsupported shader type!");

        }
    }

    inline void replaceShaderVersion(std::string& shaderSource, std::string newVersion, std::string type = "") {
        std::regex versionRegex(R"(#version\s+\d+(\s+\w+)?\b)");  // Ensures full match
        std::string replacement = "#version " + newVersion + (type != "" ? " " + type : "") + "\n";

        LOGI("Replacing shader version to %s", replacement.c_str());
        shaderSource = std::regex_replace(shaderSource, versionRegex, replacement);
    }

    inline void glslToEssl(shaderc_shader_kind kind, std::string& fullSource) {
		LOGI("GLSL to SPIR-V starting now...");
		if (FOGLTLOGLES::getEnvironmentVar("LIBGL_VGPU_DUMP") == "1") {
			LOGI("Input GLSL source:");
			LOGI("%s", fullSource.c_str());
		}

		int glslVersion = 0;
		if (sscanf(fullSource.c_str(), "#version %i", &glslVersion) != 1) {
			throw new std::runtime_error("No '#version XXX' preprocessor!");
		}

		LOGI("Detected shader GLSL version is %i", glslVersion);
		if (glslVersion < 330) {
			glslVersion = 330;
			replaceShaderVersion(fullSource, "330");
			LOGI("New shader GLSL version is %i", glslVersion);

			if (FOGLTLOGLES::getEnvironmentVar("LIBGL_VGPU_DUMP") == "1") {
				LOGI("Regexed shader:");
				LOGI("%s", fullSource.c_str());
			}
		}

		shaderc::CompileOptions spirvOptions;
		spirvOptions.SetSourceLanguage(shaderc_source_language_glsl);
		spirvOptions.SetTargetEnvironment(shaderc_target_env_opengl, glslVersion);
		spirvOptions.SetOptimizationLevel(shaderc_optimization_level_performance);

		spirvOptions.SetAutoMapLocations(true);
		spirvOptions.SetAutoBindUniforms(true);
		spirvOptions.SetAutoSampledTextures(true);

		shaderc::Compiler spirvCompiler;
		shaderc::SpvCompilationResult bytecode = spirvCompiler.CompileGlslToSpv(
			fullSource, kind,
			"shader", spirvOptions
		);

		if (bytecode.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::string errorMessage = bytecode.GetErrorMessage();
			throw std::runtime_error("Shader compilation error: " + errorMessage);
		}

		LOGI("GLSL to SPIR-V succeeded! Commencing stage 2...");
		LOGI("Translating SPIR-V to ESSL %i", shadingVersion);

		spirv_cross::CompilerGLSL esslCompiler({ bytecode.cbegin(), bytecode.cend() });
		
		spirv_cross::CompilerGLSL::Options esslOptions;
		esslOptions.version = shadingVersion;
		esslOptions.es = true;
		esslOptions.vulkan_semantics = false;
		esslOptions.enable_420pack_extension = false;
		esslOptions.force_flattened_io_blocks = true;
		esslOptions.enable_storage_image_qualifier_deduction = false;
		
		esslCompiler.set_common_options(esslOptions);
		
		spirv_cross::ShaderResources resources = esslCompiler.get_shader_resources();
		
		for (auto& resource : resources.stage_inputs) {
			esslCompiler.unset_decoration(resource.id, spv::DecorationBinding);
		}
		
		for (auto& resource : resources.stage_outputs) {
			esslCompiler.unset_decoration(resource.id, spv::DecorationBinding);
		}
		
		for (auto& resource : resources.uniform_buffers) {
			esslCompiler.unset_decoration(resource.id, spv::DecorationBinding);
		}
		
		for (auto& resource : resources.separate_images) {
			auto type = esslCompiler.get_type(resource.base_type_id);
			if (type.basetype != spirv_cross::SPIRType::SampledImage && 
				type.basetype != spirv_cross::SPIRType::Image &&
				type.basetype != spirv_cross::SPIRType::AtomicCounter) {
				esslCompiler.unset_decoration(resource.id, spv::DecorationBinding);
			}
		}
		
		fullSource = esslCompiler.compile();
		
		if (FOGLTLOGLES::getEnvironmentVar("LIBGL_VGPU_DUMP") == "1") {
			LOGI("Generated ESSL source:");
			LOGI("%s", fullSource.c_str());
		}

		LOGI("SPIR-V to ESSL succeeded!");
	}
}
