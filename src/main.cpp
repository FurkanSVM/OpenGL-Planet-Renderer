#include <cstdio>
#include <array>

#include "utility.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp> // for matrix calculation

void MouseMoveCallback(GLFWwindow* wnd, double x, double y)
{

}

void MouseButtonCallback(GLFWwindow* wnd, int button, int action, int)
{

}

void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy)
{

}

void FramebufferChangeCallback(GLFWwindow* wnd, int w, int h)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state->width = w;
    state->height = h;
}

void KeyboardCallback(GLFWwindow* wnd, int key, int scancode, int action, int modifier)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    uint32_t mode = state->mode;

    if(action != GLFW_RELEASE) return;

    if(key == GLFW_KEY_P) mode = (mode == 3) ? 0 : (mode + 1);
    if(key == GLFW_KEY_O) mode = (mode == 0) ? 3 : (mode - 1);

    state->mode = mode;
}

int main(int argc, const char* argv[])
{
    GLState state = GLState("Planet Renderer", 1280, 720,
                            CallbackPointersGLFW());
    ShaderGL vShader = ShaderGL(ShaderGL::VERTEX, "shaders/generic.vert");
    ShaderGL fShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/debug.frag");
    MeshGL mesh = MeshGL("meshes/tri.obj");
    TextureGL tex = TextureGL("textures/2k_earth_daymap.jpg",
                              TextureGL::LINEAR, TextureGL::REPEAT);
    // Set unchanged state(s)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    // =============== //
    //   RENDER LOOP   //
    // =============== //
    while(!glfwWindowShouldClose(state.window))
    {
        // Poll inputs from the OS via GLFW
        glfwPollEvents();

        // Object-common matrices
        glm::mat4x4 proj = glm::perspective(glm::radians(50.0f),
                                            float(state.width) / float(state.height),
                                            0.01f, 100.0f);
        glm::mat4x4 view = glm::lookAt(state.pos, state.gaze, state.up);

        // Start rendering
        glViewport(0, 0, state.width, state.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);

        // Bind shaders and related uniforms / textures
        // Move this somewhere proper later.
        // These must match the uniform "location" at the shader(s).
        // Vertex
        static constexpr GLuint U_TRANSFORM_MODEL   = 0;
        static constexpr GLuint U_TRANSFORM_VIEW    = 1;
        static constexpr GLuint U_TRANSFORM_PROJ    = 2;
        static constexpr GLuint U_TRANSFORM_NORMAL  = 3;
        // Fragment
        static constexpr GLuint U_MODE = 0;
        static constexpr GLuint T_ALBEDO = 0;
        // glActiveShaderProgram makes "glUniform" family of commands
        // to effect the selected shader
        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
        {
            // Don't move the triangle
            glm::mat4x4 model = glm::identity<glm::mat4x4>();
            // Normal local->world matrix of the object
            glm::mat3x3 normalMatrix = glm::inverseTranspose(model);
            glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
            glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(view));
            glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(proj));
            glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(normalMatrix));
        }
        // Fragment shader
        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
        {
            // Bind texture(s)
            // You can bind texture via GL_TEXTURE0 + x where x is the bind point at the shader
            // you do not need to explicitly say GL_TEXTURE1 GL_TEXTURE2 etc.
            // Here is a demonstration as static assertions.
            static_assert(GL_TEXTURE0 +  1 == GL_TEXTURE1, "OGL API is wrong!");
            static_assert(GL_TEXTURE0 +  2 == GL_TEXTURE2, "OGL API is wrong!");
            static_assert(GL_TEXTURE0 + 16 == GL_TEXTURE16, "OGL API is wrong!");
            glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
            glBindTexture(GL_TEXTURE_2D, tex.textureId);

            glUniform1ui(U_MODE, state.mode);
        }
        // Bind VAO
        glBindVertexArray(mesh.vaoId);
        // Draw call!
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        glfwSwapBuffers(state.window);
    }

}
