/*
[CyoClip] CyoClip.cpp

MIT License

Copyright (c) 2019 Graham Bull

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "pch.h"

namespace
{
	class Clipboard
	{
	public:
		Clipboard()
		{
			if (!::OpenClipboard(NULL))
				throw std::runtime_error("Unable to open clipboard");

			if (!::EmptyClipboard())
				throw std::runtime_error("Unable to empty clipboard");
		}
		~Clipboard()
		{
			::CloseClipboard();
		}
		void SetText(HANDLE mem, bool unicode)
		{
			if (unicode)
			{
				if (::SetClipboardData(CF_UNICODETEXT, mem) == NULL)
					throw std::runtime_error("Unable to set clipboard data");
			}
			else
			{
				if (::SetClipboardData(CF_TEXT, mem) == NULL)
					throw std::runtime_error("Unable to set clipboard data");
			}
		}
	};

	using bytes_t = std::vector<unsigned char>;

	class Memory
	{
	public:
		Memory(const bytes_t& source, int offset)
		{
			auto size = (source.size() - offset);

			mem_ = ::GlobalAlloc(GMEM_MOVEABLE, size);
			if (mem_ == NULL)
				throw std::runtime_error("Unable to allocate memory");

			auto dest = ::GlobalLock(mem_);
			if (dest == NULL)
				throw std::runtime_error("Unable to lock memory");

			::CopyMemory(dest, &source[offset], size);

			::GlobalUnlock(mem_);
		}
		~Memory()
		{
			if (mem_ != nullptr)
			{
				::GlobalFree(mem_);
				mem_ = nullptr;
			}
		}
		operator HANDLE() const { return mem_; }

	private:
		HGLOBAL mem_ = nullptr;
	};

	bool FileIsUtf16(const bytes_t& bytes)
	{
		if (bytes.size() < 2)
			return false;

		return (bytes[0] == 0xFF && bytes[1] == 0xFE) //little endian
			|| (bytes[0] == 0xFE && bytes[1] == 0xFF); //big endian
	}

	bool FileIsUtf8(const bytes_t& bytes)
	{
		if (bytes.size() < 3)
			return false;

		return (bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF);
	}

	void DetermineFileType(const bytes_t& bytes, bool& unicode, int& offset)
	{
		if (FileIsUtf16(bytes))
		{
			unicode = true;
			offset = 2;
		}
		else if (FileIsUtf8(bytes))
		{
			unicode = false;
			offset = 3;
		}
		else
		{
			unicode = false;
			offset = 0;
		}
	}
}

int wmain(int argc, wchar_t* argv[])
{
	try
	{
		auto hStdIn = ::GetStdHandle(STD_INPUT_HANDLE);

		const size_t MaxBytes = (1024 * 1024); //1MB
		bytes_t bytes(MaxBytes);

		DWORD numBytes;
		::ReadFile(hStdIn, &bytes[0], MaxBytes, &numBytes, nullptr);

		bytes.resize(numBytes);

		bool unicode;
		int offset;
		DetermineFileType(bytes, unicode, offset);

		Memory memory(bytes, offset);

		Clipboard clipboard;
		clipboard.SetText(memory, unicode);

		return 0;
	}
	catch (const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}
}
