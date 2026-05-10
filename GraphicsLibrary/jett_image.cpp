//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"
#include "jett.h"
#include "Image.h"
#include "Processor.h"


jett_image::jett_image()
{
    m_pImage = new CImage;
    m_pImage->inc_ref();
}

jett_image::jett_image(jett_image& image)
{
    m_pImage = image;
    m_pImage->inc_ref();
}

jett_image::~jett_image()
{
    m_pImage->dec_ref();
}


jett_image& jett_image::operator=( jett_image &b )
{
    if (m_pImage)
    {
        m_pImage->dec_ref();
        m_pImage = NULL;
    }
    
    m_pImage = b;
    m_pImage->inc_ref();
    
    return *this;
}

/*!
 * \brief The free the resources associated with this image
 *
 * This function frees all of the resources assocated with the image.  The image cannot be used
 * again until a loadFromFile() or createImage() is called.
 *
 */
void jett_image::discard()
{
    m_pImage->discard();
}

/** @name Image creation
 *  This function creates a bitmap
 */
///@{


/*!
 * \brief Create an unitialised bitmap
 *
 * \param width         The width of the image (in pixels)
 * \param height        The height of the image (in pixels)
 * \param type          The pixel format of the image
 * \param image_16bpp   The pixel should be 8bpp or 16bpp
 * \param mode          The cache mode of the image
 *
 * This functions creates the image in memory.  The image at this point should be assumed
 * to be unitialised and have random data in it until it is either erased or a bitblt command
 * is used to paint to it.
 *
 */
void jett_image::createImage( unsigned int width, unsigned int height, image_t type, bool image_16bpp, image_mode_t mode )
{
    m_pImage->createImage( width, height, type, image_16bpp, mode );
}

#ifndef __MACH__
/*!
 * \brief Create an unitialised windows bitmap
 *
 * \param width     The width of the image (in pixels)
 * \param height    The height of the image (in pixels)
 * \param type      The pixel format of the image
 *
 * This functions creates a windows compatible bitmap.  The image must be in a mode compatible
 * with Windows, i.e. image_mono1, image_mono2, image_mono4, image_mono8, image_bgr, image_bgra.
 *
 */
HBITMAP jett_image::createBitmap( unsigned int width, unsigned int height, image_t type )
{
    return m_pImage->createBitmap( width, height, type );
}
#endif



/*!
 * \brief Copy data in to this bitmap from another source
 *
 * \param width     The width of the image (in pixels)
 * \param height    The height of the image (in pixels)
 * \param type      The pixel format of the image
 * \param mode      The cache mode of the image
 * \param data      The external source bitmap
 * \param stride    The number of bytes per line in the source image
 * \param bpp       The number of bits per pixel of the source image
 * \param invert    Set to true to cause black/white inversion during the copy
 *
 * This function is used to copy image data in to this bitmap from another source that is not a jett_image.  It can be
 * used to integrate the jett graphics library with other sources of bitmaps.
 *
 */
void jett_image::copy_bitmap( unsigned int width, unsigned int height, image_t type, image_mode_t mode, const unsigned char *data, size_t stride, int bpp, bool invert )
{
    m_pImage->copy_bitmap( width, height, type, mode, data, stride, bpp, invert );
}


/*!
 * \brief Make the bitmap all white
 *
 * This function initialises the bitmap to be all white.  After an image is created using createImage() it is not
 * initialised and the contents are random.  Call this function to initialise the image.
 *
 */
void jett_image::erase()
{
    m_pImage->erase();
}

///@}


/** @name File operations
 *  This functions load and save bitmaps to files.
 */
///@{


/*!
 * \brief Load a bitmap from a file
 *
 * \param filename  The filename of the bitmap to load
 * \param mode      The cache mode of the image
 *
 * This function will load an image from file in JPEG, PNG, BMP or TIFF format.
 * The image can be monochrome (8 bpp), RGB, RGBA, CMYK or L*a*b* pixel format.
 *
 * Bitmap files that are in bpp less than 8 will automatically be converted
 * to at least 8 bpp.  This is different from the saveToFile() function which
 * will save bitmaps with less than 8bpp without up-converting.
 *
 * Note: This function supports loading BMP files that have 2bpp (that is 4 colour bitmaps)
 * such bitmaps are NOT supported by Windows and will not be displayed as thumbnails or 
 * be loaded by paint.  However, they are useful for greyscale inkjet printheads.
 *
 * Not all BMP files are supported:
 * - 16 bpp BMP files are not supported.
 *
 * - Embedded profiles in BMP files are not supported.
 *
 * - Compressed BMP files (in any format) are not supported. 
 *
 *
 * Not all file types support all bitmap modes:
 *
 * | Feature                    |  BMP  |  JPEG  |  TIFF  |  PNG  |
 * | :------------------------- | :---: | :----: | :----: | :---: |
 * | Monochrome                 |  Yes  |  Yes   |  Yes   |  Yes  |
 * | RGB                        |  Yes  |  Yes   |  Yes   |  Yes  |
 * | RGB + Transparency (RGBA)  |  *1*  |  *1*   |  Yes   |  Yes  |
 * | CMYK                       |  No   |  Yes   |  Yes   |  No   |
 * | CIE L*a*b*                 |  No   |  No    |  Yes   |  No   |
 * | Embedded ICC Profile       |  No   |  Yes   |  Yes   |  Yes  |
 * | 16 bit RGB, CMYK & L*a*b*  |  No   |  No    |  Yes   |  No   |
 *
 * *1: BMPs and JPEGs do not support transparency.*
 *
 */
