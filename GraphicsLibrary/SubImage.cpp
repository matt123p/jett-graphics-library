#include "StdAfx.h"
#include "SubImage.h"
#include "Image.h"
#include "GPUProcessor.h"

CSubImage::CSubImage(CImage *parent)
{
	m_parent = parent;
	m_type = image_invalid;
	m_width = 0;
    m_height = 0;
	m_stride = 0;
	m_mode = image_mode_default;
	m_gpu_changed = false;

    m_data = NULL;
    m_cl = NULL;
    m_cl_data = NULL;

#ifndef __MACH__
	m_bitmap = NULL;
#endif
}


CSubImage::~CSubImage(void)
{
	discard();
}

// Create this sub-image of the appropriate dimensions
void CSubImage::create( int start_y, int height )
{
	// First we get rid of any previous image data
	discard();

	// Copy the attributes for easy access
	m_width = m_parent->getWidth();
	m_type = m_parent->getType();
    m_mode = m_parent->getMode();
    m_start_y = start_y;
	m_height = height;

    switch (m_type)
	{
        case image_mono1:
            m_stride = (m_width/8 + 3) / 4 * 4;
            break;
        case image_mono2:
            m_stride = (m_width/4 + 3) / 4 * 4;
            break;
        case image_mono4:
            m_stride = (m_width/2 + 3) / 4 * 4;
            break;
        default:
            m_stride = (getPixelStride() * m_width + 3) / 4 * 4;
            break;
	}

    
	// We don't need to allocate memory here, we
	// do that the first time the sub-image is locked
}

#ifndef __MACH__
static int IntPow( int base, int exponent )
{
    int i;
    int output = 1;
    for( i=0 ; i < exponent ; i++ )
    { output *= base; }
    return output;
}

HBITMAP CSubImage::createBitmap( unsigned int width, unsigned int height, image_t type )
{
	// Make sure it is compatible with a windows Bitmap
	switch (type)
	{
	case image_rgb:
	case image_rgba:
	case image_cmyk:
    case image_lab:
		throw jett_exception(JETT_INVALID_ARGUMENT, 0,"The bitmap type is not compatible with a Windows HBITMAP");
		break;
	}

	// First create the bitmap
    discard();
    delete [] m_data;
    m_data = NULL;
    
    // Now setup the new data
    m_width = width;
    m_height = height;
    m_type = type;
	int bpp = 8;

	switch (type)
	{
	case image_mono1:
	    m_stride = (m_width/8 + 3) / 4 * 4;
		bpp = 1;
		break;
	case image_mono2:
	    m_stride = (m_width/4 + 3) / 4 * 4;
		bpp = 2;
		break;
	case image_mono4:
	    m_stride = (m_width/2 + 3) / 4 * 4;
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
		ZeroMemory( &b, sizeof( b ) );
		b.bi_dst.biSize = sizeof( b.bi_dst );
		b.bi_dst.biWidth = m_width;
		b.bi_dst.biHeight = m_height;
		b.bi_dst.biPlanes = 1;
		b.bi_dst.biBitCount = bpp;
		b.bi_dst.biCompression = BI_RGB;

		if( bpp <= 8 )
		{
			// Write a monochrome colour table
			int NumberOfColors = IntPow(2,bpp);
			for( int n=0 ; n < NumberOfColors ; n++ )
			{
				int l = (n * 255) / (NumberOfColors-1);
				b.palette[ n ].rgbRed = l;
				b.palette[ n ].rgbGreen = l;
				b.palette[ n ].rgbBlue = l;
			}
		}

 
		PVOID bits = NULL;
		m_bitmap = CreateDIBSection( NULL, (BITMAPINFO*)&b.bi_dst, DIB_RGB_COLORS, &bits, NULL, 0 );
		m_data = (unsigned char *)bits;
	}

    m_mode = image_mode_gpu_ro;
    m_cpu_changed = true;

	return m_bitmap;
}
#endif



// Discard this bitmap from memory
void CSubImage::discard()
{
#ifndef __MACH__
	if (m_bitmap)
	{
		// Delete the bitmap
		DeleteObject( m_bitmap );
        m_bitmap = NULL;
	}
	else
	{
        delete [] m_data;
	}
#else
    delete [] m_data;
#endif
    
	m_data = NULL;


    if (m_cl_data && m_cl)
    {
        m_cl->releaseMemObject(m_cl_data);
    }
    
    m_cl_data = NULL;
    m_cpu_changed = false;
	m_gpu_changed = false;

	m_type = image_invalid;
	m_width = 0;
    m_height = 0;
	m_stride = 0;
	m_mode = image_mode_default;
}


