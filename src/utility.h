#pragma once

#include <string>
#include <cassert>

#include <glad/glad.h>
#include <glm/glm.hpp>

struct GLFWwindow;
using GLFWcursorposfun       = void (*)(GLFWwindow*, double, double);
using GLFWmousebuttonfun     = void (*)(GLFWwindow*, int, int, int);
using GLFWscrollfun          = void (*)(GLFWwindow*, double, double);
using GLFWkeyfun             = void (*)(GLFWwindow*, int, int, int, int);
using GLFWframebuffersizefun = void (*)(GLFWwindow*, int, int);

void MouseMoveCallback(GLFWwindow*, double x, double y);
void MouseButtonCallback(GLFWwindow*, int button, int action, int);
void MouseScrollCallback(GLFWwindow*, double dx, double dy);
void FramebufferChangeCallback(GLFWwindow*, int w, int h);
void KeyboardCallback(GLFWwindow*, int button, int scancode, int action, int mode);

struct CallbackPointersGLFW
{
    GLFWcursorposfun       mMoveCallback   = MouseMoveCallback;
    GLFWmousebuttonfun     mButtonCallback = MouseButtonCallback;
    GLFWscrollfun          mScrollCallback = MouseScrollCallback;
    GLFWkeyfun             keyCallback     = KeyboardCallback;
    GLFWframebuffersizefun fboCallback     = FramebufferChangeCallback;
};

struct GLState
{
    GLFWwindow* window = nullptr;
    GLuint      renderPipeline = 0u;

    // Data from callbacks
    // FBO Params
    int32_t width  = 0;
    int32_t height = 0;
    // Camera
    glm::vec3 gaze  = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 pos   = glm::vec3(0.0f, 0.0f, 2.0f);
    glm::vec3 up    = glm::vec3(0.0f, 1.0f, 0.0f);
    // Render mode
    uint32_t mode = 2;

    // Constructors, Movement & Destructor
                GLState(const char* const windowName,
                        int width, int height,
                        CallbackPointersGLFW);
                GLState(const GLState&) = delete;
                GLState(GLState&&) = delete;
    GLState&    operator=(const GLState&) = delete;
    GLState&    operator=(GLState&&) = delete;
                ~GLState();
};

struct ShaderGL
{
    enum Type
    {
        VERTEX      = GL_VERTEX_SHADER,
        FRAGMENT    = GL_FRAGMENT_SHADER
    };

    GLuint      shaderId = 0;
    // Constructors, Movement & Destructor
                ShaderGL(Type t, const std::string& path);
                ShaderGL(const ShaderGL&) = delete;
                ShaderGL(ShaderGL&&);
    ShaderGL&   operator=(const ShaderGL&) = delete;
    ShaderGL&   operator=(ShaderGL&&);
                ~ShaderGL();
};

struct MeshGL
{
    // These intake Ids must match to the vertex shader
    // That is used currently.
    static constexpr GLuint IN_POS      = 0;
    static constexpr GLuint IN_NORMAL   = 1;
    static constexpr GLuint IN_UV       = 2;
    static constexpr GLuint IN_COLOR    = 3;

    GLuint vBufferId  = 0;
    GLuint iBufferId  = 0;
    GLuint vaoId      = 0;
    GLuint indexCount = 0;
    // Constructors, Movement & Destructor
            MeshGL(const std::string& objPath);
            MeshGL(const MeshGL&) = delete;
            MeshGL(MeshGL&&);
    MeshGL& operator=(const MeshGL&) = delete;
    MeshGL& operator=(MeshGL&&);
            ~MeshGL();
};

struct TextureGL
{
    enum SampleMode
    {
        NEAREST = GL_NEAREST_MIPMAP_NEAREST,
        LINEAR  = GL_LINEAR_MIPMAP_LINEAR
    };

    enum EdgeResolve
    {
        CLAMP    = GL_CLAMP_TO_EDGE,
        REPEAT   = GL_REPEAT,
        MIRROR   = GL_MIRRORED_REPEAT
    };

    GLuint  textureId    = 0;
    int     width        = 0;
    int     height       = 0;
    int     channelCount = 0;
    //
                TextureGL(const std::string& texPath,
                          SampleMode, EdgeResolve);
                TextureGL(const TextureGL&) = delete;
                TextureGL(TextureGL&&);
    TextureGL&  operator=(const TextureGL&) = delete;
    TextureGL&  operator=(TextureGL&&);
                ~TextureGL();
};

// Inline Definitions
inline ShaderGL::ShaderGL(ShaderGL&& other)
    : shaderId(other.shaderId)
{
    other.shaderId = 0;
}

inline ShaderGL& ShaderGL::operator=(ShaderGL&& other)
{
    assert(this != &other);
    shaderId = other.shaderId;
    other.shaderId = 0;
    return *this;
}

inline ShaderGL::~ShaderGL()
{
    if(shaderId) glDeleteProgram(shaderId);
}

inline MeshGL::MeshGL(MeshGL&& other)
    : vBufferId(other.vBufferId)
    , iBufferId(other.iBufferId)
    , vaoId(other.vaoId)
{
    other.vBufferId = 0;
    other.iBufferId = 0;
    other.vaoId = 0;
}

inline MeshGL& MeshGL::operator=(MeshGL&& other)
{
    assert(this != &other);
    vBufferId = other.vBufferId;
    iBufferId = other.iBufferId;
    vaoId = other.vaoId;
    other.vBufferId = 0;
    other.iBufferId = 0;
    other.vaoId = 0;
    return *this;
}

inline MeshGL::~MeshGL()
{
    if(vaoId) glDeleteVertexArrays(1, &vaoId);
    if(vBufferId) glDeleteBuffers(1, &vBufferId);
    if(iBufferId) glDeleteBuffers(1, &iBufferId);
}

inline TextureGL::TextureGL(TextureGL&& other)
    : textureId(other.textureId)
{
    other.textureId = 0;
}

inline TextureGL& TextureGL::operator=(TextureGL&& other)
{
    assert(this != &other);
    textureId = other.textureId;
    other.textureId = 0;
    return *this;
}

inline TextureGL::~TextureGL()
{
    if(textureId) glDeleteTextures(1, &textureId);
}