void jett_image::loadFromFile( const TCHAR *filename, image_mode_t mode )
{
    m_pImage->loadFromFile( filename, mode );
}

#ifndef __MACH__
void jett_image::loadFromFile( const char *filename, image_mode_t mode )
{
    m_pImage->loadFromFile( to_unicode(filename).c_str(), mode );
}
#endif


/*!
 * \brief Save a bitmap to a file
 *
 * \param filename  The filename to save the bitmap to
 *
 * This function will save the image as a TIFF, JPEG, PNG or BMP file.  It looks at the
 * extension of the filename and determines the correct format to save the file as.
 *
 * This functions supports saving image_mono1, image_mono2 and image_mono4 bitmaps
 * created from the dither routine.
 *
 * Note: This function supports saving BMP files that have 2bpp (that is 4 colour bitmaps)
 * such bitmaps are NOT supported by Windows and will not be displayed as thumbnails or 
 * be loaded by paint.  However, they are useful for greyscale inkjet printheads.
 *
 * Not all file types support all bitmap modes.  An attempt to save a bitmap
 * to a file type that does not support it will cause an exception.
 *
 *
 * | Feature                    |  BMP  |  JPEG  |  TIFF  |  PNG  |
 * | :------------------------- | :---: | :----: | :----: | :---: |
 * | Monochrome                 |  Yes  |  Yes   |  Yes   |  Yes  |
 * | RGB                        |  Yes  |  Yes   |  Yes   |  Yes  |
 * | RGB + Transparency (RGBA)  |  *1*  |  *1*   |  Yes   |  Yes  |
 * | CMYK                       |  No   |  Yes   |  Yes   |  No   |
 * | CIE L*a*b*                 |  No   |  No    |  Yes   |  No   |
 * | Embedded ICC Profile       |  No   |  Yes   |  Yes   |  Yes  |
 * | Save <8 bpp bitmaps        |  Yes  |  No    |  No    |  No   |
 * | 16 bit RGB, CMYK & L*a*b*  |  No   |  No    |  Yes   |  No   |
 *
 * *1: BMPs and JPEGs do not support transparency, but the image will be saved and the transparency data lost.*
 *
 */
void jett_image::saveToFile( const TCHAR *filename )
{
    m_pImage->saveToFile(filename);
}

#ifndef __MACH__
void jett_image::saveToFile( const char *filename )
{
    m_pImage->saveToFile(to_unicode(filename).c_str());
}
#endif

///@}



/** @name Attributes
 *  Find out the image's attributes
 */
///@{

/*!
 * \brief Is this image in BGR format?
 *
 * This function will return true if the image is in BGR or BGRA pixel format.
 *
 */
bool jett_image::isBGR() const
{
    return m_pImage->isBGR();
}

/*!
 * \brief Is this image in 16 bpp format?
 *
 * This function will return true if the image is in a 16bpp format.
 *
 */
bool jett_image::is16bpp() const
{
    return m_pImage->is16bpp();
}


/*!
 * \brief The width of the image
 *
 * This function returns the width of the image in pixels
 *
 */
int jett_image::getWidth() const
{
    return m_pImage->getWidth();
}

/*!
 * \brief The height of the image
 *
 * This function returns the height of the image in pixels
 *
 */
int jett_image::getHeight() const
{
    return m_pImage->getHeight();
}

/*!
 * \brief The byte line stride of the image
 *
 * This function returns the number of bytes per line in the image
 *
 */
size_t jett_image::getStride() const
{
    return m_pImage->getStride();
}

/*!
 * \brief The byte pixel stride of the image
 *
 * This function returns the number of bytes per pixel in the image.
 *
 * NOTE: The width of images (in bytes) are always rounded to the nearest 4 byte boundry.  This is consistant with Windows BMP format.
 * It is recommended this function is used to calculate the correct width in bytes.
 *
 */
size_t jett_image::getPixelStride() const
{
    return m_pImage->getPixelStride();
}

/*!
 * \brief The number of colour components in the image
 *
 * This function returns the number of colours in the image.  For example it will return 3 for RGB images and 4 
 * for CMYK images.
 *
 */
int jett_image::getComponents() const
{
    return m_pImage->getComponents();
}

/*!
 * \brief The pixel format of the image
 *
 * This function returns the pixel format of the image.
 *
 */
image_t jett_image::getType() const
{
    return m_pImage->getType();
}



/*!
 * \brief The resolution of the image in DPI
 *
 * This function returns the resolutions of the image in DPI
 *
 */
