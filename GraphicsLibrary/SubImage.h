//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_SubImage_h
#define GraphicsLibrary_SubImage_h

#include "jett.h"

class CGPUProcessor;

// This template class is used to lock a resource from a parent object.
// When the class goes out of scope the resource is automatically unlocked
// in the parent.
//
template<typename _Tparent, typename _Tdata> class _locked_ptr
{
public:
    _Tparent*   m_parent;
    _Tdata      m_data;
    
    _locked_ptr(_Tparent* parent, _Tdata data)
    {
        m_parent = parent;
        m_data = data;
    }
};

template<typename _Tparent, typename _Tdata> class locking_ptr
{
private:
    _Tparent*   m_parent;
    _Tdata      m_data;
public:
    
    /// Construct and get a lock
    locking_ptr()
    {
        m_parent = NULL;
        m_data = NULL;
    }
    
    locking_ptr( _locked_ptr<_Tparent,_Tdata> d)
    {
        m_parent = d.m_parent;
        m_data = d.m_data;
    }
    
    locking_ptr(const locking_ptr& b)
    {
        *this = b;
    }
    
    locking_ptr& operator=(const locking_ptr& b)
    {
        m_parent = b.m_parent;
        m_data = b.m_data;
        
        return *this;
    }

    
    /// Destruction and unlock
    ~locking_ptr()
    {
        m_parent->unlockData( m_data );
    }
    
    _Tdata& operator*() const
    {
        return *m_data;
    }

    _Tdata* operator&()
    {
        return &m_data;
    }

    
    _Tdata operator->() const
    {
        return m_data;
    }
    
    _Tdata get() const
    {
        return m_data;
    }
    

    
};


//
// Images are composed of one or more sub-images.
// This is because very large images cause memory fragmentation (particulary on the GPU) and sometimes
// will not fit entirely in to RAM.
// Sub-images also enable an easier way to start the processing of an image on the GPU before it has
// finished uploading to the GPU's RAM, and hence overlapping upload and processing.
// Similary, it enables the image to start being worked on before it has been completely, de-serialised
// from hard-disk.
//

class CSubImage
{
private:
    // Here is the OpenCL device we are using
    CGPUProcessor*     m_cl;

	// Here is the parent image that we are
	// part of
	CImage*		  m_parent;
	
#ifdef _WIN32
	// Windows Bitmap
	HBITMAP		  m_bitmap;
#endif


	// The data associated with the image
    image_t       m_type;
    image_mode_t  m_mode;
    bool          m_cpu_changed;
	bool		  m_gpu_changed;
    int           m_start_y;
    int			  m_width;
    int			  m_height;

	// The number of bytes/line
    size_t        m_stride;

    // The image data
    unsigned char *m_data;

    // The OpenCL memory block that currently
    // holds this image
    cl_mem        m_cl_data;
    
    // Here is the reference counter
    int           m_refs;

	// Discard this image from memory
	void discard();

public:
	CSubImage(CImage *parent);
	~CSubImage(void);

	// Create this sub-image of the appropriate dimensions
	void create( int start_y, int height );

	// Create this bitmap compatible with a Windows HBITMAP
#ifdef _WIN32
    HBITMAP createBitmap( unsigned int width, unsigned int height, image_t type );
#endif


    // Is this a BGR or BGRA image?
    bool isBGR() const;

	// Get the data about this sub-image
    int getWidth() const;
    int getHeight() const;
    int getStartY() const;
    size_t getStride() const;
    size_t getPixelStride() const;
    int getComponents() const;
    image_t getType() const;

    // Decode the mode 
    bool    is_gpu_read_only();
    bool    is_cpu_discardable();
    bool    is_gpu_discardable();

    // Get the pointers to the GPU version of this bitmap
    // (create & upload as necessary)
    _locked_ptr<CSubImage,cl_mem> lockCLData( CGPUProcessor* cl, bool read_only );
    void unlockData(cl_mem &);
    
    // Get pointers to the CPU version of this bitmap
    // (download & create as necessary)
    _locked_ptr<CSubImage,unsigned char *> lockData(bool read_only);
    void unlockData(unsigned char *);

    // Copy image from a block of memory
    void copy_bitmap( unsigned int width, unsigned int height, const unsigned char *data, size_t stride, int bpp, bool invert );

    // Apply the parent image's clipping rectangle
    void clipping( int& clip_x1, int& clip_y1, int& clip_x2, int& clip_y2, int &y_correct ) const;

};


typedef locking_ptr<CSubImage, unsigned char *> image_data_ptr;
typedef locking_ptr<CSubImage, cl_mem> image_cl_ptr;


#endif
