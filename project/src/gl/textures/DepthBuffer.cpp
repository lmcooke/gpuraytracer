#include "DepthBuffer.h"

#include "GL/glew.h"

using namespace CS123::GL;

DepthBuffer::DepthBuffer(int width, int height) :
    m_width(width),
    m_height(height)
{
    // bind() the render buffer and call glRenderbufferStorageEXT
    DepthBuffer::bind();
    glRenderbufferStorageEXT(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

    DepthBuffer::unbind();
}
