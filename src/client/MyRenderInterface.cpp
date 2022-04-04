#include <RmlUi/Core.h>

#include "Utils.hpp"
#include "ResourceManager.hpp"
#include "MyRenderInterface.hpp"

#if !(SDL_VIDEO_RENDER_OGL)
#error "Only the opengl sdl backend is supported."
#endif

namespace Alchyme {
    MyRenderInterface::MyRenderInterface(SDL_Renderer* renderer, SDL_Window* screen)
    {
        mRenderer = renderer;
        mScreen = screen;
    }

    // Called by RmlUi when it wants to render geometry that it does not wish to optimise.
    void MyRenderInterface::RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rml::TextureHandle texture, const Rml::Vector2f& translation)
    {
        // SDL uses shaders that we need to disable here  
        glUseProgramObjectARB(0);
        glPushMatrix();
        glTranslatef(translation.x, translation.y, 0);

        Rml::Vector<Rml::Vector2f> Positions(num_vertices);
        Rml::Vector<Rml::Colourb> Colors(num_vertices);
        Rml::Vector<Rml::Vector2f> TexCoords(num_vertices);
        float texw = 0.0f;
        float texh = 0.0f;

        SDL_Texture* sdl_texture = nullptr;
        if (texture)
        {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            sdl_texture = (SDL_Texture*)texture;
            SDL_GL_BindTexture(sdl_texture, &texw, &texh);
        }

        for (int i = 0; i < num_vertices; i++) {
            Positions[i] = vertices[i].position;
            Colors[i] = vertices[i].colour;
            if (sdl_texture) {
                TexCoords[i].x = vertices[i].tex_coord.x * texw;
                TexCoords[i].y = vertices[i].tex_coord.y * texh;
            }
            else TexCoords[i] = vertices[i].tex_coord;
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, &Positions[0]);
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, &Colors[0]);
        glTexCoordPointer(2, GL_FLOAT, 0, &TexCoords[0]);

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, indices);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        if (sdl_texture) {
            SDL_GL_UnbindTexture(sdl_texture);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        }

        glColor4f(1.0, 1.0, 1.0, 1.0);
        glPopMatrix();
        /* Reset blending and draw a fake point just outside the screen to let SDL know that it needs to reset its state in case it wants to render a texture */
        glDisable(GL_BLEND);
        SDL_SetRenderDrawBlendMode(mRenderer, SDL_BLENDMODE_NONE);
        SDL_RenderDrawPoint(mRenderer, -1, -1);
    }


    // Called by RmlUi when it wants to enable or disable scissoring to clip content.		
    void MyRenderInterface::EnableScissorRegion(bool enable)
    {
        if (enable)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
    }

    // Called by RmlUi when it wants to change the scissor region.		
    void MyRenderInterface::SetScissorRegion(int x, int y, int width, int height)
    {
        int w_width, w_height;
        SDL_GetWindowSize(mScreen, &w_width, &w_height);
        glScissor(x, w_height - (y + height), width, height);
    }

    // Called by RmlUi when a texture is required by the library.		
    bool MyRenderInterface::LoadTexture(Rml::TextureHandle& texture_handle,
        Rml::Vector2i& texture_dimensions, const Rml::String& source)
    {

        return false;

        Rml::FileInterface* file_interface = Rml::GetFileInterface();
        Rml::FileHandle file_handle = file_interface->Open(source);
        if (!file_handle)
            return false;
        


        file_interface->Seek(file_handle, 0, SEEK_END);
        auto buffer_size = file_interface->Tell(file_handle);
        file_interface->Seek(file_handle, 0, SEEK_SET);

        std::vector<unsigned char> buffer;
        buffer.resize(buffer_size);



        //char* buffer = new char[buffer_size];
        file_interface->Read(buffer.data(), buffer_size, file_handle);
        file_interface->Close(file_handle);

        int w, h, n;
        unsigned char* data = stbi_load_from_memory(buffer.data(), buffer.size(), &w, &h, &n, 4);
        if (data == nullptr) {
            return false;
        }

        // Create textures on the gpu
        GLuint texture;
        glGenTextures(1, &texture);

        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Bind to configure the texture
        glBindTexture(GL_TEXTURE_2D, texture);

        // Load texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);

        return false;
    }

    // Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
    bool MyRenderInterface::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions)
    {

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        Uint32 rmask = 0xff000000;
        Uint32 gmask = 0x00ff0000;
        Uint32 bmask = 0x0000ff00;
        Uint32 amask = 0x000000ff;
#else
        Uint32 rmask = 0x000000ff;
        Uint32 gmask = 0x0000ff00;
        Uint32 bmask = 0x00ff0000;
        Uint32 amask = 0xff000000;
#endif

        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*)source, source_dimensions.x, source_dimensions.y, 32, source_dimensions.x * 4, rmask, gmask, bmask, amask);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(mRenderer, surface);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_FreeSurface(surface);
        texture_handle = (Rml::TextureHandle)texture;
        return true;
    }

    // Called by RmlUi when a loaded texture is no longer required.		
    void MyRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle)
    {
        //SDL_DestroyTexture((SDL_Texture*)texture_handle);
    }
}
