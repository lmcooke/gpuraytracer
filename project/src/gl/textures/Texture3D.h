#ifndef TEXTURE3D_H
#define TEXTURE3D_H

#include "gl/textures/Texture.h"

#include "GL/glew.h"

namespace CS123 { namespace GL {

class Texture3D : public Texture {
public:
    Texture3D(unsigned char *data, int width, int height, GLenum type = GL_UNSIGNED_BYTE);

    virtual void bind() const override;
    virtual void unbind() const override;
};

}}

#endif // TEXTURE3D_H