// Is this a BGR or BGRA image
bool CSubImage::isBGR() const
{
    return m_type == image_bgr || m_type == image_bgra;
}

int CSubImage::getWidth() const
{
    return m_width;
}

int CSubImage::getHeight() const
{
    return m_height;
}

int CSubImage::getStartY() const
{
    return m_start_y;
}

size_t CSubImage::getStride() const
{
    return m_stride;
}

size_t CSubImage::getPixelStride() const
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

int CSubImage::getComponents() const
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

image_t CSubImage::getType() const
{
    return m_type;
}


/*
 
 Mode          When in GPU discard from CPU    Read-only in GPU    Discard from GPU
 image_mode_default             Yes                         No                  No
 image_mode_gpu_ro              Yes                         Yes                 No
 image_mode_cpu_only            No                          Yes                 Yes
 image_mode_gpu_copy            No                          Yes                 No
 
 
*/
bool CSubImage::is_gpu_read_only()
{
    return m_mode != image_mode_default;
}

bool CSubImage::is_cpu_discardable()
{
    return m_mode == image_mode_default || m_mode == image_mode_gpu_ro;
}

bool CSubImage::is_gpu_discardable()
{
    return m_mode == image_mode_cpu_only;
}


// Upload (if necessary) the image to the GPU and get the
// pointer to it
_locked_ptr<CSubImage,cl_mem> CSubImage::lockCLData( CGPUProcessor* cl, bool read_only )
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
        if ( is_gpu_read_only() )
        {
            flags = CL_MEM_READ_ONLY;
        }
        m_cl_data = m_cl->createBuffer(flags,  bitmap_size, NULL );
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
    }
    
    // Now discard the local copy
    if (is_cpu_discardable())
    {
        delete [] m_data;
        m_data = NULL;
    }
    
    return _locked_ptr<CSubImage,cl_mem>(this,m_cl_data);
}


//
// The GPU is finished with the image
//
void CSubImage::unlockData(cl_mem&)
{
    if (is_gpu_discardable())
    {
        m_cl->releaseMemObject(m_cl_data);
        m_cl_data = NULL;
    }
	else
	{
		m_gpu_changed = true;
	}
}

// Get pointers to the CPU version of this bitmap
// (download & create as necessary)
_locked_ptr<CSubImage,unsigned char*> CSubImage::lockData( bool read_only )
{
	size_t bitmap_size = m_stride * m_height;

	// Do we need to create the image data here?
    if (!m_data)
    {
        m_data = new unsigned char[ bitmap_size ];
	}

	// Do we need to download the image data from the GPU?
	if (m_cl && m_cl_data && m_gpu_changed)
	{
        m_cl->enqueueReadBuffer( m_cl_data, CL_TRUE, 0, bitmap_size, m_data );
        
        if (m_mode == image_mode_default)
        {
            m_cl->releaseMemObject(m_cl_data);
            m_cl_data = NULL;
        }

		m_gpu_changed = false;
    }

    return _locked_ptr<CSubImage,unsigned char*>(this,m_data);
}

void CSubImage::unlockData(unsigned char *)
{
    m_cpu_changed = true;
}

void CSubImage::copy_bitmap( unsigned int width, unsigned int height, const unsigned char *data, size_t stride, int bpp, bool invert )
{
    lockData( false );
    
    // Copy row at a time to avoid differences in stride
    size_t copy_stride = std::min(m_stride, stride);
    for (int y = 0; y < m_height; ++ y)
    {
        const unsigned char *src = data + y * stride;
        unsigned char *dst = m_data + y * m_stride;
        
        switch (bpp)
        {
            case 1:
            {
                int b = 7;
                for (int x = 0; x < m_width; ++x)
                {
                    bool c = ((*src) >> b) & 1;
                    if (invert==c)
                    {
                        *dst = 0;
                    }
                    else
                    {
                        *dst = 255;
                    }
                    
                    --b;
                    if (b==-1)
                    {
                        ++ src;
                        b = 7;
                    }
                    ++ dst;
                }
            }
                break;
            case 8:
                if (invert)
                {
                    for (int x = 0; x < m_width; ++x)
                    {
                        *dst = 255 - *src;
                        ++ src;
                        ++ dst;
                    }
                }
                else
                {
                    memcpy( dst, src, copy_stride );
                }
                break;
        }
    }
    
    
    unlockData((unsigned char*)NULL);
}

// Apply the parent image's clipping rectangle
void CSubImage::clipping( int& clip_x1, int& clip_y1, int& clip_x2, int& clip_y2, int &y_correct ) const
{
    y_correct = m_start_y;
    m_parent->clipping( clip_x1, clip_y1, clip_x2, clip_y2 );
}


