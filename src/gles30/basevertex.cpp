#include "gles30/main.h"
#include "main.h"

#include <GLES3/gl32.h>

void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount, const GLint* basevertex);

void GLES30::registerBaseVertexFunction() {
    REGISTER(glMultiDrawElementsBaseVertex);
}

void glMultiDrawElementsBaseVertex(
    GLenum mode,
    const GLsizei* count,
    GLenum type,
    const void* const* indices,
    GLsizei drawcount,
    const GLint* basevertex
) {
    for(GLsizei i = 0; i < drawcount; i++) {
        glDrawElementsBaseVertex(mode, count[i], type, indices[i], basevertex[i]);
    }
}
