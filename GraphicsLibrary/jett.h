//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//



/*! \file jett.h
 * \brief The main include file
 *
 * This file should be included by any project wishing to access the jett graphics library.
 */


#ifndef GraphicsLibrary_jett_h
#define GraphicsLibrary_jett_h

#ifndef __MACH__
#ifndef DLLEXPORT
#define DLLEXPORT __declspec(dllimport)
#endif
#else
#define DLLEXPORT
#define TCHAR char
#endif

struct _cl_mem;
typedef struct _cl_mem* cl_mem;
typedef int cl_int;

class CProcessor;
class CTransform;
class CImage;
class CFont;
class COrderedScreenCollection;
class CLinearizationCollection;

typedef CTransform* jett_transform;               ///< Opaque handle to a compiled colour transform.
typedef CFont*		jett_font;                    ///< Opaque handle to a loaded font.
typedef COrderedScreenCollection*	jett_screens; ///< Opaque handle to an ordered dither screen set.
typedef CLinearizationCollection*   jett_linearization; ///< Opaque handle to a linearization collection.

//
// Error handling
//
#include <string>
typedef std::string String;

/**
 * \defgroup error_codes Error Codes
 *
 * The error codes that can be returned
 *
 */
/*@{*/
#define JETT_INVALID_ARGUMENT            1  ///< At least argument is wrong
#define JETT_INTERNAL_ERROR              2  ///< An internal self-consistancy check failed
#define JETT_INVALID_PROFILE             3  ///< The profile is not suitable for the image
#define JETT_OPENCL_FAILURE              4  ///< An OpenCL error occurred, more information is inside the exception object
#define JETT_OPENCL_OUT_OF_MEMORY        5  ///< Out of memory on the GPU
#define JETT_INVALID_FONT                6  ///< The font cannot be read by FreeType
#define JETT_FREETYPE_FAILURE            7  ///< FreeType returned an error, more information is inside the expection object
#define JETT_GDI_FAILURE					8  ///< GDI returned an error, more informaion is inside the expection object
#define JETT_UNSUPPORTED_BITMAP          9  ///< The operation does not work with bitmaps of this type
#define JETT_IMAGE_READ_ONLY             10 ///< The image has been created read-only, so you cannot write to it
#define JETT_INVALID_BITMAP              11 ///< The bitmap is not valid or is corrupted
#define JETT_LCMS_FAILURE                12 ///< LCMS colour management system returned an error, there is more information inside the exception object
#define JETT_UNSUPPORTED_SCREEN          13 ///< The screen file is not correctly formatted
#define JETT_MATRIX_CANNOT_INVERT        14 ///< The matrix cannot be inverted (not all can!)
#define JETT_UNSUPPORTED_FEATURE         15 ///< This operation is not supported on this object type
#define JETT_INVALID_LINEARIZATION       16 ///< The imported points are invalid
#define JETT_FILE_NOT_FOUND				17 ///< The file is missing or cannot be opened


/*@}*/

/*!
 * \brief Exception type thrown by the public API.
 *
 * Functions in the Graphics Library report failures by throwing this type.
 * The main error classification is stored in \ref m_code and can be matched
 * against the values in \ref error_codes. \ref m_subsys_code optionally
 * stores a lower-level error code, for example from OpenCL or FreeType.
 *
 * \sa error_codes
 */
struct DLLEXPORT jett_exception
{
    char*	m_message;      ///< Human-readable description of the failure.
    int     m_code;        ///< One of the \ref error_codes values.
    int     m_subsys_code; ///< Optional subsystem-specific error code.
    
    /// Construct a new exception object.
    jett_exception( int code, int subsys_code, const char* message );
    /// Copy an existing exception object.
    jett_exception( const jett_exception& b );
    /// Destroy the exception and release the owned message string.
    ~jett_exception();

	/// Assign from another exception.
	jett_exception& operator=( const jett_exception &b );
};



/**
 * \defgroup bitblt_flags BitBlt flags
 *
 * The flags for the bitblt operation
 *
 */
