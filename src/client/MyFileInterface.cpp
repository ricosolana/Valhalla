#include "MyFileInterface.hpp"
#include <stdio.h>

namespace Alchyme {
	MyFileInterface::MyFileInterface(const Rml::String& root) : root(root)
	{
	}

	MyFileInterface::~MyFileInterface()
	{
	}

	// Opens a file.
	Rml::FileHandle MyFileInterface::Open(const Rml::String& path)
	{
		// Attempt to open the file relative to the application's root.
		FILE* fp = fopen((root + path).c_str(), "rb");
		if (fp != nullptr)
			return (Rml::FileHandle)fp;

		// Attempt to open the file relative to the current working directory.
		fp = fopen(path.c_str(), "rb");
		return (Rml::FileHandle)fp;
	}

	// Closes a previously opened file.
	void MyFileInterface::Close(Rml::FileHandle file)
	{
		fclose((FILE*)file);
	}

	// Reads data from a previously opened file.
	size_t MyFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file)
	{
		return fread(buffer, 1, size, (FILE*)file);
	}

	// Seeks to a point in a previously opened file.
	bool MyFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
	{
		return fseek((FILE*)file, offset, origin) == 0;
	}

	// Returns the current position of the file pointer.
	size_t MyFileInterface::Tell(Rml::FileHandle file)
	{
		return ftell((FILE*)file);
	}
}
