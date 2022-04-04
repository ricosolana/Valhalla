#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <SDL.h>
#include <GL/glew.h>

#if !(SDL_VIDEO_RENDER_OGL)
#error "Only the opengl sdl backend is supported."
#endif

namespace Alchyme {
	class MyRenderInterface : public Rml::RenderInterface
	{
	public:
		MyRenderInterface(SDL_Renderer* renderer, SDL_Window* screen);

		/// Called by RmlUi when it wants to render geometry that it does not wish to optimise.
		void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override;

		/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
		void EnableScissorRegion(bool enable) override;
		/// Called by RmlUi when it wants to change the scissor region.
		void SetScissorRegion(int x, int y, int width, int height) override;

		/// Called by RmlUi when a texture is required by the library.
		bool LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
		/// Called by RmlUi when a texture is required to be built from an internally-generated sequence of pixels.
		bool GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source, const Rml::Vector2i& source_dimensions) override;
		/// Called by RmlUi when a loaded texture is no longer required.
		void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	private:
		SDL_Renderer* mRenderer;
		SDL_Window* mScreen;
	};
}
