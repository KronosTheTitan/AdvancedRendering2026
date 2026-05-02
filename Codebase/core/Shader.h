//
// Created by tjall on 07/04/2026.
//

#ifndef RAWENGINE_SHADER_H
#define RAWENGINE_SHADER_H
#include <string>
#include <glm/vec3.hpp>
#include <glad/glad.h>

#include "imgui_impl_opengl3_loader.h"


class Shader {
public:
    unsigned int shaderProgram;

    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    void Use();
    void setVec3(std::string name, const glm::vec3& value);
    void setFloat(std::string name, float value);
};



#endif //RAWENGINE_SHADER_H
