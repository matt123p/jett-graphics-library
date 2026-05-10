//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "Image.h"
#include "GPUProcessor.h"
#include <math.h>
#include <setjmp.h>

#include <tiffio.h>
#include <jpeglib.h>
#include <png.h>

#include "lcms_transform_sampler.h"

// Structures for loading BMP file
typedef struct RGBApixel {
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;
} RGBApixel;

struct BMFH
{
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
};

struct BMIH
{
	uint32_t biSize;
	int32_t  biWidth;
	int32_t  biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
};

static int IntPow(int base, int exponent)
{
	int i;
	int output = 1;
	for (i = 0; i < exponent; i++)
	{
		output *= base;
	}
	return output;
}

CImage::CImage()
{
	m_type = image_invalid;
	m_16bpp = false;
	m_width = 0;
	m_height = 0;
	m_stride = 0;
	m_data = NULL;
	m_cl = NULL;
	m_cl_data = NULL;
	m_profile_data = NULL;
	m_profile_size = 0;
	m_x_res = 72;
	m_y_res = 72;
	m_clip_x1 = 0;
	m_clip_y1 = 0;
	m_clip_width = 0;
	m_clip_height = 0;
	m_refs = 0;

#ifdef _WIN32
	m_bitmap = NULL;
#endif

	m_mode = image_mode_default;
	m_cpu_changed = false;
	m_gpu_changed = false;
	m_gpu_write_pending = false;
}

CImage::~CImage()
{
	discard();
}

// Discard this bitmap from memory
void CImage::discard()
{
	delete[] m_profile_data;
	m_profile_data = NULL;
	m_profile_size = 0;

#ifdef _WIN32
	if (m_bitmap)
	{
		// Delete the bitmap
		DeleteObject(m_bitmap);
	}
	else
#endif
	{
		delete[] m_data;
	}
#ifdef _WIN32
	m_bitmap = NULL;
#endif
	m_data = NULL;

	if (m_cl_data && m_cl)
	{
		m_cl->releaseMemObject(m_cl_data);
	}

	m_cl_data = NULL;
	m_type = image_invalid;
	m_16bpp = false;
	m_width = 0;
	m_height = 0;
	m_stride = 0;
	m_x_res = 72;
	m_y_res = 72;
	m_clip_x1 = 0;
	m_clip_y1 = 0;
	m_clip_width = 0;
	m_clip_height = 0;
	m_cl_data = NULL;
	m_mode = image_mode_default;
	m_cpu_changed = false;
	m_gpu_changed = false;
	m_gpu_write_pending = false;
}

void CImage::createImage(unsigned int width, unsigned int height, image_t type, bool image_16bpp, image_mode_t mode)
{
	// Erase the old image data
	discard();
	delete[] m_data;
	delete[] m_profile_data;
	m_data = NULL;
	m_profile_data = NULL;
	m_profile_size = 0;

	// Now setup the new data
	m_width = width;
	m_height = height;
	m_clip_x1 = 0;
	m_clip_y1 = 0;
	m_clip_width = width;
	m_clip_height = height;

	m_type = type;
	m_16bpp = image_16bpp;

	int bytes_per_col = m_16bpp ? 2 : 1;

	switch (type)
	{
	case image_mono1:
		m_stride = (m_width / 8 + 3) / 4 * 4;
		break;
	case image_mono2:
		m_stride = (m_width / 4 + 3) / 4 * 4;
		break;
	case image_mono4:
		m_stride = (m_width / 2 + 3) / 4 * 4;
		break;
	default:
		m_stride = (getPixelStride() * m_width * bytes_per_col + 3) / 4 * 4;
		break;
	}

	// Generate the image buffer
	delete[] m_data;
	m_data = new unsigned char[m_height * m_stride];

	m_mode = mode;
	m_cpu_changed = false;
	m_gpu_changed = false;
	m_gpu_write_pending = false;
}

#ifdef _WIN32
HBITMAP CImage::createBitmap(unsigned int width, unsigned int height, image_t type)
{
	// Make sure it is compatible with a windows Bitmap
	switch (type)
	{
	case image_rgb:
	case image_rgba:
	case image_cmyk:
	case image_lab:
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "The bitmap type is not compatible with a Windows HBITMAP");
		break;
	}

	// First create the bitmap
	discard();
	delete[] m_data;
	delete[] m_profile_data;
	m_data = NULL;
	m_profile_data = NULL;
	m_profile_size = 0;

	// Now setup the new data
	m_width = width;
	m_height = height;
	m_clip_x1 = 0;
	m_clip_y1 = 0;
	m_clip_width = width;
	m_clip_height = height;
	m_type = type;
	m_16bpp = false;
	int bpp = 8;

	switch (type)
	{
	case image_mono1:
		m_stride = (m_width / 8 + 3) / 4 * 4;
		bpp = 1;
		break;
	case image_mono2:
		m_stride = (m_width / 4 + 3) / 4 * 4;
		bpp = 2;
		break;
	case image_mono4:
		m_stride = (m_width / 2 + 3) / 4 * 4;
		bpp = 4;
		break;
	case image_mono:
		m_stride = (getPixelStride() * m_width + 3) / 4 * 4;
		bpp = 8;
		break;
	case image_bgr:
		m_stride = (getPixelStride() * m_width + 3) / 4 * 4;
		bpp = 24;
		break;
	case image_bgra:
		m_stride = (getPixelStride() * m_width + 3) / 4 * 4;
		bpp = 32;
		break;
	}

	// Generate the image buffer
	{
		struct {
			BITMAPINFOHEADER bi_dst;
			RGBQUAD	palette[256];
		} b;
		ZeroMemory(&b, sizeof(b));
		b.bi_dst.biSize = sizeof(b.bi_dst);
		b.bi_dst.biWidth = m_width;
		b.bi_dst.biHeight = m_height;
		b.bi_dst.biPlanes = 1;
		b.bi_dst.biBitCount = bpp;
		b.bi_dst.biCompression = BI_RGB;

		if (bpp <= 8)
		{
			// Write a monochrome colour table
			int NumberOfColors = IntPow(2, bpp);
			for (int n = 0; n < NumberOfColors; n++)
			{
				int l = (n * 255) / (NumberOfColors - 1);
				b.palette[n].rgbRed = l;
				b.palette[n].rgbGreen = l;
				b.palette[n].rgbBlue = l;
			}
		}

		PVOID bits = NULL;
		m_bitmap = CreateDIBSection(NULL, (BITMAPINFO*)&b.bi_dst, DIB_RGB_COLORS, &bits, NULL, 0);
		m_data = (unsigned char*)bits;
	}

	m_mode = image_mode_gpu_ro;
	m_cpu_changed = true;
	m_gpu_changed = false;
	m_gpu_write_pending = false;

	return m_bitmap;
}
#endif

