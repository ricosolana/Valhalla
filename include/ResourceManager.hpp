#pragma once

#include <map>
#include <string>

#include <GL/glew.h>
#include <vector>

#include "Texture2D.hpp"
#include "Shader.hpp"

namespace Alchyme {
    namespace ResourceManager {

        void SetRoot(const char* r);

        bool ReadFileBytes(const char* path, std::vector<unsigned char> &buffer);
        bool ReadFileBytes(const char* path, std::string& buffer);

        // loads (and generates) a shader program from file loading vertex, fragment (and geometry) shader's source code. If gShaderFile is not nullptr, it also loads a geometry shader
        Shader    LoadShader(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile, std::string name);
        // retrieves a stored sader
        Shader    GetShader(std::string name);
        // loads (and generates) a texture from file
        Texture2D LoadTexture(const char* file, bool alpha, std::string name);
        // retrieves a stored texture
        Texture2D GetTexture(std::string name);
        // properly de-allocates all loaded resources
        void      Clear();
    };
}
