#include "gles20/main.h"
#include "es/proxy.h"
#include "es/texture.h"
#include "es/texparam.h"
#include "main.h"
#include "utils/log.h"

#include <GLES2/gl2.h>

void OV_glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
void OV_glTexImage3D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels);
void OV_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params);
void OV_glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void OV_glTexParameteri(GLenum target, GLenum pname, GLint param);

static GLint maxTextureSize = 0;
GLint proxyWidth, proxyHeight, proxyDepth, proxyInternalFormat;

void GLES20::registerTextureOverrides() {
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    LOGI("GL_MAX_TEXTURE_SIZE is %i", maxTextureSize);

    REGISTEROV(glTexImage2D);
    REGISTEROV(glTexImage3D);
    REGISTEROV(glGetTexLevelParameteriv);
    REGISTEROV(glTexParameterf);
    REGISTEROV(glTexParameteri);
}

void OV_glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    LOGI("glTexParameterf: target=%u pname=%u param=%f", target, pname, param);
    
    selectProperTexParamf(target, pname, param);
    glTexParameterf(target, pname, param);
}

void OV_glTexParameteri(GLenum target, GLenum pname, GLint param) {
    LOGI("glTexParameteri: target=%u pname=%u param=%d", target, pname, param);

    selectProperTexParami(target, pname, param);
    glTexParameteri(target, pname, param);
}

void OV_glTexImage2D(
    GLenum target,
    GLint level,
    GLint internalFormat,
    GLsizei width, GLsizei height,
    GLint border, GLenum format,
    GLenum type, const void* pixels
) {
    LOGI("glTexImage2D: internalformat=%i border=%i format=%i type=%u", internalFormat, border, format, type);
    if (isProxyTexture(target)) {
        LOGI("Received proxy texture of 2D");
        proxyWidth = (( width << level ) > maxTextureSize) ? 0 : width;
        proxyHeight = (( height << level ) > maxTextureSize) ? 0 : height;
        proxyInternalFormat = internalFormat;
    } else {
        selectProperTexType(internalFormat, type);
        
        // Check if format needs adjustment based on internalFormat
        if (format == GL_RGBA && internalFormat != GL_RGBA) {
            GLenum properFormat = selectProperTexFormat(internalFormat);
            if (properFormat != format) {
                LOGI("glTexImage2D format adjusted: %u->%u", format, properFormat);
                format = properFormat;
            }
        }
        glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
    }
}

void OV_glTexImage3D(
    GLenum target,
    GLint level,
    GLint internalFormat,
    GLsizei width, GLsizei height, GLsizei depth,
    GLint border, GLenum format,
    GLenum type, const void* pixels
) {
    LOGI("glTexImage3D: internalformat=%i border=%i format=%i type=%u", internalFormat, border, format, type);
    
    if (isProxyTexture(target)) {
        LOGI("Received proxy texture of 3D");
        proxyWidth = ((width << level) > maxTextureSize) ? 0 : width;
        proxyHeight = ((height << level) > maxTextureSize) ? 0 : height;
        proxyDepth = depth;
        proxyInternalFormat = internalFormat;
    } else {
        selectProperTexType(internalFormat, type);
        
        if (format == GL_RGBA && internalFormat != GL_RGBA) {
            GLenum properFormat = selectProperTexFormat(internalFormat);
            if (properFormat != format) {
                LOGI("glTexImage3D format adjusted: %u->%u", format, properFormat);
                format = properFormat;
            }
        }
        glTexImage3D(target, level, internalFormat, width, height, depth, 
                     border, format, type, pixels);
    }
}

void OV_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params) {
    LOGI("glGetTexLevelParameteriv: target=%u level=%i pname=%u", target, level, pname);
    
    if (isProxyTexture(target)) {
        switch (pname) {
            case GL_TEXTURE_WIDTH:
                (*params) = nlevel(proxyWidth, level);
                return;
            case GL_TEXTURE_HEIGHT:
                (*params) = nlevel(proxyHeight, level);
                return;
            case GL_TEXTURE_DEPTH:
                (*params) = nlevel(proxyDepth, level);
                return;
            case GL_TEXTURE_INTERNAL_FORMAT:
                (*params) = proxyInternalFormat;
                return;
        }
    }

    glGetTexLevelParameteriv(target, level, pname, params);
}