void CImage::copy_bitmap(unsigned int width, unsigned int height, image_t type, image_mode_t mode, const unsigned char* data, size_t stride, int bpp, bool invert)
{
	createImage(width, height, type, false, mode);

	// Copy row at a time to avoid differences in stride
	size_t copy_stride = std::min(m_stride, stride);
	for (int y = 0; y < m_height; ++y)
	{
		const unsigned char* src = data + y * stride;
		unsigned char* dst = m_data + y * m_stride;

		switch (bpp)
		{
		case 1:
		{
			int b = 7;
			for (int x = 0; x < m_width; ++x)
			{
				bool c = ((*src) >> b) & 1;
				if (invert == c)
				{
					*dst = 0;
				}
				else
				{
					*dst = 255;
				}

				--b;
				if (b == -1)
				{
					++src;
					b = 7;
				}
				++dst;
			}
		}
		break;
		case 8:
			if (invert)
			{
				for (int x = 0; x < m_width; ++x)
				{
					*dst = 255 - *src;
					++src;
					++dst;
				}
			}
			else
			{
				memcpy(dst, src, copy_stride);
			}
			break;
		}
	}

	m_cpu_changed = true;
	m_gpu_changed = false;
}

void CImage::loadFromFile(const TCHAR* filename, image_mode_t mode)
{
	// Keep track of the user requested mode
	m_mode = mode;

	// Does this file exist?
	FILE* fin = NULL;
	_tfopen_s(&fin, filename, _T("rb"));
	if (!fin)
	{
		throw jett_exception(JETT_FILE_NOT_FOUND, 0, "Unable to open image file");
	}
	fclose(fin);

	m_cpu_changed = true;
	m_gpu_changed = false;

	if (loadFromFileTIFF(filename))
	{
		// Loaded!
		return;
	}
	else if (loadFromFileBMP(filename))
	{
		// Loaded!
		return;
	}
	else if (loadFromFilePNG(filename))
	{
		// Loaded!
		return;
	}
	else if (loadFromFileJPEG(filename))
	{
		// Loaded!
		return;
	}
	else
	{
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Cannot load image file");
	}
}

void CImage::saveToFile(const TCHAR* filename)
{
	// What is the extension?
	const TCHAR* extension = _tcsrchr(filename, '.');
	if (extension)
	{
		if (strcasecmp(extension, _T(".tiff")) == 0 || strcasecmp(extension, _T(".tif")) == 0)
		{
			saveToFileTIFF(filename);
			return;
		}
		else if (strcasecmp(extension, _T(".jpeg")) == 0 || strcasecmp(extension, _T(".jpg")) == 0 || strcasecmp(extension, _T(".jpe")) == 0)
		{
			saveToFileJPEG(filename);
			return;
		}
		else if (strcasecmp(extension, _T(".png")) == 0)
		{
			saveToFilePNG(filename);
			return;
		}
		else if (strcasecmp(extension, _T(".bmp")) == 0)
		{
			saveToFileBMP(filename);
			return;
		}
	}

	throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unknown file type");
}

static jmp_buf tif_jmp_buf;

// TIFF error handlers
static void image_tiffWarningHandler(const char* module, const char* fmt, va_list args)
{
	// Warnings are ignored
}

static char tiff_error_bufer[512];

static void image_tiffErrorHandler(const char* module, const char* fmt, va_list args)
{
	vsprintf_s(tiff_error_bufer, sizeof(tiff_error_bufer), (const char*)fmt, args);
	longjmp(tif_jmp_buf, 1);
}

// Create this image from a file
bool CImage::loadFromFileTIFF(const TCHAR* filename)
{
	TIFF* tif = NULL;

	// Set up the return function
	int err = setjmp(tif_jmp_buf);
	if (err)
	{
		// Do we throw this as an exception?
		if (tif)
		{
			throw jett_exception(JETT_INVALID_BITMAP, 0, tiff_error_bufer);
		}
		else
		{
			// No, it is probably not a TIF file
			return false;
		}
	}

	// Configure the error handlers
	TIFFSetWarningHandler(image_tiffWarningHandler);
	TIFFSetErrorHandler(image_tiffErrorHandler);

	// Open the TIFF file
#ifdef UNICODE
	tif = TIFFOpenW(filename, "r");
#else
	tif = TIFFOpen(filename, "r");
#endif

	if (!tif)
	{
		return false;
	}

	// Read the file size
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &m_width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &m_height);

	// Set the clipping rectangle
	m_clip_x1 = 0;
	m_clip_y1 = 0;
	m_clip_width = m_width;
	m_clip_height = m_height;

	// Determine the file format
	short photometric, samples_per_pixel, bps;
	TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);

	// Determine the image type
	if (photometric == PHOTOMETRIC_CIELAB && samples_per_pixel == 3)
	{
		// This is a Lab image
		m_type = image_lab;
	}
	else if (photometric == PHOTOMETRIC_SEPARATED && samples_per_pixel == 4)
	{
		// This is a CMYK image
		m_type = image_cmyk;
	}
	else if ((photometric == PHOTOMETRIC_MINISWHITE || photometric == PHOTOMETRIC_MINISBLACK) && samples_per_pixel == 1)
	{
		m_type = image_mono;
	}
	else if (photometric == PHOTOMETRIC_RGB && samples_per_pixel == 4)
	{
		m_type = image_rgba;
	}
	else if (photometric == PHOTOMETRIC_RGB && samples_per_pixel == 3)
	{
		m_type = image_rgb;
	}
	else
	{
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Unsupported TIFF type");
	}

	// Is this an 8 or 16 bit image?
	switch (bps)
	{
	case 8:
		m_16bpp = false;
		break;
	case 16:
		m_16bpp = true;
		break;
	default:
		// We only accept this if we are in rgb or rgba mode
		// because libtiff will convert it for us
		if (m_type == image_rgb || m_type == image_rgba)
		{
			m_type = image_rgba;
			m_16bpp = false;
		}
		else
		{
			throw jett_exception(JETT_INVALID_BITMAP, 0, "Unsupported TIFF type");
		}
		break;
	}

	// Is there a resolution
	m_x_res = 72;
	m_y_res = 72;
	uint16_t res_unit = 0;
	float xres, yres;
	if (TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &res_unit)
		&& TIFFGetField(tif, TIFFTAG_XRESOLUTION, &xres)
		&& TIFFGetField(tif, TIFFTAG_XRESOLUTION, &yres))
	{
		switch (res_unit)
		{
		case RESUNIT_NONE:
			break;
		case RESUNIT_INCH:
			m_x_res = static_cast<int>(xres + 0.5);
			m_y_res = static_cast<int>(yres + 0.5);
			break;
		case RESUNIT_CENTIMETER:
			m_x_res = static_cast<int>(xres / (10 / 25.4) + 0.5);
			m_y_res = static_cast<int>(yres / (10 / 25.4) + 0.5);
			break;
		}
	}

	// Create the bitmap
	createImage(m_width, m_height, m_type, m_16bpp, m_mode);

	// Load the embedded profile - if not L*a*b*
	// If it is L*a*b* then we subsitute the embedded profile
	// for our default one
	if (m_type != image_lab)
	{
		uint32_t EmbedLen = 0;
		uint8_t* EmbedBuffer = NULL;
		if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &EmbedLen, &EmbedBuffer))
		{
			m_profile_size = EmbedLen;
			m_profile_data = new unsigned char[EmbedLen];
			memcpy(m_profile_data, EmbedBuffer, EmbedLen);
		}
	}
	else
	{
		// Only for L*a*b* we add the default profile
		set_profile_data(_T(":lab"));
	}

	// Now load it from file
	if (m_type == image_rgba && !m_16bpp)
	{
		// Load using the special libtiff function
		TIFFReadRGBAImageOriented(tif, m_width, m_height, reinterpret_cast<uint32_t*>(m_data), ORIENTATION_TOPLEFT, 0);
	}
	else
	{
		// Load as RAW data!
		for (int y = 0; y < m_height; ++y)
		{
			unsigned char* p = m_data + y * m_stride;
			TIFFReadScanline(tif, p, static_cast<uint32_t>(y), 0);
		}
	}

	TIFFClose(tif);

	return true;
}

