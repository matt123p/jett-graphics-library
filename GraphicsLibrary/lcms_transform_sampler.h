//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_lcms_transform_sampler_h
#define GraphicsLibrary_lcms_transform_sampler_h

// #define CMS_DLL 1
#include <lcms2.h>
#include "transform.h"


class lcms_transform_sampler : public transform_sampler
{
private:
    // Input colour profile
    cmsHPROFILE m_hInProfile;
	FILE*	    m_in_file;
    
    // The output colour profile
    cmsHPROFILE m_hOutProfile;
	FILE*		m_out_file;
    
    // The colour transform
    cmsHTRANSFORM m_hTransform;


    // Determine the LCMS colour type from the profile type
    int getLCMSType( cmsColorSpaceSignature colourspace );

    // Determine the LCMS scaling from the profile type
    float getLCMSScale( cmsColorSpaceSignature colourspace );
    
    // The scaling factors to make the output 0..1
    float   m_in_scale;
    float   m_out_scale;

    // The number of colours in the transform
    int     m_dimensions_in;
    int     m_dimensions_out;
    
    // Open a colour profile
    cmsHPROFILE openProfile( const TCHAR *filename, FILE* &file );
    cmsHPROFILE openProfile( CImage &image );
    void openTransform( int intent, int flags );

public:
    
    lcms_transform_sampler();
    virtual ~lcms_transform_sampler();

    //
    // Build the transform
    //
    void open( const TCHAR *file_in, const TCHAR *file_out, int intent, int flags );
    void open( CImage& image, const TCHAR *file_out, int intent, int flags );    
    void open( const TCHAR *file_in, CImage &image, int intent, int flags );
    void open( CImage& image_src, CImage& image_dst, int intent, int flags);
    
    //
    // Build the transform from a devicelink profile
    //
    void open( const TCHAR* devicelink_file, int intent, int flags );

    
    //
    // Create an in-memory standard profile
    //
    static cmsHPROFILE createStandardProfile( const TCHAR *file_in );
    
    //
    // Determine the transform properties
    //
    virtual int dimensions_in();
    virtual int dimensions_out();
    
    // 
    // Perform a colour management operation
    // 
    virtual void sample( float *sample_in, float *sample_out );

};

#endif
