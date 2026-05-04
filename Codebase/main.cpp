#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <stb_image.h>

#include "core/mesh.h"
#include "core/assimpLoader.h"
#include "core/Shader.h"
#include "core/texture.h"

//#define MAC_CLION
#define VSTUDIO

#ifdef MAC_CLION
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#endif

#ifdef VSTUDIO
// Note: install imgui with:
//     ./vcpkg.exe install imgui[glfw-binding,opengl3-binding]
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#endif

int g_width = 800;
int g_height = 600;

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebufferSizeCallback(GLFWwindow *window,
                             int width, int height) {
    g_width = width;
    g_height = height;
    glViewport(0, 0, width, height);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(g_width, g_height, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    //Setup platforms
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 400");

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    stbi_set_flip_vertically_on_load(true);

    Shader* geometryShaderProgram = new Shader("shaders/deferred_geometry_vertex.vsh","shaders/deferred_geometry_fragment.fsh");
    Shader* lightingShaderProgram = new Shader("shaders/deferred_lighting_vertex.vsh","shaders/deferred_lighting_fragment.fsh");
    Shader* lightBoxShaderProgram = new Shader("shaders/deferred_lighting_vertex.vsh","shaders/deferred_lighting_fragment.fsh");

    core::Mesh quad = core::Mesh::generateQuad();
    core::Model quadModel({quad});
    //quadModel.translate(glm::vec3(0,0,-2.5));
    //quadModel.scale(glm::vec3(5, 5, 1));

    core::Model suzanne = core::AssimpLoader::loadModel("models/sphere_flat.obj");
    core::Model box = core::AssimpLoader::loadModel("models/box.obj");

    int grid_width = 10;
    int grid_height = 10;

    std::vector<core::Model> spheres;

    for (int x = 0; x < grid_width; x++)
    {
        for (int y = 0; y < grid_height; y++)
        {
            core::Model sphere = core::AssimpLoader::loadModel("models/sphere.obj");

            sphere.translate(glm::vec3(x * 2.5, 0.0f, y * 2.5));

            spheres.push_back(sphere);
        }
    }

    core::Texture cmgtGatoTexture("textures/CMGaTo_crop.png");

    glm::vec4 clearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    glClearColor(clearColor.r,
                 clearColor.g, clearColor.b, clearColor.a);

    glm::vec3 cameraPos = glm::vec3(-10.0f, 10.0f, -10.0f);
    glm::vec3 cameraTarget = glm::vec3(12.5f, 0.0f, 12.5f);
    glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));
    glm::vec3 cameraUp = glm::cross(cameraDirection, cameraRight);

    //VP
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(g_width) / static_cast<float>(g_height), 0.1f, 100.0f);

    GLint modelMatrixUniform = glGetUniformLocation(geometryShaderProgram->shaderProgram, "model");
    GLint viewMatrixUniform = glGetUniformLocation(geometryShaderProgram->shaderProgram, "view");
    GLint projectionMatrixUniform = glGetUniformLocation(geometryShaderProgram->shaderProgram, "projection");

    // configure g-buffer framebuffer
    // ------------------------------
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition;
    unsigned int gNormal;
    unsigned int gAlbedoSpec;
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, g_width, g_height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, g_width, g_height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_width, g_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_width, g_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    double currentTime = glfwGetTime();
    double finishFrameTime = 0.0;
    float deltaTime = 0.0f;
    float rotationStrength = 100.0f;

    // lighting info
    // -------------
    const unsigned int NR_LIGHTS = 32;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        // calculate slightly random offsets
        float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
        float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
        float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
        lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
        // also calculate random color
        float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.0
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }

    // shader configuration
    // --------------------
    lightingShaderProgram->Use();
    glUniform1i(glGetUniformLocation(lightingShaderProgram->shaderProgram, "gPosition"), 0);
    glUniform1i(glGetUniformLocation(lightingShaderProgram->shaderProgram, "gNormal"), 1);
    glUniform1i(glGetUniformLocation(lightingShaderProgram->shaderProgram, "gAlbedoSpec"), 2);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Raw Engine v2");
        ImGui::Text("Hello :)");
        ImGui::End();

        processInput(window);

        for (int i = 0; i < spheres.size(); i++)
        {
            spheres[i].rotate(glm::vec3(0.0f, 1.0f, 0.0f), glm::radians(rotationStrength) * static_cast<float>(deltaTime));
        }

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. geometry pass: render scene's geometry/color data into gbuffer
        // -----------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (int i = 0; i < spheres.size(); i++)
        {
            geometryShaderProgram->Use();
            glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, glm::value_ptr(spheres[i].getModelMatrix()));
            glUniformMatrix4fv(viewMatrixUniform, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, glm::value_ptr(projection));

            spheres[i].render();
            glBindVertexArray(0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
        // -----------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        lightingShaderProgram->Use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

        // send light relevant uniforms
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            lightingShaderProgram->setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
            lightingShaderProgram->setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
            // update attenuation parameters and calculate radius
            const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
            const float linear = 0.7f;
            const float quadratic = 1.8f;
            lightingShaderProgram->setFloat("lights[" + std::to_string(i) + "].Linear", linear);
            lightingShaderProgram->setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
            // then calculate radius of light volume/sphere
            const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
            float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
            lightingShaderProgram->setFloat("lights[" + std::to_string(i) + "].Radius", radius);
        }
        glUniform3f(glGetUniformLocation(lightingShaderProgram->shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

        quadModel.render();

        // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
        // ----------------------------------------------------------------------------------
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
        // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
        // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the
        // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
        glBlitFramebuffer(0, 0, g_width, g_height, 0, 0, g_width, g_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3. render lights on top of scene
        // --------------------------------
        lightBoxShaderProgram->Use();
        GLint lightBoxMvpMatrixUniform = glGetUniformLocation(lightBoxShaderProgram->shaderProgram, "mvp");
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, lightPositions[i]);
            model = glm::scale(model, glm::vec3(0.125f));
            //shaderLightBox.setMat4("model", model);

            glUniformMatrix4fv(lightBoxMvpMatrixUniform, 1, GL_FALSE, glm::value_ptr(projection * view * box.getModelMatrix()));
            //shaderLightBox.setVec3("lightColor", lightColors[i]);
            box.render();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
        finishFrameTime = glfwGetTime();
        deltaTime = static_cast<float>(finishFrameTime - currentTime);
        currentTime = finishFrameTime;
    }

    glDeleteProgram(geometryShaderProgram->shaderProgram);
    glDeleteShader(lightingShaderProgram->shaderProgram);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}