// JPEG error handlers
typedef struct my_error_mgr* my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
jpeg_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	// TODO: my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	throw jett_exception(JETT_INVALID_BITMAP, 0, "JPEG error");
}

// Create this image from a file
bool CImage::loadFromFileJPEG(const TCHAR* filename)
{
	/* these are standard libjpeg structures for reading(decompression) */
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	FILE* infile = NULL;
	_tfopen_s(&infile, filename, _T("rb"));
	if (!infile)
	{
		return false;
	}

	// here we set up the standard libjpeg error handler
	cinfo.err = jpeg_std_error(&jerr);

	// Set up our own error handlers
	jerr.error_exit = jpeg_error_exit;

	/* setup decompression process and source, then read JPEG header */
	try
	{
		jpeg_create_decompress(&cinfo);
		/* this makes the library read from infile */
		jpeg_stdio_src(&cinfo, infile);
		/* for the ICC coloour data */
		jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xffff);
		/* reading the image header which contains image information */
		jpeg_read_header(&cinfo, TRUE);

		/* Start decompression jpeg here */
		jpeg_start_decompress(&cinfo);

		m_x_res = 72;
		m_y_res = 72;

		m_16bpp = false;

		// Read image parameters
		m_width = cinfo.output_width;
		m_height = cinfo.output_height;

		// Set the clipping rectangle
		m_clip_x1 = 0;
		m_clip_y1 = 0;
		m_clip_width = m_width;
		m_clip_height = m_height;

		switch (cinfo.density_unit)
		{
		case RESUNIT_NONE:
			break;
		case RESUNIT_INCH:
			m_x_res = static_cast<int>(cinfo.X_density);
			m_y_res = static_cast<int>(cinfo.Y_density);
			break;
		case RESUNIT_CENTIMETER:
			m_x_res = static_cast<int>(cinfo.X_density / (10 / 25.4) + 0.5);
			m_y_res = static_cast<int>(cinfo.Y_density / (10 / 25.4) + 0.5);
			break;
		}

		// Look for an ICC profile
		{
			bool invalid_profile = false;
			int num_markers = 0;
			int seq_no;
			size_t total_length;

			const int MAX_SEQ_NO = 255;			// sufficient since marker numbers are bytes
			bool     marker_present[MAX_SEQ_NO + 1];	// 1 if marker found
			size_t   data_length[MAX_SEQ_NO + 1];	// size of profile data in marker
			JOCTET* icc_data[MAX_SEQ_NO + 1];	// offset for data in marker

			/**
			 this first pass over the saved markers discovers whether there are
			 any ICC markers and verifies the consistency of the marker numbering.
			 */

			memset(marker_present, 0, (MAX_SEQ_NO + 1));

			for (struct jpeg_marker_struct* marker = cinfo.marker_list; !invalid_profile && marker; marker = marker->next)
			{
				if (marker->marker == JPEG_APP0 + 2
					&& marker->data_length >= 14
					&& memcmp(marker->data, "ICC_PROFILE", 12) == 0)
				{
					if (num_markers == 0)
					{
						// number of markers
						num_markers = GETJOCTET(marker->data[13]);
					}
					else if (num_markers != GETJOCTET(marker->data[13]))
					{
						invalid_profile = true;
						break;
					}
					// sequence number
					seq_no = GETJOCTET(marker->data[12]);
					if (seq_no <= 0 || seq_no > num_markers)
					{
						invalid_profile = true;
						break;
					}
					if (marker_present[seq_no])
					{
						invalid_profile = true;
						break;
					}
					marker_present[seq_no] = true;
					data_length[seq_no] = marker->data_length - 14;
					icc_data[seq_no] = marker->data + 14;
				}
			}

			if (num_markers == 0)
			{
				invalid_profile = true;
			}

			// Check for valid data set
			total_length = 0;
			for (seq_no = 1; !invalid_profile && seq_no <= num_markers; seq_no++)
			{
				if (marker_present[seq_no] == 0)
				{
					invalid_profile = true;
					break;
				}
				total_length += data_length[seq_no];
			}

			if (total_length <= 0)
			{
				invalid_profile = true;
			}

			// allocate space for assembled data
			if (!invalid_profile)
			{
				m_profile_data = new unsigned char[total_length];
				m_profile_size = total_length;

				// and fill it in
				total_length = 0;
				for (seq_no = 1; seq_no <= num_markers; seq_no++)
				{
					memcpy(m_profile_data + total_length, icc_data[seq_no], data_length[seq_no]);
					total_length += data_length[seq_no];
				}
			}
		}

		// Determine the image type
		switch (cinfo.out_color_space)
		{
		case JCS_GRAYSCALE:		/* monochrome */
			m_type = image_mono;
			break;
		case JCS_RGB:		/* red/green/blue */
			m_type = image_rgb;
			break;
		case JCS_CMYK:		/* C/M/Y/K */
			m_type = image_cmyk;
			break;
		case JCS_YCbCr:
		case JCS_UNKNOWN:
		case JCS_YCCK:
		default:
			throw jett_exception(JETT_INVALID_BITMAP, 0, "Unsupport JPEG type");
			break;
		}

		// Determine the stride
		m_stride = (getPixelStride() * m_width + 31) / 32 * 32;

		// Generate the image buffer
		delete[] m_data;
		m_data = new unsigned char[m_stride * m_height];

		/* read one scan line at a time */
		for (int y = 0; y < m_height; ++y)
		{
			unsigned char* p = m_data + y * m_stride;
			jpeg_read_scanlines(&cinfo, &p, 1);
			if (m_type == image_cmyk)
			{
				for (size_t i = 0; i < m_stride; ++i)
				{
					p[i] = 255 - p[i];
				}
			}
		}
		/* wrap up decompression, destroy objects, free pointers and close open files */
		jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
	}
	catch (...)
	{
		fclose(infile);
		return false;
	}

	return true;
}