/*@{*/
#define bitblt_cubic_scaling        1   ///< When the image needs to be resized use bi-cubic scaling
#define bitblt_nn_scaling			0	///< When the image needs to be resized use nearest-neighbour scaling (default)
#define bitblt_rotate_0             0	///< Do not rotate the image (default)
#define bitblt_rotate_90            2   ///< Rotate the image by 90 degrees clockwise
#define bitblt_rotate_180           4   ///< Rotate the image by 180 degrees clockwise
#define bitblt_rotate_270           8   ///< Rotate the image by 270 degrees clockwise
#define bitblt_white_is_transparent 16  ///< For 8bpp source images.  Make white pixels transparent in the copy
#define bitblt_use_alpha            32  ///< Use the Alpha component to merge the bitmap with the destination bitmap.  Only works for RGBA source imags
/*@}*/

/**
 * \defgroup polygon_flags Polygon Fill flags
 *
 * The flags for the polygon fill
 *
 */
/*@{*/
#define polygon_fast                0   ///< No anti-aliasing (default)
#define polygon_best                1   ///< Anti-aliasing

/*@}*/

/**
 * \defgroup line_flags Line drawing flags
 *
 * The flags for the line drawing
 *
 */
/*@{*/
#define line_fast                   0   ///< No anti-aliasing (default)
#define line_best                   1   ///< Anti-aliasing
#define line_join_none              2   ///< No join between lines
#define line_join_bevel             4   ///< Flat join between lines (default)
#define line_join_miter             8   ///< Miter join between lines
/*@}*/

/**
 * \defgroup string_flags Text drawing flags
 *
 * The flags for the text drawing
 *
 */
/*@{*/
#define string_rotate_0             0   ///< Draw the string horizontal to the Y axis (default)
#define string_rotate_90            1   ///< Draw the string rotated downwards
#define string_rotate_180           2   ///< Draw the string backwards
#define string_rotate_270           4   ///< Draw the string rotated upwards
/*@}*/


/**
 * \defgroup linearization_flags Linearization import flags
 *
 * The flags for the linearization
 *
 */
/*@{*/
#define lin_import_normal           0   ///< The data represents a linearization curve directly (default)
#define lin_import_measurement      1   ///< The data represents a measurement curve
#define lin_import_filter           2   ///< Filter the data to reduce measurement noise
/*@}*/

/**
 * \defgroup rendering_intents Rendering Intent flags
 *
 * The flags for the rendering intents
 *
 */
/*@{*/

// ICC Intents
#define INTENT_PERCEPTUAL                0  ///< Create a profile with Perceptual rendering intent
#define INTENT_RELATIVE_COLORIMETRIC     1  ///< Create a profile with Relative Colourmetric rendering intent
#define INTENT_SATURATION                2  ///< Create a profile with Saturation rendering intent
#define INTENT_ABSOLUTE_COLORIMETRIC     3  ///< Create a profile with Absoluted Colourmetric rendering intent

// Non-ICC intents
#define INTENT_PRESERVE_K_ONLY_PERCEPTUAL             10 ///< Create a profile with Perceptual rendering intent but preserve K channel in CMYK->CMYK transforms
#define INTENT_PRESERVE_K_ONLY_RELATIVE_COLORIMETRIC  11 ///< Create a profile with Relative Colourmetric rendering intent but preserve K channel in CMYK->CMYK transforms
#define INTENT_PRESERVE_K_ONLY_SATURATION             12 ///< Create a profile with Saturation rendering intent but preserve K channel in CMYK->CMYK transforms
#define INTENT_PRESERVE_K_PLANE_PERCEPTUAL            13 ///< Create a profile with Perceptual rendering intent but preserve K plane in CMYK->CMYK transforms
#define INTENT_PRESERVE_K_PLANE_RELATIVE_COLORIMETRIC 14 ///< Create a profile with Relative Colourmetric rendering intent but preserve K plane in CMYK->CMYK transforms
#define INTENT_PRESERVE_K_PLANE_SATURATION            15 ///< Create a profile with Saturation rendering intent but preserve K plane in CMYK->CMYK transforms

/*@}*/


/*!
 * \brief A two-dimensional floating point coordinate.
 *
 * This type is used by the drawing and transform APIs for points, extents,
 * and measured text dimensions.
 */
struct DLLEXPORT jett_point
{
    float x; ///< Horizontal coordinate in pixels.
    float y; ///< Vertical coordinate in pixels.
    
