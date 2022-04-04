#include "ResourceManager.hpp"

#include <iostream>
#include <sstream>
#include <fstream>

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#include <robin_hood.h>

namespace Alchyme {
    namespace ResourceManager {
        // Instantiate static variables
        std::string root;
        robin_hood::unordered_map<std::string, Texture2D>    Textures;
        robin_hood::unordered_map<std::string, Shader>       Shaders;

        void SetRoot(const char* r) {
            root = r;
        }

        FILE* OpenFile(const std::string &path) {
            return fopen((root + path).c_str(), "rb");
        }

        bool ReadFileBytes(const char* path, std::vector<unsigned char> &buffer) {
            auto f = OpenFile(path);

            if (!f)
                return false;

            fseek(f, 0, SEEK_END);
            auto buffer_size = ftell(f);
            fseek(f, 0, SEEK_SET);

            buffer.resize(buffer_size);

            //char* buffer = new char[buffer_size];
            fread(buffer.data(), 1, buffer_size, f);
            fclose(f);
            return true;
        }

        bool ReadFileBytes(const char* path, std::string& buffer) {
            auto f = OpenFile(path);

            if (!f)
                return false;

            fseek(f, 0, SEEK_END);
            auto buffer_size = ftell(f);
            fseek(f, 0, SEEK_SET);

            buffer.resize(buffer_size);

            //char* buffer = new char[buffer_size];
            fread(buffer.data(), 1, buffer_size, f);
            fclose(f);
            return true;
        }

        Shader loadShaderFromFile(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile)
        {
            // 1. retrieve the vertex/fragment source code from filePath
            std::string vertexCode;
            std::string fragmentCode;
            std::string geometryCode;

            if (!ReadFileBytes(vShaderFile, vertexCode))
                throw std::runtime_error("unable to load vertex shader file");
            if (!ReadFileBytes(fShaderFile, fragmentCode))
                throw std::runtime_error("unable to load frag shader file");
            if (!ReadFileBytes(gShaderFile, geometryCode))
                throw std::runtime_error("unable to load geometry shader file");

            const char* vShaderCode = vertexCode.c_str();
            const char* fShaderCode = fragmentCode.c_str();
            const char* gShaderCode = geometryCode.c_str();
            // 2. now create shader object from source code
            Shader shader;
            shader.Compile(vShaderCode, fShaderCode, gShaderFile != nullptr ? gShaderCode : nullptr);
            return shader;
        }

        Texture2D loadTextureFromFile(const char* file, bool alpha)
        {
            // create texture object
            Texture2D texture;
            if (alpha)
            {
                texture.Internal_Format = GL_RGBA;
                texture.Image_Format = GL_RGBA;
            }

            std::vector<unsigned char> out;
            ReadFileBytes(file, out);

            // load image
            int w, h, n;
            unsigned char* data = stbi_load_from_memory(out.data(), out.size(), &w, &h, &n, 4);
            // now generate texture
            texture.Generate(w, h, data);
            // and finally free image data
            stbi_image_free(data);
            return texture;
        }

        Shader LoadShader(const char* vShaderFile, const char* fShaderFile, const char* gShaderFile, std::string name)
        {
            Shaders[name] = loadShaderFromFile(vShaderFile, fShaderFile, gShaderFile);
            return Shaders[name];
        }

        Shader GetShader(std::string name)
        {
            return Shaders[name];
        }

        Texture2D LoadTexture(const char* file, bool alpha, std::string name)
        {
            Textures[name] = loadTextureFromFile(file, alpha);
            return Textures[name];
        }

        Texture2D GetTexture(std::string name)
        {
            return Textures[name];
        }

        void Clear()
        {
            // (properly) delete all shaders	
            for (auto iter : Shaders)
                glDeleteProgram(iter.second.ID);
            // (properly) delete all textures
            for (auto iter : Textures)
                glDeleteTextures(1, &iter.second.ID);
        }
    }
}