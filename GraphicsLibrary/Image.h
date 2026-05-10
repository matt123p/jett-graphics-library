//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_Image_h
#define GraphicsLibrary_Image_h

class CGPUProcessor;
#include "jett.h"

//
// This class represents an image we
// have in memory
//
class CImage
{
private:
    // Here is the OpenCL device we are using
    CGPUProcessor*     m_cl;

    // The data associated with the image
    image_t       m_type;
    bool          m_16bpp;
    image_mode_t  m_mode;
    bool          m_cpu_changed;
    bool          m_gpu_changed;
    bool          m_gpu_write_pending;
    int			  m_width;
    int			  m_height;
    
    // Clipping rectangle
    int m_clip_x1;
    int m_clip_y1;
    int m_clip_width;
    int m_clip_height;

    // The number of bytes/line
    size_t        m_stride;

#ifdef _WIN32
	// Windows Bitmap
	HBITMAP		  m_bitmap;
#endif
    
    // The image data
    unsigned char *m_data;
    
    // The profile data associated with this file
    unsigned char *m_profile_data;
    size_t        m_profile_size;
    
    // The resolution of this image in Dots Per Inch (DPI)
    int           m_x_res;
    int           m_y_res;
    
    // The OpenCL memory block that currently
    // holds this image
    cl_mem        m_cl_data;
    
    // Here is the reference counter
    int           m_refs;
    
    bool loadFromFileTIFF( const TCHAR *filename );
    bool loadFromFileJPEG( const TCHAR *filename );
    bool loadFromFileBMP( const TCHAR *filename );
    bool loadFromFilePNG( const TCHAR *filename );
    
    void saveToFileTIFF( const TCHAR *filename );
    void saveToFileJPEG( const TCHAR *filename );
    void saveToFilePNG( const TCHAR *filename );
    void saveToFileBMP( const TCHAR *filename );

public:
    CImage();
    ~CImage();

    // Create this image from a file
    void createImage( unsigned int width, unsigned int height, image_t type, bool image_16bpp, image_mode_t mode );
#ifdef _WIN32
    HBITMAP createBitmap( unsigned int width, unsigned int height, image_t type );
#endif
    void loadFromFile( const TCHAR *filename, image_mode_t mode );
    void saveToFile( const TCHAR *filename );
    
    // Is this a BGR or BGRA image
    bool isBGR() const;
    
    // Is this a 16bpp image?
    bool is16bpp() const;

    int getWidth() const;
    int getHeight() const;
    size_t getStride() const;
    size_t getPixelStride() const;
    int getComponents() const;
    image_t getType() const;
    image_mode_t getMode() const { return m_mode; }

    // Discard this bitmap from memory
    void discard();
    
    // Make the bitmap all white (must be in CPU memory)
    void erase();
    
    // Copy data from another source
    void copy_bitmap( unsigned int width, unsigned int height, image_t type, image_mode_t mode, const unsigned char *data, size_t stride, int bpp, bool invert );
    
    // Get the pointers to the GPU version of this bitmap
    // (create & upload as necessary)
    cl_mem lockCLData( CGPUProcessor* cl, bool read_only );
    void   unlockCLData( CGPUProcessor* cl );
    
    // Get pointers to the CPU version of this bitmap
    // (download & create as necessary)
    unsigned char* lockData(bool read_only);
    void unlockData();
    
    // Get the embedded profile data
    // (returns NULL if there is none)
    void* get_profile_data() const;
    size_t get_profile_size() const;
    
    // Set the embedded profile data
    void set_profile_data( const TCHAR *filename );
    void set_profile_data( void* data, size_t size);
    
    // Get/Set the resolution of this image (in DPI)
    void get_resolution( int &x, int &y ) const;
    void set_resolution( int x, int y );
    
    // Get/Set the clipping rectangle
    void set_clip_rect( int clip_x1, int clip_y1, int clip_width, int clip_height );
    void get_clip_rect( int& clip_x1, int& clip_y1, int& clip_width, int& clip_height ) const;
    void clipping( int& clip_x1, int& clip_y1, int& clip_x2, int& y2 ) const;

    
    // Decode the mode 
    bool    is_gpu_read_only();
    bool    is_cpu_discardable();
    bool    is_gpu_discardable();

    // Reference counting
    void    inc_ref();
    void    dec_ref();
};

#endif