    /// Construct the point `(0, 0)`.
    jett_point()
    {
        x = 0;
        y = 0;
    }
    
    /// Construct a point from floating point coordinates.
    jett_point( double _x, double _y )
    {
        x = static_cast<float>(_x);
        y = static_cast<float>(_y);
    }

};

/*!
 * \brief An affine transformation matrix.
 *
 * The matrix is represented as:
 *
 * \code
 * a b 0
 * c d 0
 * e f 1
 * \endcode
 *
 * It can represent rotation, scaling, shear, and translation. The matrix is
 * primarily used with matrix-based bitblt operations and transformed font
 * rendering.
 */
struct DLLEXPORT jett_matrix
{
    //
    // The matrix is:
    //  a b 0
    //  c d 0
    //  e f 1
    //
	float a;    ///< First row, first column.
	float b;    ///< First row, second column.
	float c;    ///< Second row, first column.
	float d;    ///< Second row, second column.
	float e;    ///< Translation in X.
	float f;    ///< Translation in Y.
	 
	/// Construct a matrix with full affine coefficients, including translation.
	jett_matrix( float _a, float _b, float _c, float _d, float _e, float _f );

	/// Construct a matrix with affine coefficients and no translation.
	jett_matrix( float _a, float _b, float _c, float _d );

	/// Construct the identity matrix.
	jett_matrix();
    
    /// Apply the transform to a point.
    jett_point apply( const jett_point &p ) const;
    
    /// Append another affine transform to this matrix.
    void append( const jett_matrix & m );
    
    /// Return the inverse matrix.
    /// 	hrows jett_exception if the matrix is not invertible.
    jett_matrix invert() const;
    
    /// Append a rotation in radians.
    void rotate( double angle );
    
    /// Append a translation.
    void translate( jett_point p );
    
    /// Return true if the matrix is the identity transform.
    bool is_ident() const;

};

//
// The image/bitmap class
//


/**
 * \enum image_t
 * \brief Pixel formats supported by jett_image.
 *
 * Most drawing operations accept all full-colour formats and transparently
 * handle 8-bit or 16-bit channel depth. The low bit-depth monochrome formats
 * are primarily used as printer output targets for dithering.
 **/
 enum image_t
{
    image_invalid,		///< Invalid, undefined
	image_mono1,		///< A 1 bpp monochrome image (Can only be used with dithering)
    image_mono2,		///< A 2 bpp monochrome image (Can only be used with dithering)
    image_mono4,		///< A 4 bpp monochrome image (Can only be used with dithering)
    image_mono,			///< A 8 / 16 bpp monochrome image
    image_rgb,			///< A 24 / 48 bpp rgb image
    image_bgr,			///< A 24 / 48 bpp rgb image in Windows/BMP format
    image_rgba,			///< A 32 / 64 bpp rgb image with an Alpha channel
    image_bgra,			///< A 32 / 64 bpp rgb image with an Alpha channel in Windows/BMP format
    image_cmyk,			///< A 32 / 64 bpp CMYK image
    image_lab,          ///< A 24 / 48 bpp 1976 CIE L*a*b* image
};



/**
 * \enum image_mode_t
 * \brief CPU/GPU cache policy for an image.
 *
 * This mode controls where an image prefers to live when the library is using
 * the OpenCL backend. The default mode is appropriate for most applications.
 * More specialized modes are useful when you know an image is GPU-resident,
 * CPU-only, or intended for repeated read-only access from the GPU.
 *
 * | Mode                  | When in GPU discard from CPU | Read-only in GPU | Discard from GPU |
 * | :-------------------- | :--------------------------: | :--------------: | :--------------: |
 * | image_mode_default    |        Yes                   |     No           |      No          |
 * | image_mode_gpu_ro     |        Yes                   |     Yes          |      No          |
 * | image_mode_cpu_only   |        No                    |     Yes          |      Yes         |
 * | image_mode_gpu_copy   |        No                    |     Yes          |      No          |
 */
