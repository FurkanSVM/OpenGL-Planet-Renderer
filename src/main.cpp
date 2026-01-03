#include <cstdio>
#include <array>

#include "utility.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp> // for matrix calculation


glm::vec3 cameraOffset(0.0f, 2.5f, 5.0f); // Başlangıç offset'i
float timeSpeed = 1.0f; // Zaman hızı çarpanı (L/K ile değişir)

void MouseMoveCallback(GLFWwindow* wnd, double x, double y)
{
    auto* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if(!state->leftMousePressed)
    {
        state->lastMouseX = x;
        state->lastMouseY = y;
        return;
    }

    float dx = float(x - state->lastMouseX);
    float dy = float(y - state->lastMouseY);

    state->lastMouseX = x;
    state->lastMouseY = y;

    float sensitivity = 0.005f;
    dx *= sensitivity;
    dy *= sensitivity;

    if(state->mode >= 0 && state->mode <= 2) {
        // Gezegen modunda offset'i döndür
        glm::mat4 yaw   = glm::rotate(glm::mat4(1.0f), -dx, glm::vec3(0,1,0));
        glm::mat4 pitch = glm::rotate(glm::mat4(1.0f), -dy, glm::vec3(1,0,0));
        cameraOffset = glm::vec3(yaw * pitch * glm::vec4(cameraOffset, 1.0f));
    } else {
        // FPS modunda eski davranış
        glm::vec3 forward = glm::normalize(state->gaze - state->pos);
        glm::vec3 right   = glm::normalize(glm::cross(forward, state->up));
        // yaw
        forward = glm::vec3(glm::rotate(glm::mat4(1.0f), -dx, state->up) * glm::vec4(forward, 0.0f));
        // pitch
        forward = glm::vec3(glm::rotate(glm::mat4(1.0f), -dy, right) * glm::vec4(forward, 0.0f));
        state->gaze = state->pos + forward;
    }
}

void MouseButtonCallback(GLFWwindow* wnd, int button, int action, int)
{
    auto* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));

    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if(action == GLFW_PRESS)
            state->leftMousePressed = true;
        else if(action == GLFW_RELEASE)
            state->leftMousePressed = false;
    }
}

void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    if(state->mode >= 0 && state->mode <= 2) {
        // Gezegen modunda offset'in uzunluğunu değiştir (zoom)
        float zoomSpeed = 0.5f;
        float len = glm::length(cameraOffset);
        len -= static_cast<float>(dy) * zoomSpeed;
        len = glm::clamp(len, 1.0f, 50.0f); // min/max zoom
        cameraOffset = glm::normalize(cameraOffset) * len;
    } else {
        // FPS modunda mevcut davranış
        glm::vec3 forward = glm::normalize(state->gaze - state->pos);
        state->pos += static_cast<float>(dy) * 0.1f * forward;
    }

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

    // Tuş basılı mı, bırakıldı mı takip et
    if(key >= 0 && key < 1024) {
        if(action == GLFW_PRESS)
            state->keyPressed[key] = true;
        else if(action == GLFW_RELEASE)
            state->keyPressed[key] = false;
    }

    if(action != GLFW_RELEASE) return;

    if(key == GLFW_KEY_P) mode = (mode == 3) ? 0 : (mode + 1);
    if(key == GLFW_KEY_O) mode = (mode == 0) ? 3 : (mode - 1);
    
    // L: Zamanı hızlandır
    if(key == GLFW_KEY_L) {
        timeSpeed += 0.25f;
        if(timeSpeed > 3.0f) timeSpeed = 3.0f; // Maksimum hız
    }
    // K: Zamanı yavaşlat (ardarda basınca geri gider)
    if(key == GLFW_KEY_K) {
        timeSpeed -= 0.25f;
        if(timeSpeed < -3.0f) timeSpeed = -3.0f; // Minimum (geri) hız
    }

    state->mode = mode;
    
}

// --- Planet struct ve gezegen dizisi ---
#include <vector>
struct Planet {
    float orbitRadius;      // Yörünge yarıçapı
    float orbitSpeed;       // Yörünge açısal hızı
    float selfSpeed;        // Kendi etrafında dönme hızı
    float radius;           // Gezegen yarıçapı (çizim için ölçek)
    glm::vec3 color;        // (şimdilik kullanılmıyor)
    TextureGL* texture;     // Texture pointer
    int parentIndex;        // Bağlı olduğu gezegenin indexi (-1 ise merkez)
    float orbitPhase;       // Başlangıç fazı
};