void CImage::saveToFileJPEG(const TCHAR* filename)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	J_COLOR_SPACE color_space = JCS_RGB;
	switch (m_type)
	{
	case image_mono:
		color_space = JCS_GRAYSCALE;
		break;
	case image_rgb:
	case image_rgba:
	case image_bgr:
	case image_bgra:
		color_space = JCS_RGB;
		break;
	case image_cmyk:
		color_space = JCS_CMYK;
		break;
	default:
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Cannot save images of this type as a JPEG");
		break;
	}

	// Open the output file
	FILE* outfile = NULL;
	_tfopen_s(&outfile, filename, _T("wb"));
	if (!outfile)
	{
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	// here we set up the standard libjpeg error handler
	cinfo.err = jpeg_std_error(&jerr);

	// Set up our own error handlers
	jerr.error_exit = jpeg_error_exit;

	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	// Setting the parameters of the output file here
	cinfo.image_width = m_width;
	cinfo.image_height = m_height;
	cinfo.input_components = getComponents();
	cinfo.in_color_space = color_space;
	cinfo.density_unit = RESUNIT_INCH;
	cinfo.X_density = m_x_res;
	cinfo.Y_density = m_y_res;

	// default compression parameters, we shouldn't be worried about these
	jpeg_set_defaults(&cinfo);

	// Now do the compression ..
	jpeg_start_compress(&cinfo, TRUE);

	// Do we have an ICC profile?
	if (m_profile_data && m_profile_size)
	{
		int seq_no = 1;
		size_t marker_size = 0xffff - 8;
		size_t header_size = 14;
		size_t max_profile_bytes = marker_size - header_size;
		size_t data_left = m_profile_size;
		int num_markers = static_cast<int>((m_profile_size + max_profile_bytes - 1) / max_profile_bytes);
		unsigned char* profile_data = m_profile_data;
		unsigned char* marker = new unsigned char[marker_size];
		memcpy(marker, "ICC_PROFILE", 12);
		while (data_left > 0)
		{
			size_t data_in_this_marker = std::min(max_profile_bytes, data_left);
			marker[12] = seq_no;
			marker[13] = num_markers;
			memcpy(marker + 14, profile_data, data_in_this_marker);

			jpeg_write_marker(&cinfo, JPEG_APP0 + 2, marker, static_cast<int>(data_in_this_marker + header_size));

			++seq_no;
			data_left -= data_in_this_marker;
			profile_data += data_in_this_marker;
		}

		delete[] marker;
	}

	// Get the data
	unsigned char* data = lockData(true);

	if (isBGR())
	{
		// If this is BGR, then we must invert the numbers
		// before submitting to the write_scanline function
		size_t pixel_stride = getPixelStride();
		unsigned char* bgr_buffer = new unsigned char[m_stride];
		while (cinfo.next_scanline < cinfo.image_height)
		{
			unsigned char* p = data + cinfo.next_scanline * m_stride;
			size_t j = 0;
			for (size_t i = 0; i < m_stride; i += pixel_stride)
			{
				bgr_buffer[j] = p[i + 2];
				bgr_buffer[j + 1] = p[i + 1];
				bgr_buffer[j + 2] = p[i + 0];

				// We always skip out the A channel
				j += 3;
			}
			jpeg_write_scanlines(&cinfo, &bgr_buffer, 1);
		}
		delete[] bgr_buffer;
	}
	else if (m_type == image_rgba)
	{
		// If this is RGBA, then we must ignore the A channel
		size_t pixel_stride = getPixelStride();
		unsigned char* bgr_buffer = new unsigned char[m_stride];
		while (cinfo.next_scanline < cinfo.image_height)
		{
			unsigned char* p = data + cinfo.next_scanline * m_stride;
			size_t j = 0;
			for (size_t i = 0; i < m_stride; i += pixel_stride)
			{
				bgr_buffer[j] = p[i];
				bgr_buffer[j + 1] = p[i + 1];
				bgr_buffer[j + 2] = p[i + 2];

				// We always skip out the A channel
				j += 3;
			}
			jpeg_write_scanlines(&cinfo, &bgr_buffer, 1);
		}
		delete[] bgr_buffer;
	}
	else if (m_type == image_cmyk)
	{
		// If this is CMYK, then we must invert the numbers
		// before submitting to the write_scanline function
		unsigned char* cmyk_buffer = new unsigned char[m_stride];
		while (cinfo.next_scanline < cinfo.image_height)
		{
			unsigned char* p = data + cinfo.next_scanline * m_stride;
			for (size_t i = 0; i < m_stride; ++i)
			{
				cmyk_buffer[i] = 255 - p[i];
			}
			jpeg_write_scanlines(&cinfo, &cmyk_buffer, 1);
		}
		delete[] cmyk_buffer;
	}
	else
	{
		while (cinfo.next_scanline < cinfo.image_height)
		{
			unsigned char* p = data + cinfo.next_scanline * m_stride;
			jpeg_write_scanlines(&cinfo, &p, 1);
		}
	}

	unlockData();

	/* similar to read file, clean up after we're done compressing */
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(outfile);
}

static bool SafeFread(char* buffer, int size, int number, FILE* fp)
{
	int ItemsRead;
	if (feof(fp))
	{
		return false;
	}

	ItemsRead = (int)fread(buffer, size, number, fp);
	return ItemsRead == number;
}