enum image_mode_t
{
    image_mode_default,             ///<  Exists on both GPU & CPU and is transferred back/forth as necessary
    image_mode_gpu_ro,              ///<  After loading is transferred up to GPU as read-only object
    image_mode_cpu_only,            ///<  Exists on CPU and is copied up to GPU automatically and discarded from GPU when operation finished
    image_mode_gpu_copy             ///<  Exists on CPU and is copied up to GPU automatically as a read-only object
};


/*!
 * \brief Progress snapshot reported during screen generation.
 */
struct jett_progress
{
    int     m_phase;        ///< The phase of screen creation.  1 is initial bitmap generation, 2 is rank generation.
    int     m_iteration;    ///< The iteration of this phase
    int     m_total;        ///< The total number of iterations expected for this phase (note, for phase 1 this is just an estimate).
};

/// Callback prototype used by create_screens() to report progress.
typedef void (*jett_progress_callback)( jett_progress &b );

/*!
 * \brief Public image container used by the Graphics Library.
 *
 * An jett_image owns bitmap data plus related metadata such as pixel format,
 * resolution, clipping rectangle, and embedded ICC profile data. The handle is
 * reference-counted internally, so copying an jett_image shares the same
 * underlying image rather than duplicating pixels.
 *
 * Typical use:
 * - construct an empty image;
 * - call createImage() or loadFromFile();
 * - optionally attach profile data or a clip rectangle;
 * - draw through an jett context or access raw pixels via lockData();
 * - save, discard, or let the object destruct.
 */
class DLLEXPORT jett_image
{

    CImage *m_pImage;
    
public:
    /// Construct an empty image handle.
    jett_image();
    
    /// Copy an image handle and share the same underlying bitmap.
    jett_image(jett_image& image);
    
    /// Release this handle's reference to the underlying bitmap.
    ~jett_image();
    
    /// Assign another image handle and share the same underlying bitmap.
    jett_image& operator=( jett_image &b );
    
    /**
     * \brief Create an uninitialized image buffer.
     *
     * \param width Width in pixels.
     * \param height Height in pixels.
     * \param type Pixel format of the new image.
     * \param image_16bpp Set to true for 16-bit channels, false for 8-bit channels.
     * \param mode CPU/GPU cache policy.
     *
     * The pixel data is intentionally left uninitialized for performance.
     * Call erase() if you need a white background before drawing.
     */
    void createImage( unsigned int width, unsigned int height, image_t type, bool image_16bpp = false, image_mode_t mode = image_mode_default );
#ifndef __MACH__
	/**
	 * \brief Create a Windows-compatible bitmap-backed image.
	 *
	 * This overload is intended for GDI integration and therefore only supports
	 * image types that can be represented as a Windows bitmap.
	 */
	HBITMAP createBitmap( unsigned int width, unsigned int height, image_t type );
#endif

    /**
     * \brief Load an image from a file.
     *
     * Supported input formats are JPEG, PNG, BMP, and TIFF. Where the file
     * format supports it, embedded ICC profile data is imported automatically.
     */
    void loadFromFile( const TCHAR *filename, image_mode_t mode = image_mode_default );

    /// Narrow-character overload of loadFromFile().
#ifndef __MACH__
    void loadFromFile( const char *filename, image_mode_t mode = image_mode_default );
#endif

    /**
     * \brief Save the image to a file.
     *
     * The output format is selected from the filename extension.
     */
    void saveToFile( const TCHAR *filename );
    
    /// Narrow-character overload of saveToFile().
#ifndef __MACH__
    void saveToFile( const char *filename );
#endif

    /// Return true if the image stores colour channels in BGR or BGRA order.
    bool isBGR() const;
    
    /// Return true if the image uses 16-bit channels.
    bool is16bpp() const;
    
	/// Return the image width in pixels.
	int getWidth() const;
    /// Return the image height in pixels.
    int getHeight() const;
    /// Return the number of bytes between consecutive scan lines.
    size_t getStride() const;
    /// Return the number of bytes per pixel.
    size_t getPixelStride() const;
    /// Return the number of colour channels in the image format.
    int getComponents() const;
    /// Return the image pixel format.
    image_t getType() const;
    
    /**
     * \brief Release all resources associated with the current image contents.
     *
     * After discard(), the handle becomes empty and must be repopulated with
     * createImage() or loadFromFile() before further use.
     */
    void discard();
    