int main(int argc, const char* argv[])
{
    GLState state = GLState("Planet Renderer", 1280, 720,
                            CallbackPointersGLFW());
    ShaderGL vShader = ShaderGL(ShaderGL::VERTEX, "../working_dir/shaders/generic.vert");
    ShaderGL fShader = ShaderGL(ShaderGL::FRAGMENT, "../working_dir/shaders/debug.frag");
    // Shadow mapping shaders
    ShaderGL shadowVShader = ShaderGL(ShaderGL::VERTEX, "../working_dir/shaders/shadow.vert");
    ShaderGL shadowFShader = ShaderGL(ShaderGL::FRAGMENT, "../working_dir/shaders/shadow.frag");
    MeshGL sphereMesh = MeshGL("../working_dir/meshes/sphere_80k.obj");
    TextureGL texEarth = TextureGL("../working_dir/textures/2k_earth_daymap.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texMoon = TextureGL("../working_dir/textures/2k_moon.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texMoonMoon = TextureGL("../working_dir/textures/2k_jupiter.jpg", TextureGL::LINEAR, TextureGL::REPEAT); // örnek
    // --- PART 2.2: Earth Shading Effects ---
    TextureGL texEarthSpecular("../working_dir/textures/2k_earth_specular_map.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texEarthNight("../working_dir/textures/2k_earth_nightmap_alpha.png", TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texEarthClouds("../working_dir/textures/2k_earth_clouds_alpha.png", TextureGL::LINEAR, TextureGL::REPEAT);
    // --- Background and Sun Textures ---
    TextureGL texStars("../working_dir/textures/8k_stars_milky_way.jpg", TextureGL::LINEAR, TextureGL::REPEAT);
    // Güneş için basit bir sarı sphere kullanılacak, texture gerekirse eklenir
    auto orthoProj = [](float size, float aspect, float near, float far) {
        float w = size * aspect;
        float h = size;
        return glm::ortho(-w, w, -h, h, near, far);
    };
    GLuint samplerLinear;
    glGenSamplers(1, &samplerLinear);
    glSamplerParameteri(samplerLinear, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(samplerLinear, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(samplerLinear, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(samplerLinear, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // =============== //
    // SHADOW MAP FBO  //
    // =============== //
    const int SHADOW_MAP_SIZE = 2048;
    GLuint shadowFBO;
    GLuint shadowColorTex;
    GLuint shadowDepthTex;
    
    // Create shadow map color texture (32-bit float, single channel)
    glGenTextures(1, &shadowColorTex);
    glBindTexture(GL_TEXTURE_2D, shadowColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create shadow map depth texture
    glGenTextures(1, &shadowDepthTex);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create framebuffer
    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowColorTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTex, 0);
    
    // Check FBO status
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Shadow FBO is not complete!\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Shadow pipeline
    GLuint shadowPipeline;
    glGenProgramPipelines(1, &shadowPipeline);
    glBindProgramPipeline(shadowPipeline);
    
    // Gezegenleri oluştur
    std::vector<Planet> planets;
    // Dünya (merkez)
    planets.push_back({0.0f, 0.0f, 0.2f, 1.0f, glm::vec3(0.5,0.5,1.0), &texEarth, -1, 0.0f}); // Dünya
    planets.push_back({2.0f, 0.7f, 0.5f, 0.27f, glm::vec3(0.8,0.8,0.8), &texMoon, 0, 0.0f}); // Ay
    planets.push_back({0.6f, 2.0f, 1.0f, 0.1f, glm::vec3(1.0,0.7,0.7), &texMoonMoon, 1, 0.0f}); // Ayın Ayı

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    float time = 0.0f;

    // =============== //
    //   RENDER LOOP   //
    // =============== //
    int lastMode = state.mode; // Son kullanılan kamera modu
    while(!glfwWindowShouldClose(state.window))
    {
        glfwPollEvents();
        glViewport(0, 0, state.width, state.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // FPS modunda sürekli hareket
        if(state.mode == 3) {
            glm::vec3 forward = glm::normalize(state.gaze - state.pos);
            glm::vec3 right = glm::normalize(glm::cross(forward, state.up));
            float speed = 0.1f;
            if(state.keyPressed[GLFW_KEY_W]) {
                state.pos += forward * speed;
                state.gaze += forward * speed;
            }
            if(state.keyPressed[GLFW_KEY_S]) {
                state.pos -= forward * speed;
                state.gaze -= forward * speed;
            }
            if(state.keyPressed[GLFW_KEY_A]) {
                state.pos -= right * speed;
                state.gaze -= right * speed;
            }
            if(state.keyPressed[GLFW_KEY_D]) {
                state.pos += right * speed;
                state.gaze += right * speed;
            }
        }
        // Zamanı güncelle (timeSpeed ile çarpılır)
        time += 0.016f * timeSpeed; // ~60 FPS için sabit artış

        float cam_dist = 5.0f;

        // Her gezegenin gerçek pozisyonunu hesapla
        std::vector<glm::vec3> planetPositions(planets.size());
        for(size_t i=0; i<planets.size(); ++i) {
            const Planet& planet = planets[i];
            float angle = planet.orbitPhase + planet.orbitSpeed * time;
            if(planet.parentIndex >= 0) {
                glm::vec3 parentPos = planetPositions[planet.parentIndex];
                planetPositions[i] = parentPos + planet.orbitRadius * glm::vec3(cos(angle), 0, sin(angle));
            } else {
                planetPositions[i] = glm::vec3(0.0f);
            }
        }

        // Uniform konumları (doğrudan shader program id'si ile alınmalı) -- SADECE BİR KEZ TANIMLANIR
        static GLint locModel = glGetUniformLocation(vShader.shaderId, "uModel");
        static GLint locView = glGetUniformLocation(vShader.shaderId, "uView");
        static GLint locProj = glGetUniformLocation(vShader.shaderId, "uProjection");
        static GLint locNormal = glGetUniformLocation(vShader.shaderId, "uNormalMatrix");
        static GLint locMode = glGetUniformLocation(fShader.shaderId, "uMode");
        static GLint locLightDir = glGetUniformLocation(fShader.shaderId, "uLightDir");
        static GLint locViewDir = glGetUniformLocation(fShader.shaderId, "uViewDir");
        // Shadow mapping uniform locations
        static GLint locShadowMap = glGetUniformLocation(fShader.shaderId, "uShadowMap");
        static GLint locLightVP = glGetUniformLocation(fShader.shaderId, "uLightVP");
        static GLint locShadowBias = glGetUniformLocation(fShader.shaderId, "uShadowBias");
        // Shadow shader uniform locations
        static GLint locShadowModel = glGetUniformLocation(shadowVShader.shaderId, "uModel");
        static GLint locShadowView = glGetUniformLocation(shadowVShader.shaderId, "uView");
        static GLint locShadowProj = glGetUniformLocation(shadowVShader.shaderId, "uProjection");
        static constexpr GLuint T_ALBEDO = 0;

        // --- DİNAMİK LIGHT DIRECTION ---
        float sunOrbitSpeed = 0.2f; // yavaş dönsün
        float sunAngle = time * sunOrbitSpeed;
        glm::vec3 lightDir = glm::normalize(glm::vec3(cos(sunAngle), 0.5f, sin(sunAngle)));

        // ===================== //
        //     SHADOW PASS       //
        // ===================== //
        // Light view matrix: looking from sun direction
        glm::vec3 lightForward = -glm::normalize(lightDir);
        glm::vec3 lightUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 lightRight = glm::normalize(glm::cross(lightForward, lightUp));
        lightUp = glm::normalize(glm::cross(lightRight, lightForward));
        
        // Ortho projection that covers the scene
        float orthoSize = 10.0f; // Sahneyi kapsayacak kadar büyük
        glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, -50.0f, 50.0f);
        glm::mat4 lightView = glm::lookAt(glm::vec3(0.0f), lightForward, lightUp);
        glm::mat4 lightVP = lightProj * lightView;
        
        // Render to shadow FBO
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Clear with large value (far)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        
        // Use shadow pipeline
        glBindProgramPipeline(shadowPipeline);
        glUseProgramStages(shadowPipeline, GL_VERTEX_SHADER_BIT, shadowVShader.shaderId);
        glUseProgramStages(shadowPipeline, GL_FRAGMENT_SHADER_BIT, shadowFShader.shaderId);
        
        // Render planets to shadow map
        for(size_t i=0; i<planets.size(); ++i) {
            const Planet& planet = planets[i];
            glm::vec3 pos = planetPositions[i];
            
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, planet.selfSpeed * time, glm::vec3(0,1,0));
            model = glm::scale(model, glm::vec3(planet.radius));
            
            glActiveShaderProgram(shadowPipeline, shadowVShader.shaderId);
            glUniformMatrix4fv(locShadowModel, 1, false, glm::value_ptr(model));
            glUniformMatrix4fv(locShadowView, 1, false, glm::value_ptr(lightView));
            glUniformMatrix4fv(locShadowProj, 1, false, glm::value_ptr(lightProj));
            
            glBindVertexArray(sphereMesh.vaoId);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        
        // Switch back to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindProgramPipeline(state.renderPipeline);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glViewport(0, 0, state.width, state.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. BACKGROUND STARS SPHERE (kamera merkezli, her zaman en arkada)
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE); // İçeriden bakıyoruz, cull face kapalı olmalı
        {
            float bgRadius = 50.0f;
            // Kamera merkezine yerleştir (sadece rotation etkili olsun)
            glm::mat4 bgModel = glm::translate(glm::mat4(1.0f), state.pos) * glm::scale(glm::mat4(1.0f), glm::vec3(bgRadius));
            glm::mat3 bgNormal = glm::inverseTranspose(glm::mat3(bgModel));
            glm::mat4x4 bgProj = glm::perspective(glm::radians(50.0f),
                                            float(state.width) / float(state.height),
                                            0.01f, 200.0f);
            glm::mat4x4 bgView = glm::lookAt(state.pos, state.gaze, state.up);
            // Vertex shader
            glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
            glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
            glUniformMatrix4fv(locModel, 1, false, glm::value_ptr(bgModel));
            glUniformMatrix4fv(locView, 1, false, glm::value_ptr(bgView));
            glUniformMatrix4fv(locProj, 1, false, glm::value_ptr(bgProj));
            glUniformMatrix3fv(locNormal, 1, false, glm::value_ptr(bgNormal));
            // Fragment shader
            glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
            glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texStars.textureId);
            glBindSampler(0, samplerLinear);
            glUniform1ui(locMode, 3); // Sadece texture
            glBindVertexArray(sphereMesh.vaoId);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        glEnable(GL_CULL_FACE);
        // 2. SUN SPHERE (kameraya göre, ışık yönünün tersine uzakta)
        {
            float sunDist = 80.0f;  // Kameradan uzaklık
            float sunRadius = 0.8f; // Küçük küre
            // Güneş kamera pozisyonundan, ışık yönünün tersine doğru uzakta
            glm::vec3 sunPos = state.pos - glm::normalize(lightDir) * sunDist;
            glm::mat4 sunModel = glm::translate(glm::mat4(1.0f), sunPos) * glm::scale(glm::mat4(1.0f), glm::vec3(sunRadius));
            glm::mat3 sunNormal = glm::inverseTranspose(glm::mat3(sunModel));
            glm::mat4x4 sunProj = glm::perspective(glm::radians(50.0f),
                                            float(state.width) / float(state.height),
                                            0.01f, 200.0f);
            glm::mat4x4 sunView = glm::lookAt(state.pos, state.gaze, state.up);
            // Vertex shader
            glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
            glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
            glUniformMatrix4fv(locModel, 1, false, glm::value_ptr(sunModel));
            glUniformMatrix4fv(locView, 1, false, glm::value_ptr(sunView));
            glUniformMatrix4fv(locProj, 1, false, glm::value_ptr(sunProj));
            glUniformMatrix3fv(locNormal, 1, false, glm::value_ptr(sunNormal));
            // Fragment shader
            glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
            glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
            glUniform1ui(locMode, 12); // Sun rendering mode (shader'da saf sarı)
            glBindVertexArray(sphereMesh.vaoId);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        // Kamera moduna göre pozisyon ve bakış noktası ayarla
        if (state.mode >= 0 && state.mode <= 2) {
            glm::vec3 target = planetPositions[state.mode];
            if (lastMode != state.mode) {
                cameraOffset = glm::vec3(0.0f, 2.5f, 5.0f); // Mod değişince offset'i sıfırla
            }
            state.pos = target + cameraOffset;
            state.gaze = target;
            // Mouse hareketleriyle state.pos ve state.gaze değiştirilebilir!
        } else if (state.mode == 3) {
            // FPS modunda mouse ile hareket zaten aktif
        }
        lastMode = state.mode;




        glm::mat4x4 proj = glm::perspective(glm::radians(50.0f),
                                            float(state.width) / float(state.height),
                                            0.01f, 100.0f);
        glm::mat4x4 view = glm::lookAt(state.pos, state.gaze, state.up);


        glEnable(GL_CULL_FACE);

        // (Tekrar tanımlama kaldırıldı)

        // Her gezegeni sırayla çiz
        for(size_t i=0; i<planets.size(); ++i) {
            const Planet& planet = planets[i];
            glm::vec3 pos = planetPositions[i];

            // Model matrisi: yörünge + kendi etrafında dönme + ölçek
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, planet.selfSpeed * time, glm::vec3(0,1,0));
            model = glm::scale(model, glm::vec3(planet.radius));
            glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));

            // Vertex shader
            glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
            glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
            glUniformMatrix4fv(locModel, 1, false, glm::value_ptr(model));
            glUniformMatrix4fv(locView, 1, false, glm::value_ptr(view));
            glUniformMatrix4fv(locProj, 1, false, glm::value_ptr(proj));
            glUniformMatrix3fv(locNormal, 1, false, glm::value_ptr(normalMatrix));

            // Fragment shader
            glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
            glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
            
            // Shadow map uniforms (for all planets)
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, shadowColorTex);
            glBindSampler(4, samplerLinear);
            glUniform1i(locShadowMap, 4);
            glUniformMatrix4fv(locLightVP, 1, false, glm::value_ptr(lightVP));
            glUniform1f(locShadowBias, 0.002f); // Bias for shadow acne
            glUniform3fv(locLightDir, 1, glm::value_ptr(lightDir));

            if (i == 0) { // Earth
                // Albedo
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texEarth.textureId);
                glBindSampler(0, samplerLinear);
                // Specular
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texEarthSpecular.textureId);
                glBindSampler(1, samplerLinear);
                // Night
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texEarthNight.textureId);
                glBindSampler(2, samplerLinear);
                // Uniformlar (shader'da karşılığı olmalı)
                glUniform1i(glGetUniformLocation(fShader.shaderId, "uAlbedoMap"), 0);
                glUniform1i(glGetUniformLocation(fShader.shaderId, "uSpecularMap"), 1);
                glUniform1i(glGetUniformLocation(fShader.shaderId, "uNightMap"), 2);
                // View direction: Kamera → gezegen merkezi
                glm::vec3 viewDir = glm::normalize(state.pos - planetPositions[i]);
                glUniform3fv(locViewDir, 1, glm::value_ptr(viewDir));
                glUniform1ui(locMode, 10); // Earth özel shading mode (shader'da kontrol et)
            } else {
                glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
                glBindTexture(GL_TEXTURE_2D, planet.texture->textureId);
                glBindSampler(T_ALBEDO, samplerLinear);
                // View direction for specular (other planets too)
                glm::vec3 viewDir = glm::normalize(state.pos - planetPositions[i]);
                glUniform3fv(locViewDir, 1, glm::value_ptr(viewDir));
                glUniform1ui(locMode, 4); // Texture with diffuse + specular + shadow
            }

            glBindVertexArray(sphereMesh.vaoId);
            glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, nullptr);

            // --- Cloud küresi (Earth için) ---
            if (i == 0) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                // Bulutlar için ayrı model matrisi: Earth'den bağımsız dönüş hızı
                float cloudRotationSpeed = 0.05f; // Earth'den farklı hız
                glm::mat4 cloudModel = glm::translate(glm::mat4(1.0f), pos);
                cloudModel = glm::rotate(cloudModel, cloudRotationSpeed * time, glm::vec3(0,1,0));
                cloudModel = glm::scale(cloudModel, glm::vec3(planet.radius * 1.01f)); // Biraz daha büyük küre
                glm::mat3 cloudNormalMatrix = glm::inverseTranspose(glm::mat3(cloudModel));
                
                glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
                glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
                glUniformMatrix4fv(locModel, 1, false, glm::value_ptr(cloudModel));
                glUniformMatrix3fv(locNormal, 1, false, glm::value_ptr(cloudNormalMatrix));
                
                glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
                glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texEarthClouds.textureId);
                glBindSampler(0, samplerLinear);
                glUniform1ui(locMode, 11); // Cloud rendering mode
                glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, nullptr);
                glDisable(GL_BLEND);
            }
        }

        glfwSwapBuffers(state.window);
    }

}