// Create this image from a file
bool CImage::loadFromFileBMP(const TCHAR* filename)
{
	FILE* fp = NULL;
	_tfopen_s(&fp, filename, _T("rb"));
	if (fp == NULL)
	{
		// File not found!
		return false;
	}

	// read the file header

	BMFH bmfh;
	bool NotCorrupted = true;

	NotCorrupted &= SafeFread((char*)&(bmfh.bfType), sizeof(uint16_t), 1, fp);

	// Is this a bitmap?
	if (bmfh.bfType != 19778)
	{
		fclose(fp);
		return false;
	}

	NotCorrupted &= SafeFread((char*)&(bmfh.bfSize), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmfh.bfReserved1), sizeof(uint16_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmfh.bfReserved2), sizeof(uint16_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmfh.bfOffBits), sizeof(uint32_t), 1, fp);

	// read the info header
	BMIH bmih;
	NotCorrupted &= SafeFread((char*)&(bmih.biSize), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biWidth), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biHeight), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biPlanes), sizeof(uint16_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biBitCount), sizeof(uint16_t), 1, fp);

	NotCorrupted &= SafeFread((char*)&(bmih.biCompression), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biSizeImage), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biXPelsPerMeter), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biYPelsPerMeter), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biClrUsed), sizeof(uint32_t), 1, fp);
	NotCorrupted &= SafeFread((char*)&(bmih.biClrImportant), sizeof(uint32_t), 1, fp);

	if (!NotCorrupted)
	{
		// This is not a bitmap file
		fclose(fp);
		return false;
	}

	m_x_res = static_cast<int>(bmih.biXPelsPerMeter / (1000 / 25.4) + 0.5);
	m_y_res = static_cast<int>(bmih.biYPelsPerMeter / (1000 / 25.4) + 0.5);

	// Is this resolution valid?
	if (m_x_res == 0 || m_y_res == 0)
	{
		m_x_res = 72;
		m_y_res = 72;
	}

	// if bmih.biCompression 1 or 2, then the file is RLE compressed
	if (bmih.biCompression == 1 || bmih.biCompression == 2 || bmih.biCompression > 3)
	{
		fclose(fp);
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Unsupport BMP type");
	}

	if (bmih.biCompression == 3 && bmih.biBitCount != 16)
	{
		fclose(fp);
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Unsupported 16bpp BMP type");
	}

	// set the size
	if ((int)bmih.biWidth <= 0)
	{
		fclose(fp);
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Invalid size for BMP image");
	}

	// if < 16 bits, read the palette
	RGBApixel Colors[256];
	memset(Colors, 0, sizeof(Colors));
	bool mono_image = false;
	if (bmih.biBitCount < 16)
	{
		mono_image = true;

		// determine the number of colors specified in the
		// color table
		int NumberOfColorsToRead = std::min(256, ((int)bmfh.bfOffBits - 54) / 4);

		for (int n = 0; n < NumberOfColorsToRead; n++)
		{
			SafeFread((char*)&(Colors[n]), 4, 1, fp);
			mono_image &= Colors[n].Red == Colors[n].Green
				&& Colors[n].Red == Colors[n].Blue;
		}
	}

	switch (bmih.biBitCount)
	{
	case 1:
	case 2:
	case 4:
	case 8:
		if (mono_image)
		{
			createImage(bmih.biWidth, abs(bmih.biHeight), image_mono, false, m_mode);
		}
		else
		{
			createImage(bmih.biWidth, abs(bmih.biHeight), image_bgra, false, m_mode);
		}
		break;
	case 24:
		createImage(bmih.biWidth, abs(bmih.biHeight), image_bgr, false, m_mode);
		break;
	case 32:
		createImage(bmih.biWidth, abs(bmih.biHeight), image_bgra, false, m_mode);
		break;
	default:
		// Note: we don't support 16bpp BMP files as they are silly
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Invalid BPP for BMP image");
	}

	// skip blank data if bfOffBits so indicates
	fseek(fp, bmfh.bfOffBits, SEEK_SET);

	// 8, 24 & 32 bpp files can just be read directly in
	if (getPixelStride() * 8 == bmih.biBitCount && bmih.biHeight <= 0)
	{
		// This code reads 8, 24 or 32 bpp data directly where the image size and desintation
		// size are the same
		fread(m_data, 1, m_height * m_stride, fp);
	}
	else if (getPixelStride() * 8 == bmih.biBitCount && bmih.biHeight >= 0)
	{
		// positive height indicates that the image is inverted
		for (int y = m_height - 1; y >= 0 && !feof(fp); --y)
		{
			unsigned char* p = m_data + y * m_stride;
			fread(p, 1, m_stride, fp);
		}
	}
	else
	{
		// Use the palette to read
		int mask = (1 << bmih.biBitCount) - 1;
		int pixels_per_byte = 8 / bmih.biBitCount;
		int bytes_per_row = (m_width * bmih.biBitCount + 7) / 8;
		bytes_per_row = (bytes_per_row + 3) / 4 * 4;
		unsigned char* buffer = new unsigned char[bytes_per_row];

		if (mono_image)
		{
			for (int y = 0; y < m_height && !feof(fp); ++y)
			{
				unsigned char* p;
				if (bmih.biHeight > 0)
				{
					p = m_data + (m_height - y - 1) * m_stride;
				}
				else
				{
					p = m_data + y * m_stride;
				}
				fread(buffer, 1, bytes_per_row, fp);

				// Convert...
				for (int x = 0; x < bytes_per_row; ++x)
				{
					int shift = 8 - bmih.biBitCount;
					for (int px = 0; px < pixels_per_byte; ++px)
					{
						int b = (buffer[x] >> shift) & mask;
						*p = Colors[b].Red;
						++p;
						b -= bmih.biBitCount;
					}
				}
			}
		}
		else
		{
			// RGBA image
			for (int y = 0; y < m_height && !feof(fp); ++y)
			{
				unsigned char* p;
				if (bmih.biHeight > 0)
				{
					p = m_data + (m_height - y - 1) * m_stride;
				}
				else
				{
					p = m_data + y * m_stride;
				}
				fread(buffer, 1, bytes_per_row, fp);

				// Convert...
				for (int x = 0; x < bytes_per_row; ++x)
				{
					int shift = 8 - bmih.biBitCount;
					for (int px = 0; px < pixels_per_byte; ++px)
					{
						int b = buffer[x] >> shift;
						*p = Colors[b].Blue;
						++p;
						*p = Colors[b].Green;
						++p;
						*p = Colors[b].Red;
						++p;
						*p = Colors[b].Alpha;
						++p;
						b -= bmih.biBitCount;
					}
				}
			}
		}

		delete[] buffer;
	}

	fclose(fp);

	return true;
}

