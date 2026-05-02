//
// Created by tjall on 07/04/2026.
//

#include "Shader.h"

#include <fstream>
#include <sstream>

#include "imgui_impl_opengl3_loader.h"

std::string readFileToString(const std::string &filePath) {
    std::ifstream fileStream(filePath, std::ios::in);
    if (!fileStream.is_open()) {
        printf("Could not open file: %s\n", filePath.c_str());
        return "";
    }
    std::stringstream buffer;
    buffer << fileStream.rdbuf();
    return buffer.str();
}

GLuint generateShader(const std::string &shaderPath, GLuint shaderType) {
    printf("Loading shader: %s\n", shaderPath.c_str());
    const std::string shaderText = readFileToString(shaderPath);
    const GLuint shader = glCreateShader(shaderType);
    const char *s_str = shaderText.c_str();
    glShaderSource(shader, 1, &s_str, nullptr);
    glCompileShader(shader);
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Error! Shader issue [%s]: %s\n", shaderPath.c_str(), infoLog);
    }
    return shader;
}

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    const GLuint vertexShader = generateShader(vertexPath, GL_VERTEX_SHADER);
    const GLuint fragmentShader = generateShader(fragmentPath, GL_FRAGMENT_SHADER);

    int success;
    char infoLog[512];
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Error! Making Shader Program: %s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::Use()
{
    glUseProgram(shaderProgram);
}

void Shader::setVec3(std::string name, const glm::vec3& value)
{
    glUniform3f(glGetUniformLocation(shaderProgram, name.c_str()), value[0], value[1], value[2]);
}

void Shader::setFloat(std::string name, float value)
{
    glUniform1f(glGetUniformLocation(this->shaderProgram, name.c_str()), value);
}