    /// Fill the image with white pixels.
    void erase();
    
    /**
     * \brief Import pixel data from a caller-owned bitmap buffer.
     *
     * Use this to bridge from external bitmap memory into an jett_image.
     */
    void copy_bitmap( unsigned int width, unsigned int height, image_t type, image_mode_t mode, const unsigned char *data, size_t stride, int bpp, bool invert );
        
    /**
     * \brief Access the CPU-visible pixel buffer.
     *
     * \param read_only Set to true when the caller will not modify the data.
     * \return Pointer to the first pixel of the CPU representation.
     *
     * If the image currently resides only on the GPU, a CPU copy is created or
     * downloaded automatically. Every successful lockData() call must be paired
     * with unlockData().
     */
    unsigned char* lockData(bool read_only);
    /// Release a pointer previously obtained from lockData().
    void unlockData();
    
    /// Return a pointer to the embedded ICC profile data, or NULL if none is attached.
    void* get_profile_data() const;
    /// Return the size in bytes of the embedded ICC profile data.
    size_t get_profile_size() const;
    
    /**
     * \brief Attach ICC profile data from a file path or built-in profile token.
     *
     * Built-in tokens are :srgb, :mono, and :lab.
     */
    void set_profile_data( const TCHAR *filename );

    /// Narrow-character overload of set_profile_data().
#ifndef __MACH__
    void set_profile_data( const char *filename );
#endif
    /**
     * \brief Attach ICC profile data from a memory buffer.
     *
     * The profile bytes are copied into the image; the caller retains ownership
     * of the input buffer.
     */
    void set_profile_data( void* data, size_t size);

    /// Return the stored image resolution in DPI.
    void get_resolution( int &x, int &y ) const;
    /// Set the stored image resolution in DPI.
    void set_resolution( int x, int y );

    /**
     * \brief Restrict subsequent drawing to a rectangle within the image.
     *
     * The clip rectangle applies to later drawing operations that target this image.
     */
    void set_clip_rect( int clip_x1, int clip_y1, int clip_width, int clip_height );
    /// Return the current clipping rectangle.
    void get_clip_rect( int& clip_x1, int& clip_y1, int& clip_width, int& clip_height );
    /// Remove any active clipping rectangle.
    void unset_clip_rect();

    /// Convert to the internal image implementation pointer.
    operator CImage*()
    {
        return m_pImage;
    }
};




//
// Structures to extend the colour management system
//

/*!
 * \brief Abstract interface for user-defined colour transforms.
 *
 * Implement this class when you need a custom channel-to-channel mapping that
 * is not conveniently expressed as an ICC transform. After passing an instance
 * to build_transform(transform_sampler*), the library copies the sampled result
 * into its own internal transform representation, so the sampler object may be
 * destroyed.
 */
class DLLEXPORT transform_sampler
{
public:
    /// Virtual destructor for subclass cleanup.
    virtual ~transform_sampler() { }
    
    /// Return the number of input channels consumed by sample().
    virtual int dimensions_in() = 0;
    /// Return the number of output channels written by sample().
    virtual int dimensions_out() = 0;
    
    /// Transform one normalized input sample into one normalized output sample.
    virtual void sample( float *sample_in, float *sample_out ) = 0;
};


/*!
 * \brief Primary library context and factory for graphics operations.
 *
 * Create one jett object for each execution backend you want to use. Most
 * applications create a single CPU-backed object. Performance-sensitive print
 * pipelines may create an additional GPU-backed object and move work between
 * them explicitly.
 *
 * This class owns the processor backend and exposes the complete public API for
 * image composition, colour management, dithering, text rendering, font and
 * screen creation, and low-level OpenCL access.
 */
class DLLEXPORT jett
{
private:
    
    // The connection to processor (GPU or CPU) that we are using
    CProcessor* m_cl;
    
public:
    
    /// Construct an uninitialized library context.
    jett();
    /// Destroy the processor backend owned by this context.
    ~jett();
    
    /**
     * \brief Initialize the library backend.
     *
     * \param use_gpu Set to true to select the OpenCL backend, or false to use the CPU backend.
     * \throws jett_exception if initialization fails.
     *
     * Call init() exactly once, immediately after construction and before any
     * other API on the object.
     */
    void init( bool use_gpu );
    