void CImage::saveToFileBMP(const TCHAR* filename)
{
	int BitDepth = 8;
	switch (m_type)
	{
	case image_mono1:
		BitDepth = 1;
		break;
	case image_mono2:
		BitDepth = 2;
		break;
	case image_mono4:
		BitDepth = 4;
		break;
	case image_mono:
		BitDepth = 8;
		break;
	case image_rgb:
	case image_bgr:
		BitDepth = 24;
		break;
	case image_rgba:
	case image_bgra:
		BitDepth = 32;
		break;
	case image_cmyk:
	case image_lab:
	default:
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Cannot save images of this type as a BMP");
		break;
	}

	// Open the image file
	FILE* fp = NULL;
	_tfopen_s(&fp, filename, _T("wb"));
	if (!fp)
	{
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	// some preliminaries
	int dTotalPixelBytes = static_cast<int>(m_height * m_stride);

	int dPaletteSize = 0;
	if (BitDepth <= 8)
	{
		dPaletteSize = static_cast<int>(IntPow(2, BitDepth) * 4.0);
	}

	int dTotalFileSize = 14 + 40 + dPaletteSize + dTotalPixelBytes;

	// write the file header
	BMFH bmfh;
	bmfh.bfType = 19778;
	bmfh.bfSize = (uint32_t)dTotalFileSize;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = (uint32_t)(14 + 40 + dPaletteSize);

	fwrite((char*)&(bmfh.bfType), sizeof(uint16_t), 1, fp);
	fwrite((char*)&(bmfh.bfSize), sizeof(uint32_t), 1, fp);
	fwrite((char*)&(bmfh.bfReserved1), sizeof(uint16_t), 1, fp);
	fwrite((char*)&(bmfh.bfReserved2), sizeof(uint16_t), 1, fp);
	fwrite((char*)&(bmfh.bfOffBits), sizeof(uint32_t), 1, fp);

	// write the info header
	BMIH bmih;
	bmih.biSize = 40;
	bmih.biWidth = m_width;
	bmih.biHeight = m_height;
	bmih.biPlanes = 1;
	bmih.biBitCount = BitDepth;
	bmih.biCompression = 0;
	bmih.biSizeImage = (uint32_t)dTotalPixelBytes;
	bmih.biXPelsPerMeter = static_cast<int>(m_y_res * (1000 / 25.4) + 0.5);
	bmih.biYPelsPerMeter = static_cast<int>(m_x_res * (1000 / 25.4) + 0.5);
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;

	fwrite(&(bmih.biSize), sizeof(uint32_t), 1, fp);
	fwrite(&(bmih.biWidth), sizeof(uint32_t), 1, fp);
	fwrite(&(bmih.biHeight), sizeof(uint32_t), 1, fp);
	fwrite(&(bmih.biPlanes), sizeof(uint16_t), 1, fp);
	fwrite(&(bmih.biBitCount), sizeof(uint16_t), 1, fp);
	fwrite(&(bmih.biCompression), sizeof(uint32_t), 1, fp);
	fwrite(&(bmih.biSizeImage), sizeof(uint32_t), 1, fp);
	fwrite(&(bmih.biXPelsPerMeter), sizeof(uint32_t), 1, fp);
	fwrite(&(bmih.biYPelsPerMeter), sizeof(uint32_t), 1, fp);
	fwrite(&(bmih.biClrUsed), sizeof(uint32_t), 1, fp);
	fwrite(&(bmih.biClrImportant), sizeof(uint32_t), 1, fp);

	// write the palette
	if (BitDepth <= 8)
	{
		// Write a monochrome colour table
		int NumberOfColors = IntPow(2, BitDepth);
		for (int n = 0; n < NumberOfColors; n++)
		{
			int l = (n * 255) / (NumberOfColors - 1);
			uint32_t col = l << 16 | l << 8 | l;
			fwrite(&col, 4, 1, fp);
		}
	}

	// Get the data
	unsigned char* data = lockData(true);

	// write the pixels
	if (BitDepth > 8 && !isBGR())
	{
		unsigned char* rgb_buffer = new unsigned char[m_stride];
		size_t pixel_stride = getPixelStride();

		for (int y = m_height - 1; y >= 0; --y)
		{
			unsigned char* line = data + (y * m_stride);

			if (BitDepth == 24)
			{
				for (size_t x = 0; x < m_stride; x += pixel_stride)
				{
					// Convert RGB -> BGR
					rgb_buffer[x] = line[x + 2];
					rgb_buffer[x + 1] = line[x + 1];
					rgb_buffer[x + 2] = line[x + 0];
				}
			}
			else
			{
				for (size_t x = 0; x < m_stride; x += pixel_stride)
				{
					// Convert RGBA -> BGRA
					rgb_buffer[x] = line[x + 2];
					rgb_buffer[x + 1] = line[x + 1];
					rgb_buffer[x + 2] = line[x + 0];
					rgb_buffer[x + 3] = line[x + 3];
				}
			}
			fwrite(rgb_buffer, getStride(), 1, fp);
		}

		delete[] rgb_buffer;
	}
	else
	{
		for (int y = m_height - 1; y >= 0; --y)
		{
			unsigned char* line = data + (y * m_stride);
			fwrite(line, getStride(), 1, fp);
		}
	}

	unlockData();

	// All done
	fclose(fp);
}

bool CImage::loadFromFilePNG(const TCHAR* filename)
{
	png_byte header[8];    // 8 is the maximum size that can be checked
	bool is_sixteen_bpp = false;

	/* open file and test for it being a png */
	FILE* fp = NULL;
	_tfopen_s(&fp, filename, _T("rb"));
	if (!fp)
	{
		// Can't open!
		return false;
	}
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
	{
		// Not a PNG file
		fclose(fp);
		return false;
	}

	/* initialize stuff */
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
	{
		// Invalid PNG file
		fclose(fp);
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Invalid PNG image");
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		// Invalid PNG file
		fclose(fp);
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Invalid PNG image");
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		// Invalid PNG file
		fclose(fp);
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Invalid PNG image");
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	// Read the image size
	m_width = png_get_image_width(png_ptr, info_ptr);
	m_height = png_get_image_height(png_ptr, info_ptr);

	// Set the clipping rectangle
	m_clip_x1 = 0;
	m_clip_y1 = 0;
	m_clip_width = m_width;
	m_clip_height = m_height;

	int color_type = png_get_color_type(png_ptr, info_ptr);
	int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	is_sixteen_bpp = (bit_depth == 16);

	if (is_sixteen_bpp)
	{
#if defined(_WIN32) || defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
		png_set_swap(png_ptr);
#endif
	}

	// Force the image in to the format we
	// want (either RGB, RGBA or Monochrome)
	if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_expand(png_ptr);
	}
	else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
	{
		png_set_expand(png_ptr);
	}
	else if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	{
		png_set_expand(png_ptr);
	}
	else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(png_ptr);
	}

	/*
	double gamma;
	if (png_get_gAMA(png_ptr, info_ptr, &gamma))
	{
		png_set_gamma(png_ptr, 2.2, gamma);
	}
	*/

	png_uint_32 res_x = 0;
	png_uint_32 res_y = 0;
	int unit_type = PNG_RESOLUTION_UNKNOWN;
	if (png_get_pHYs(png_ptr, info_ptr, &res_x, &res_y, &unit_type) && unit_type == PNG_RESOLUTION_METER)
	{
		m_x_res = static_cast<int>(res_x / (1000 / 25.4) + 0.5);
		m_y_res = static_cast<int>(res_y / (1000 / 25.4) + 0.5);
	}
	else
	{
		m_x_res = 72;
		m_y_res = 72;
	}
	png_read_update_info(png_ptr, info_ptr);

	m_stride = png_get_rowbytes(png_ptr, info_ptr);
	int channels = (int)png_get_channels(png_ptr, info_ptr);

	switch (channels)
	{
	case 1:
		m_type = image_mono;
		break;
	case 3:
		m_type = image_rgb;
		break;
	case 4:
		m_type = image_rgba;
		break;
	default:
		png_read_end(png_ptr, NULL);
		fclose(fp);
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Invalid PNG image");
	}

	m_16bpp = is_sixteen_bpp;

	// Is there an ICC profile
	png_charp name = NULL;
	int compression_type = 0;
	png_bytep profile_data = NULL;
	png_uint_32 profile_size = 0;
	png_get_iCCP(png_ptr, info_ptr, &name, &compression_type, &profile_data, &profile_size);
	if (profile_data && profile_size)
	{
		// Make a copy of the data
		set_profile_data(profile_data, profile_size);
	}

	// Allocate memory
	m_data = new unsigned char[m_height * m_stride];

	png_bytep* row_pointers = new png_bytep[m_height];
	for (int i = 0; i < m_height; ++i)
	{
		row_pointers[i] = (png_bytep)(m_data + i * m_stride);
	}

	png_read_image(png_ptr, row_pointers);

	delete[] row_pointers;

	png_read_end(png_ptr, NULL);
	fclose(fp);

	return true;
}

