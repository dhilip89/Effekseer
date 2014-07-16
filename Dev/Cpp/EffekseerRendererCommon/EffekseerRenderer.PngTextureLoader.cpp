
//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include "EffekseerRenderer.PngTextureLoader.h"

#if _WIN32
#define __PNG_DDI 1
#else
#define __PNG_DDI 0
#endif

#if __PNG_DDI
#define WINVER          0x0501
#define _WIN32_WINNT    0x0501
#include <windows.h>
#include <gdiplus.h>
#else
#include <png.h>
#endif

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
namespace EffekseerRenderer
{
#if __PNG_DDI
static Gdiplus::GdiplusStartupInput		gdiplusStartupInput;
static ULONG_PTR						gdiplusToken;
#else
static void PngReadData(png_structp png_ptr, png_bytep data, png_size_t length)
{
	auto d = (uint8_t**) png_get_io_ptr(png_ptr);
	memcpy(data, *d, length);
	(*d) += length;
}
#endif

std::vector<uint8_t> PngTextureLoader::textureData;
int32_t PngTextureLoader::textureWidth;
int32_t PngTextureLoader::textureHeight;

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool PngTextureLoader::Load(void* data, int32_t size, bool rev)
{
#if __PNG_DDI
	auto global = GlobalAlloc(GMEM_MOVEABLE,size);
	auto buf = GlobalLock(global);
	CopyMemory(buf, data, size);
	GlobalUnlock(global);
	LPSTREAM stream = NULL;
	CreateStreamOnHGlobal( global, false, &stream);
	Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(stream);
	ES_SAFE_RELEASE(stream);
	GlobalFree(global);


	if( bmp != NULL && bmp->GetLastStatus() == Gdiplus::Ok )
	{
		textureWidth = bmp->GetWidth();
		textureHeight = bmp->GetHeight();
		textureData.resize(textureWidth * textureHeight * 4);

		if(rev)
		{
			for(auto y = 0; y < textureHeight; y++ )
			{
				for(auto x = 0; x < textureWidth; x++ )
				{
					Gdiplus::Color  color;
					bmp->GetPixel(x, textureHeight - y - 1, &color);

					textureData[(x + y * textureWidth) * 4 + 0] = color.GetR();
					textureData[(x + y * textureWidth) * 4 + 1] = color.GetG();
					textureData[(x + y * textureWidth) * 4 + 2] = color.GetB();
					textureData[(x + y * textureWidth) * 4 + 3] = color.GetA();
				}
			}
		}
		else
		{
			for(auto y = 0; y < textureHeight; y++ )
			{
				for(auto x = 0; x < textureWidth; x++ )
				{
					Gdiplus::Color  color;
					bmp->GetPixel(x, y, &color);

					textureData[(x + y * textureWidth) * 4 + 0] = color.GetR();
					textureData[(x + y * textureWidth) * 4 + 1] = color.GetG();
					textureData[(x + y * textureWidth) * 4 + 2] = color.GetB();
					textureData[(x + y * textureWidth) * 4 + 3] = color.GetA();
				}
			}
		}
		
		return true;
	}
	else
	{
		ES_SAFE_DELETE(bmp);
		return false;
	}
#else
	uint8_t* data_ = (uint8_t*) data;

	/* png�A�N�Z�X�\���̂��쐬 */
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	/* ���[�h�R�[���o�b�N�֐��w�� */
	png_set_read_fn(png, &data_, &PngReadData);

	/* png�摜���\���̂��쐬 */
	png_infop png_info = png_create_info_struct(png);

	/* �G���[�n���h�����O */
	if (setjmp(png_jmpbuf(png)))
	{
		png_destroy_read_struct(&png, &png_info, NULL);
		return false;
	}

	/* IHDR�`�����N�����擾 */
	png_read_info(png, png_info);

	/* RGBA8888�t�H�[�}�b�g�ɕϊ����� */
	if (png_info->bit_depth < 8)
	{
		png_set_packing(png);
	}
	else if (png_info->bit_depth == 16)
	{
		png_set_strip_16(png);
	}

	uint32_t pixelBytes = 4;
	switch (png_info->color_type)
	{
	case PNG_COLOR_TYPE_PALETTE:
		png_set_palette_to_rgb(png);
		pixelBytes = 4;
		break;
	case PNG_COLOR_TYPE_GRAY:
		png_set_expand_gray_1_2_4_to_8(png);
		pixelBytes = 3;
		break;
	case PNG_COLOR_TYPE_RGB:
		pixelBytes = 3;
		break;
	case PNG_COLOR_TYPE_RGBA:
		break;
	}

	uint8_t* image = new uint8_t[png_info->width * png_info->height * pixelBytes];
	uint32_t pitch = png_info->width * pixelBytes;

	/* �C���[�W�f�[�^��ǂݍ��� */

	textureWidth = png_info->width;
	textureHeight = png_info->height;
	textureData.resize(textureWidth * textureHeight * 4);

	if (rev)
	{
		for (uint32_t i = 0; i < png_info->height; i++)
		{
			png_read_row(png, &image[(png_info->height - 1 - i) * pitch], NULL);
		}
	}
	else
	{
		for (uint32_t i = 0; i < png_info->height; i++)
		{
			png_read_row(png, &image[i * pitch], NULL);
		}
	}

	if (pixelBytes == 4)
	{
		memcpy(textureData.data(), image, png_info->width * png_info->height * pixelBytes);
	}
	else
	{
		for (int32_t y = 0; y < png_info->height; y++)
		{
			for (int32_t x = 0; x < png_info->width; x++)
			{
				auto src = (x + y * png_info->width) * 3;
				auto dst = (x + y * png_info->width) * 4;
				textureData[dst + 0] = image[src + 0];
				textureData[dst + 1] = image[src + 1];
				textureData[dst + 2] = image[src + 2];
				textureData[dst + 3] = 255;
			}
		}
	}
	
	delete [] image;
	png_destroy_read_struct(&png, &png_info, NULL);

	return true;
#endif
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void PngTextureLoader::Unload()
{
	textureData.clear();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void PngTextureLoader::Initialize()
{
#if __PNG_DDI
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL );
#endif
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void PngTextureLoader::Finalize()
{
#if __PNG_DDI
	Gdiplus::GdiplusShutdown(gdiplusToken);
#endif
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
}
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------