    //
    // The bitblt commands
    //
    
    /**
     * \brief Copy a source rectangle into a destination rectangle.
     *
     * This overload performs the full bitblt pipeline: optional colour
     * transform, optional resize, optional 90-degree rotation, optional alpha
     * blending, and optional monochrome transparency. Behaviour is controlled
     * by the flags from \ref bitblt_flags.
     */
    void bitblt( jett_transform trans, jett_image& src_bitmap,
              int src_x1, int src_y1, int src_width, int src_height,
              jett_image& dst_bitmap,
              int dst_x1, int dst_y1, int dst_width, int dst_height, int flags );

    /// Copy an entire source image to a destination position.
    void bitblt( jett_transform trans, jett_image& src_bitmap,
                jett_image& dst_bitmap, int dst_x1, int dst_y1, int flags );

    /// Copy a source rectangle to a destination position without resizing.
    void bitblt( jett_transform trans, jett_image& src_bitmap,
                int src_x1, int src_y1, int src_width, int src_height,
                jett_image& dst_bitmap, int dst_x1, int dst_y1, int flags );
    
    /**
     * \brief Draw a source rectangle using an affine matrix.
     *
     * Use this overload for arbitrary rotation, scaling, shear, and translated
     * placement that cannot be expressed as an axis-aligned destination rectangle.
     */
    void bitblt( jett_transform trans, jett_image& src_bitmap,
              int src_x1, int src_y1, int src_width, int src_height,
              jett_image& dst_bitmap,
              const jett_matrix& matrix, int flags );
    
    /// Fill a polygon on the destination image.
    void polygon( jett_image& dst_bitmap, unsigned char *colour, jett_point *points, int n, int flags );

    /// Draw a polyline, or a closed path when close is true.
    void lines( jett_image& dst_bitmap, unsigned char *colour, float width, jett_point *points, int n, bool close, int flags );

    /// Fill an axis-aligned rectangle on the destination image.
    void rectangle( jett_image& dst_bitmap, unsigned char *colour, int dst_x1, int dst_y1, int dst_width, int dst_height );
    
    /// Draw Unicode text using a previously created font.
    void text( jett_font f, const unsigned char* colour, jett_image& dst_bitmap, int x, int y, const TCHAR * str, int flags );

    /// Narrow-character overload of text().
#ifndef __MACH__
	void text( jett_font f, const unsigned char* colour, jett_image& dst_bitmap, int x, int y, const char * str, int flags );
#endif

    /// Measure the rendered size of a Unicode string.
    jett_point size_text( jett_font f, const TCHAR * str );

    /// Narrow-character overload of size_text().
#ifndef __MACH__
    jett_point size_text( jett_font f, const char * str );
#endif

    /// Block until all queued work on this context has completed.
    void flush();

    /**
     * \brief Convert a single pixel with a compiled colour transform.
     *
     * This is useful for validation, table generation, and reproducing the
     * exact colour math used by the library for full-image operations.
     */
    void convert( jett_transform trans, const unsigned char *src_pixel, unsigned char *dst_pixel );
    
	/**
	 * \brief Create a font from a font file.
	 *
	 * Width and height are expressed in pixels.
	 */
	jett_font create_font( const TCHAR *filename, int width, int height, bool anti_alias );

#ifndef __MACH__
	/// Narrow-character overload of create_font().
	jett_font create_font( const char *filename, int width, int height, bool anti_alias );
#endif
    
    /// Apply an affine transform to a font before drawing or measuring text.
    void set_font_matrix( jett_font f, const jett_matrix & matrix);

#ifndef __MACH__
	/// Create a font from an existing Windows HFONT with explicit pixel size.
	jett_font create_font( HFONT f, int width, int height, bool anti_alias );	

	/// Create a font from an existing Windows HFONT.
	jett_font create_font( HFONT f );	
#endif

    /// Destroy a font created by any create_font() overload.
    void destroy_font( jett_font f );

    
	/// Create an empty linearization collection.
    jett_linearization create_linearization();