void CImage::saveToFilePNG(const TCHAR* filename)
{
	int png_color_type = 0;
	switch (m_type)
	{
	case image_mono:
		png_color_type = PNG_COLOR_TYPE_GRAY;
		break;
	case image_rgb:
		png_color_type = PNG_COLOR_TYPE_RGB;
		break;
	case image_bgr:
		png_color_type = PNG_COLOR_TYPE_RGB;
		break;
	case image_rgba:
		png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
		break;
	case image_bgra:
		png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
		break;

	case image_cmyk:
	case image_lab:
	default:
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Cannot save images of this type as a PNG");
		break;
	}

	// Open the image file
	FILE* outfile = NULL;
	_tfopen_s(&outfile, filename, _T("wb"));
	if (!outfile)
	{
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_byte** row_pointers = NULL;

	// Initialize the write struct
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		fclose(outfile);
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	// Initialize the info struct
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(outfile);
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	// Set up error handling
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(outfile);
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	// Set image attributes
	png_set_IHDR(png_ptr,
		info_ptr,
		m_width,
		m_height,
		m_16bpp ? 16 : 8,
		png_color_type,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	// Take into account BGR images
	if (isBGR())
	{
		png_set_bgr(png_ptr);
	}

	if (m_16bpp)
	{
#if defined(_WIN32) || defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
		png_set_swap(png_ptr);
#endif
	}

	if (m_profile_data && m_profile_size > 0)
	{
		png_set_iCCP(png_ptr, info_ptr, "profile", 0, m_profile_data, m_profile_size);
	}

	// Get the data
	unsigned char* data = lockData(true);

	/// Initialize rows of PNG
	row_pointers = static_cast<png_byte**>(png_malloc(png_ptr, m_height * sizeof(png_byte*)));
	for (int y = 0; y < m_height; ++y)
	{
		row_pointers[y] = data + y * getStride();
	}

	/// Actually write the image data
	png_init_io(png_ptr, outfile);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	unlockData();

	// Cleanup
	png_free(png_ptr, row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(outfile);
}

size_t CImage::getPixelStride() const
{
	switch (m_type)
	{
	case image_mono:
		return 1;
	case image_rgb:
	case image_bgr:
		return 3;
	case image_rgba:
	case image_bgra:
		return 4;
	case image_cmyk:
		return 4;
	case image_lab:
		return 3;
	case image_invalid:
	default:
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Invalid image type");
	}

	return 1;
}

int CImage::getComponents() const
{
	switch (m_type)
	{
	case image_mono:
		return 1;
	case image_rgb:
	case image_rgba:
	case image_bgr:
	case image_bgra:
		return 3;
	case image_cmyk:
		return 4;
	case image_lab:
		return 3;
	case image_invalid:
	default:
		throw jett_exception(JETT_INVALID_BITMAP, 0, "Invalid image type");
	}

	return 1;
}

// Save this image to file
void CImage::saveToFileTIFF(const TCHAR* filename)
{
#ifdef UNICODE
	TIFF* dif = TIFFOpenW(filename, "w");
#else
	TIFF* dif = TIFFOpen(filename, "w");
#endif

	if (!dif)
	{
		throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unable to open image file for writing");
	}

	// Download the image data
	unsigned char* data = lockData(true);

	try
	{
		short bps = m_16bpp ? 16 : 8;
		TIFFSetField(dif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(dif, TIFFTAG_IMAGELENGTH, m_height);
		TIFFSetField(dif, TIFFTAG_IMAGEWIDTH, m_width);
		TIFFSetField(dif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(dif, TIFFTAG_BITSPERSAMPLE, bps);

		// Do we embed a profile?
		// (only if not L*a*b* - there is no point in saving the L*a*b* profile)
		if (m_type != image_lab && m_profile_data && m_profile_size)
		{
			uint32_t EmbedLen = static_cast<uint32_t>(m_profile_size);
			uint8_t* EmbedBuffer = m_profile_data;
			TIFFSetField(dif, TIFFTAG_ICCPROFILE, EmbedLen, EmbedBuffer);
		}

		switch (m_type)
		{
		case image_mono:
			TIFFSetField(dif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
			break;
		case image_rgb:
		case image_bgr:
			TIFFSetField(dif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
			break;
		case image_rgba:
		case image_bgra:
			TIFFSetField(dif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
			break;
		case image_cmyk:
			TIFFSetField(dif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED);
			break;
		case image_lab:
			TIFFSetField(dif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_CIELAB);
			break;
		default:
			throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Cannot save images of this type as a TIFF");
			break;
		}

		float xres = static_cast<float>(m_x_res);
		float yres = static_cast<float>(m_y_res);
		TIFFSetField(dif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
		TIFFSetField(dif, TIFFTAG_XRESOLUTION, xres);
		TIFFSetField(dif, TIFFTAG_XRESOLUTION, yres);

		TIFFSetField(dif, TIFFTAG_SAMPLESPERPIXEL, getPixelStride());
		TIFFSetField(dif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

		switch (m_type)
		{
		case image_bgra:
		{
			size_t pixel_stride = getPixelStride();
			if (m_16bpp)
			{
				unsigned short* rgba_buffer = new unsigned short[m_stride];
				for (int y = 0; y < m_height; y++)
				{
					unsigned short* line = (unsigned short*)(data + (y * m_stride));
					for (size_t x = 0; x < m_stride; x += pixel_stride)
					{
						// Convert BGRA -> RGBA
						rgba_buffer[x] = line[x + 2];
						rgba_buffer[x + 1] = line[x + 1];
						rgba_buffer[x + 2] = line[x + 0];
						rgba_buffer[x + 3] = line[x + 3];
					}
				}
				delete[] rgba_buffer;
			}
			else
			{
				unsigned char* rgba_buffer = new unsigned char[m_stride];
				for (int y = 0; y < m_height; y++)
				{
					unsigned char* line = data + (y * m_stride);
					for (size_t x = 0; x < m_stride; x += pixel_stride)
					{
						// Convert BGRA -> RGBA
						rgba_buffer[x] = line[x + 2];
						rgba_buffer[x + 1] = line[x + 1];
						rgba_buffer[x + 2] = line[x + 0];
						rgba_buffer[x + 3] = line[x + 3];
					}
					TIFFWriteScanline(dif, rgba_buffer, static_cast<int>(y));
				}
				delete[] rgba_buffer;
			}
		}
		break;
		case image_bgr:
		{
			size_t pixel_stride = getPixelStride();
			if (m_16bpp)
			{
				unsigned short* rgba_buffer = new unsigned short[4 * m_width];

				for (int y = 0; y < m_height; y++)
				{
					unsigned short* line = (unsigned short*)(data + (y * m_stride));
					for (size_t x = 0; x < m_stride; x += pixel_stride)
					{
						// Convert BGR -> RGBA
						rgba_buffer[x] = line[x + 2];
						rgba_buffer[x + 1] = line[x + 1];
						rgba_buffer[x + 2] = line[x + 0];
						rgba_buffer[x + 3] = 255;
					}
					TIFFWriteScanline(dif, rgba_buffer, static_cast<int>(y));
				}
				delete[] rgba_buffer;
			}
			else
			{
				unsigned char* rgba_buffer = new unsigned char[4 * m_width];
				for (int y = 0; y < m_height; y++)
				{
					unsigned char* line = data + (y * m_stride);
					for (size_t x = 0; x < m_stride; x += pixel_stride)
					{
						// Convert BGR -> RGBA
						rgba_buffer[x] = line[x + 2];
						rgba_buffer[x + 1] = line[x + 1];
						rgba_buffer[x + 2] = line[x + 0];
						rgba_buffer[x + 3] = 255;
					}
					TIFFWriteScanline(dif, rgba_buffer, static_cast<int>(y));
				}
				delete[] rgba_buffer;
			}
		}
		break;
		default:
			for (int y = 0; y < m_height; y++)
			{
				unsigned char* line = data + (y * m_stride);
				TIFFWriteScanline(dif, line, static_cast<int>(y));
			}
			break;
		}
	}
	catch (...)
	{
		unlockData();
		TIFFClose(dif);
		throw;
	}

	unlockData();
	TIFFClose(dif);
}

// Upload (if necessary) the image to the GPU and get the
// pointer to it
cl_mem CImage::lockCLData(CGPUProcessor* cl, bool read_only)
{
	if (is_gpu_read_only() && !read_only)
	{
		// We don't have the appropriate data to do this...
		throw jett_exception(JETT_IMAGE_READ_ONLY, 0, "The image was created as a read-only object and cannot be written to");
	}

	if (cl)
	{
		m_cl = cl;
	}

	size_t bitmap_size = m_stride * m_height;

	// Create the buffer (as necessary)
	if (!m_cl_data)
	{
		if (!m_cl)
		{
			// We don't have the appropriate data to do this...
			throw jett_exception(JETT_INTERNAL_ERROR, 0, "Failed to create OpenCL buffer");
		}

		// Create the buffer
		int flags = CL_MEM_READ_WRITE;
		if (is_gpu_read_only())
		{
			flags = CL_MEM_READ_ONLY;
		}
		m_cl_data = m_cl->createBuffer(flags, bitmap_size, NULL);
		m_cpu_changed = true;
	}

	// Upload the bitmap (if it has changed or this is a new buffer)
	if (m_cpu_changed)
	{
		if (!m_data)
		{
			// We don't have the appropriate data to do this...
			throw jett_exception(JETT_INTERNAL_ERROR, 0, "Failed to create OpenCL buffer");
		}

		m_cl->enqueueWriteBuffer(m_cl_data, CL_TRUE, 0, bitmap_size, m_data);
		m_cpu_changed = false;
		m_gpu_changed = false;
	}

	m_gpu_write_pending = m_gpu_write_pending || !read_only;

	// Now discard the local copy
	if (is_cpu_discardable())
	{
		delete[] m_data;
		m_data = NULL;
	}

	return m_cl_data;
}

//
// The GPU is finished with the image
//
void CImage::unlockCLData(CGPUProcessor* cl)
{
	if (is_gpu_discardable())
	{
		m_cl->releaseMemObject(m_cl_data);
		m_cl_data = NULL;
	}
	else if (m_gpu_write_pending)
	{
		m_gpu_changed = true;
	}

	m_gpu_write_pending = false;
}

// Get pointers to the CPU version of this bitmap
// (download & create as necessary)
unsigned char* CImage::lockData(bool read_only)
{
	if (!m_data || (m_cl && m_cl_data && m_gpu_changed))
	{
		size_t bitmap_size = m_stride * m_height;
		if (!m_data)
		{
			m_data = new unsigned char[bitmap_size];
		}

		if (m_cl && m_cl_data)
		{
			m_cl->enqueueReadBuffer(m_cl_data, CL_TRUE, 0, bitmap_size, m_data);
		}

		if (m_mode == image_mode_default)
		{
			m_cl->releaseMemObject(m_cl_data);
			m_cl_data = NULL;
		}

		m_gpu_changed = false;
	}

	return m_data;
}

void CImage::unlockData()
{
	m_cpu_changed = true;
	m_gpu_changed = false;
}

// Is this a BGR or BGRA image
bool CImage::isBGR() const
{
	switch (m_type)
	{
	case image_invalid:
	case image_mono1:
	case image_mono2:
	case image_mono4:
	case image_mono:
	case image_rgb:
	case image_rgba:
	case image_cmyk:
	case image_lab:
		return false;
	case image_bgr:
	case image_bgra:
		return true;
	}

	return false;
}

// Is this a 16bpp image?
bool CImage::is16bpp() const
{
	return m_16bpp;
}

int CImage::getWidth() const
{
	return m_width;
}

int CImage::getHeight() const
{
	return m_height;
}

size_t CImage::getStride() const
{
	return m_stride;
}

image_t CImage::getType() const
{
	return m_type;
}

// Get the embedded profile data
// (returns NULL if there is none)
void* CImage::get_profile_data() const
{
	return m_profile_data;
}

size_t CImage::get_profile_size() const
{
	return m_profile_size;
}

// Set the embedded profile data
void CImage::set_profile_data(const TCHAR* filename)
{
	// Get rid of the old profile
	if (m_profile_data)
	{
		delete[] m_profile_data;
		m_profile_data = NULL;
		m_profile_size = 0;
	}

	// Is this a standard profile?
	cmsHPROFILE hProfile = lcms_transform_sampler::createStandardProfile(filename);

	if (hProfile)
	{
		// We must convert the profile to an in-memory profile
		cmsUInt32Number size;
		cmsSaveProfileToMem(hProfile, NULL, &size);
		m_profile_size = size;
		m_profile_data = new unsigned char[m_profile_size];
		cmsSaveProfileToMem(hProfile, m_profile_data, &size);

		return;
	}

	// Load the profile data from file
	FILE* infile = NULL;
	_tfopen_s(&infile, filename, _T("rb"));
	if (!infile)
	{
		throw jett_exception(JETT_FILE_NOT_FOUND, 0, "Unable to open ICC profile file");
	}

	// Get the size of the file
	fseek(infile, 0, SEEK_END);
	m_profile_size = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	// Read in the data
	m_profile_data = new unsigned char[m_profile_size];
	fread(m_profile_data, m_profile_size, 1, infile);
	fclose(infile);

	return;
}

void CImage::set_profile_data(void* data, size_t size)
{
	// Get rid of the old profile
	if (m_profile_data)
	{
		delete[] m_profile_data;
		m_profile_data = NULL;
		m_profile_size = 0;
	}

	m_profile_data = new unsigned char[size];
	m_profile_size = size;
	memcpy(m_profile_data, data, size);
}

// Get/Set the resolution of this image (in DPI)
void CImage::get_resolution(int& x, int& y) const
{
	x = m_x_res;
	y = m_y_res;
}

void CImage::set_resolution(int x, int y)
{
	m_x_res = x;
	m_y_res = y;
}

// Get/Set the clipping rectangle
void CImage::set_clip_rect(int clip_x1, int clip_y1, int clip_width, int clip_height)
{
	m_clip_x1 = clip_x1;
	m_clip_y1 = clip_y1;
	m_clip_width = clip_width;
	m_clip_height = clip_height;
}

void CImage::get_clip_rect(int& clip_x1, int& clip_y1, int& clip_width, int& clip_height) const
{
	clip_x1 = m_clip_x1;
	clip_y1 = m_clip_y1;
	clip_width = m_clip_width;
	clip_height = m_clip_height;
}

void CImage::clipping(int& clip_x1, int& clip_y1, int& clip_x2, int& clip_y2) const
{
	clip_x1 = std::max(0, m_clip_x1);
	clip_y1 = std::max(0, m_clip_y1);
	clip_x2 = std::min(static_cast<int>(m_width) - 1, m_clip_x1 + m_clip_width - 1);
	clip_y2 = std::min(static_cast<int>(m_height) - 1, m_clip_y1 + m_clip_height - 1);
}

/*

 Mode          When in GPU discard from CPU    Read-only in GPU    Discard from GPU
 image_mode_default             Yes                         No                  No
 image_mode_gpu_ro              Yes                         Yes                 No
 image_mode_cpu_only            No                          Yes                 Yes
 image_mode_gpu_copy            No                          Yes                 No

*/
bool CImage::is_gpu_read_only()
{
	return m_mode != image_mode_default;
}

bool CImage::is_cpu_discardable()
{
	return m_mode == image_mode_default || m_mode == image_mode_gpu_ro;
}

bool CImage::is_gpu_discardable()
{
	return m_mode == image_mode_cpu_only;
}

// Erase this bitmap (must be in CPU memory)
void CImage::erase()
{
	lockData(false);
	if (m_data)
	{
		switch (m_type)
		{
		case image_mono1:
		case image_mono2:
		case image_mono4:
		case image_mono:
		case image_bgr:
		case image_rgb:
		case image_lab:
			memset(m_data, 255, m_height * m_stride);
			break;
		case image_cmyk:
		case image_invalid:
			memset(m_data, 0, m_height * m_stride);
			break;
		case image_rgba:
		case image_bgra:
			memset(m_data, 255, m_height * m_stride);
			break;
		default:
			throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Unsupported image type for erase");
			break;
		}
		m_cpu_changed = true;
	}
	unlockData();
}

// Reference counting
void CImage::inc_ref()
{
	++m_refs;
}

void CImage::dec_ref()
{
	--m_refs;
	if (m_refs == 0)
	{
		delete this;
	}
}