void jett_image::get_resolution( int &x, int &y ) const
{
    m_pImage->get_resolution(x, y);
}


/*!
 * \brief The resolution of the image in DPI
 *
 * This function sets the resolutions of the image in DPI
 *
 */
void jett_image::set_resolution( int x, int y )
{
    m_pImage->set_resolution(x, y);
}

///@}



/** @name Embdedded Profile
 *  Read and set the embedded ICC profile for this image
 */
///@{

/*!
 * \brief Get the embedded profile data
 *
 * This function returns a pointer to the embedded ICC colour profile.  It will return NULL if the image
 * does not have an embedded profile.
 *
 */
void* jett_image::get_profile_data() const
{
    return m_pImage->get_profile_data();
}

/*!
 * \brief Get the embedded profile data size
 *
 * This function returns the size of the embedded ICC colour profile.  It will return 0 if the image
 * does not have an embedded profile.
 *
 */
size_t jett_image::get_profile_size() const
{
    return m_pImage->get_profile_size();
}

/*!
 * \brief Set the embedded profile from an ICC file
 *
 * \param   filename    The ICC file to load and use as the profile
 *
 * This function loads the file and sets it as the embedded profile of this image.  When the
 * image is saved or the image is used with a transform creation function this profile is used.
 *
 */
void jett_image::set_profile_data( const TCHAR *filename )
{
	m_pImage->set_profile_data(filename);
}

#ifndef __MACH__
void jett_image::set_profile_data( const char *filename )
{
    m_pImage->set_profile_data(to_unicode(filename).c_str());
}
#endif

/*!
 * \brief Set the embedded profile from memory
 *
 * \param   data    A pointer to memory holding the ICC profile
 * \param   size    The size of the ICC profile.
 *
 * This function set ithe embedded profile of this image.  The data is copied so the memory block pointed to
 * by data is not required once the function returns. 
 * When the image is saved or the image is used with a transform creation function this profile is used.
 *
 */
void jett_image::set_profile_data( void* data, size_t size)
{
    m_pImage->set_profile_data(data, size);
}


///@}


/** @name Clipping
 *  Get and set the clipping rectangle for the image
 */
///@{

/*!
 * \brief Set the image's clipping rectangle
 *
 * \param clip_x1        The left edge of the clipping rectangle
 * \param clip_y1        The top edge of the clipping rectangle
 * \param clip_width     The width of the clipping rectangle
 * \param clip_height    The height of the clipping rectangle
 *
 * This function sets the clipping rectangle for image.  The clipping rectangle
 * is used whenever this image is the dst_bitmap for any drawing operation.
 *
 * It is not used if the image is the source of a bitblt.
 *
 * \sa \ref page_clip
 *
 */
void jett_image::set_clip_rect( int clip_x1, int clip_y1, int clip_width, int clip_height )
{
    m_pImage->set_clip_rect( clip_x1, clip_y1, clip_width, clip_height );
}

/*!
 * \brief Unset the image's clipping rectangle
 *
 * \param clip_x1        The left edge of the clipping rectangle
 * \param clip_y1        The top edge of the clipping rectangle
 * \param clip_width     The width of the clipping rectangle
 * \param clip_height    The height of the clipping rectangle
 *
 * This function unsets the clipping rectangle for image.  After this call
 * no clipping is applied to the drawing operations.
 *
 * \sa \ref page_clip
 *
 */
void jett_image::unset_clip_rect()
{
    m_pImage->set_clip_rect( 0, 0, m_pImage->getWidth(), m_pImage->getHeight() );
}


/*!
 * \brief Get the image's clipping rectangle
 *
 * \param clip_x1        The left edge of the clipping rectangle
 * \param clip_y1        The top edge of the clipping rectangle
 * \param clip_width     The width of the clipping rectangle
 * \param clip_height    The height of the clipping rectangle
 *
 * This function gets the current clipping rectangle for image.
 *
 * \sa \ref page_clip
 *
 */
void jett_image::get_clip_rect( int& clip_x1, int& clip_y1, int& clip_width, int& clip_height )
{
    m_pImage->get_clip_rect( clip_x1, clip_y1, clip_width, clip_height );
}

///@}

/** @name Access raw data
 *  Access the bitmap's underlying raw data
 */
///@{


/*!
 * \brief Access the raw data of this bitmap
 *
 * \param read_only   Set to true if you only require read-only access to the raw data
 *
 * This function will return a pointer to the raw image data.  When the data is finished with call the unlockData() function.
 *
 * NOTE: The width of images (in bytes) are always rounded to the nearest 4 byte boundry.  This is consistant with Windows BMP format.
 * It is recommended that functions use the getPixelStride function to ensure the correct width in bytes is used.
 */
unsigned char* jett_image::lockData(bool read_only)
{
    return m_pImage->lockData( read_only );
}

/*!
 * \brief Finish the access of the raw data of this bitmap
 *
 * When the raw data is finished with call this function.
 */
void jett_image::unlockData()
{
    m_pImage->unlockData();
}


///@}