	/**
	 * \brief Import a linearization curve into a collection.
	 *
	 * \param lin Target collection.
	 * \param curve Array of points describing the curve.
	 * \param points Number of entries in curve.
	 * \param options Flags from \ref linearization_flags.
	 */
    void import_linearization( jett_linearization lin, jett_point* curve, size_t points, int options );
    
	/// Apply a linearization collection to an image in place.
    void linearize( jett_image& src_image, jett_linearization linearization );

	/// Destroy a linearization collection.
    void destroy_linearization( jett_linearization lin );

    
	/**
	 * \brief Build a colour transform from two ICC profile paths or built-in profile tokens.
	 *
	 * Supported built-in profile tokens are :srgb, :mono, and :lab.
	 */
    jett_transform build_transform( const TCHAR *file_in, const TCHAR *file_out, int intent );
    /// Build a colour transform from an image profile to a profile path or built-in token.
    jett_transform build_transform( jett_image& src_image, const TCHAR *file_out, int intent );
    /// Build a colour transform from a profile path or built-in token to an image profile.
    jett_transform build_transform( const TCHAR *file_in, jett_image& dst_image, int intent );
    /// Build a colour transform between the embedded profiles of two images.
    jett_transform build_transform( jett_image& src_image, jett_image& dst_image, int intent );

#ifndef __MACH__
	/// Narrow-character overload of build_transform().
	jett_transform build_transform( const char *file_in, const char *file_out, int intent );
    /// Narrow-character overload of build_transform().
    jett_transform build_transform( jett_image& src_image, const char *file_out, int intent );
    /// Narrow-character overload of build_transform().
    jett_transform build_transform( const char *file_in, jett_image& dst_image, int intent );
#endif
    
    /**
     * \brief Build a transform from a device-link profile or built-in device-link token.
     *
     * Supported built-in device-link tokens are :mono_cmyk and :mono_rgb.
     */
    jett_transform build_transform( const TCHAR *devicelink_file, int intent );
#ifndef __MACH__
	/// Narrow-character overload of build_transform().
	jett_transform build_transform( const char *devicelink_file, int intent );
#endif
    
    /// Build a transform from a caller-supplied transform_sampler implementation.
    jett_transform build_transform( transform_sampler* pSampler );
    
    /// Destroy a transform created by any build_transform() overload.
    void destroy_transform( jett_transform trans );

	/**
	 * \brief Generate ordered dither screens with the void-and-cluster technique.
	 *
	 * \param planes Number of output colour planes.
	 * \param size Pointer to an array of one screen size per plane.
	 * \param sigma Void-and-cluster sigma parameter.
	 * \param output_levels Number of output tone levels per plane.
	 * \param callback Optional progress callback.
	 */
	jett_screens create_screens( int planes, int* size, double sigma, int output_levels, jett_progress_callback callback = NULL);

	/// Load a previously saved screen set from a file.
	jett_screens load_screens( const TCHAR *filename );

#ifndef __MACH__
	/// Narrow-character overload of load_screens().
	jett_screens load_screens( const char *filename );
#endif

	/// Save a screen set to a file for later reuse.
	void save_screens( jett_screens s, const TCHAR *filename );

#ifndef __MACH__
	/// Narrow-character overload of save_screens().
	void save_screens( jett_screens s, const char *filename );
#endif

	/// Destroy a screen set.
	void destroy_screens( jett_screens s );

	/**
	 * \brief Dither an image into separate per-plane output images.
	 *
	 * The destination array must contain pre-created output images compatible
	 * with the selected screen set.
	 */
	void dither( jett_image& src_image, jett_image* dst_images, jett_screens screens, jett_linearization linearization, int scale = 0 );

	/// Dither an image in place.
	void dither( jett_image& src_image, jett_screens screens, jett_linearization linearization, int scale = 0 );

	/// Force an image into GPU memory. Only valid when init(true) was used.
	void cache_image( jett_image &image, bool read_only );

	/**
	 * \brief Lock an image for direct OpenCL access.
	 *
	 * Pair every successful call with unlockCLData(). Only valid when the
	 * context uses the GPU backend.
	 */
	cl_mem lockCLData( jett_image &image, bool read_only );
	/// Release a lock obtained from lockCLData().
	void unlockCLData( jett_image &image );
};


#endif
