//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "lcms_transform_sampler.h"
#include "Image.h"


static void LogErrorHandlerFunction(cmsContext ContextID, cmsUInt32Number ErrorCode,
                                    const char *Text)
{
    
}


lcms_transform_sampler::lcms_transform_sampler()
{
    cmsSetLogErrorHandler(LogErrorHandlerFunction);
    m_hInProfile = NULL;
    m_hOutProfile = NULL;
    m_hTransform = NULL;
    m_in_scale = 1.0f;
    m_out_scale = 1.0f;
    m_dimensions_in = 1;
    m_dimensions_out = 1;
	m_in_file = NULL;
	m_out_file = NULL;
}

lcms_transform_sampler::~lcms_transform_sampler()
{
    if (m_hTransform)
    {
        cmsDeleteTransform(m_hTransform);
    }
    
    if (m_hInProfile)
    {
        cmsCloseProfile(m_hInProfile);
    }
    
    if (m_hOutProfile)
    {
        cmsCloseProfile(m_hOutProfile);
    }

	if (m_in_file)
	{
		fclose( m_in_file );
	}

	if (m_out_file)
	{
		fclose( m_out_file );
	}
}

//
// Build the transform
//
void lcms_transform_sampler::open( const TCHAR *file_in, const TCHAR *file_out, int intent, int flags )
{
	m_hInProfile =  openProfile(file_in, m_in_file );
    m_hOutProfile = openProfile(file_out, m_out_file );
    openTransform( intent, flags );
}

void lcms_transform_sampler::open( CImage& image, const TCHAR *file_out, int intent, int flags )
{
	m_hInProfile =  openProfile(image);
    m_hOutProfile = openProfile(file_out, m_out_file);
    openTransform( intent, flags );
}

void lcms_transform_sampler::open( const TCHAR *file_in, CImage &image, int intent, int flags )
{
	m_hInProfile =  openProfile(file_in, m_in_file);
    m_hOutProfile = openProfile(image);
    openTransform( intent, flags );
}

void lcms_transform_sampler::open( CImage& image_src, CImage& image_dst, int intent, int flags)
{
	m_hInProfile =  openProfile(image_src);
    m_hOutProfile = openProfile(image_dst);
    openTransform( intent, flags );
}

//
// Build the transform from a devicelink profile
//
void lcms_transform_sampler::open( const TCHAR* devicelink_file, int intent, int flags )
{
	m_hInProfile =  openProfile(devicelink_file, m_in_file );
    m_hOutProfile = NULL;
    openTransform( intent, flags );
}

void lcms_transform_sampler::openTransform( int intent, int flags )
{
    // By forcing LCMS to use the same number of points in it's LUT grid
    // as we do, we ensure that there is no interpolation when the
    // grid is sampled.
    flags |= cmsFLAGS_GRIDPOINTS( TRANSFORM_AXIS_SIZE );
    
    // Open the profile
    if (m_hInProfile && m_hOutProfile)
    {
        // We have an input and an output profile - it is not a devicelink profile
        m_hTransform = cmsCreateTransform(m_hInProfile, getLCMSType( cmsGetColorSpace(m_hInProfile) ),
                                    m_hOutProfile, getLCMSType( cmsGetColorSpace(m_hOutProfile) ),
                                    intent, flags );
    }
    else
    {
        // There is no output profile - assume a devicelink profile
        m_hTransform = cmsCreateMultiprofileTransform(&m_hInProfile, 1, getLCMSType( cmsGetColorSpace(m_hInProfile) ),
                                          getLCMSType( cmsGetColorSpace(m_hOutProfile) ), intent, flags );
    }
    
    if (!m_hTransform)
    {
        throw jett_exception(JETT_LCMS_FAILURE,0, "Could not create colour transform");
    }
    
    // Set the scaling factors
    m_in_scale = getLCMSScale(cmsGetColorSpace(m_hInProfile));
    m_out_scale = getLCMSScale(cmsGetColorSpace(m_hOutProfile));
    
    // Set the dimensions
    m_dimensions_in = cmsChannelsOf(cmsGetColorSpace(m_hInProfile));
    m_dimensions_out = cmsChannelsOf(cmsGetColorSpace(m_hOutProfile));
}



cmsHPROFILE lcms_transform_sampler::openProfile( const TCHAR *filename, FILE* &file )
{
    // Is this a standard profile?
    cmsHPROFILE hProfile = createStandardProfile( filename );

    // Otherwise, try and load the file
    if (!hProfile)
    {

#ifdef _WIN32
	    file = NULL;
		_tfopen_s( &file, filename,_T("rb"));
		if ( !file )
		{
            throw jett_exception(JETT_LCMS_FAILURE, 0, "Could not load input profile");
		}

		hProfile =  cmsOpenProfileFromStream( file, "r" );

        if (!hProfile)
        {
            throw jett_exception(JETT_LCMS_FAILURE, 0, "Could not load input profile");
        }
#else
        file = NULL;
        hProfile = cmsOpenProfileFromFile(filename, "r");
        if (!hProfile)
        {
            throw jett_exception(JETT_LCMS_FAILURE, 0, "Could not load input profile");
        }
#endif
    }
    
    return hProfile;
}

//
// Create an in-memory standard profile
//
cmsHPROFILE lcms_transform_sampler::createStandardProfile( const TCHAR *file_in )
{
    cmsHPROFILE hProfile = NULL;
    
    if (strcasecmp(file_in,_T(":srgb")) == 0)
    {
        hProfile = cmsCreate_sRGBProfile();
    }
    else if (strcasecmp(file_in,_T(":mono")) == 0)
    {
        cmsToneCurve* Gamma = cmsBuildGamma(NULL, 2.2);
        cmsHPROFILE hProfile = cmsCreateGrayProfile(cmsD50_xyY(), Gamma);
        cmsFreeToneCurve(Gamma);
        return hProfile;
    }
    else if (strcasecmp(file_in,_T(":lab")) == 0)
    {
        hProfile = cmsCreateLab2Profile( NULL );
    }

    return hProfile;
}

cmsHPROFILE lcms_transform_sampler::openProfile( CImage &image )
{
    cmsHPROFILE hProfile = NULL;
    
    if (image.get_profile_size() == 0)
    {
        throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Image does not have an embedded profile");
    }

    hProfile =  cmsOpenProfileFromMem(image.get_profile_data(), static_cast<cmsUInt32Number>(image.get_profile_size()) );
    if (!hProfile)
    {
        throw jett_exception(JETT_LCMS_FAILURE, 0, "Could not load image profile");
    }

    return hProfile;
}




int lcms_transform_sampler::getLCMSType( cmsColorSpaceSignature colourspace )
{
    switch (colourspace)
    {
        case cmsSigLabData:
            return TYPE_Lab_8;
        case cmsSigRgbData:     
            return TYPE_RGB_FLT;
        case cmsSigGrayData:
            return TYPE_GRAY_FLT;
        case cmsSigCmykData:
            return TYPE_CMYK_FLT;

        // Unsupported
        case cmsSigXYZData:     
        case cmsSigLuvData:     
        case cmsSigYCbCrData:   
        case cmsSigCmyData:     
        case cmsSigYxyData:     
        case cmsSigHsvData:     
        case cmsSigHlsData:     
        default:
            throw jett_exception( JETT_INVALID_ARGUMENT, 0, "Unsupported colour space in ICC profile" );
            break;
    }
    
}

// Determine the LCMS scaling from the profile type
float lcms_transform_sampler::getLCMSScale( cmsColorSpaceSignature colourspace )
{
    switch (colourspace)
    {
        case cmsSigLabData:
            return 0.0f;
        case cmsSigRgbData:     
            return 1.0f;
        case cmsSigGrayData:
            return 1.0f;
        case cmsSigCmykData:
            return 1.0f/100.0f;
            
            // Unsupported
        case cmsSigXYZData:     
        case cmsSigLuvData:     
        case cmsSigYCbCrData:   
        case cmsSigCmyData:     
        case cmsSigYxyData:     
        case cmsSigHsvData:     
        case cmsSigHlsData:     
        default:
            throw jett_exception( JETT_INVALID_ARGUMENT, 0, "Unsupported colour space in ICC profile" );
            break;
    }
    
}



//
// Determine the transform properties
//
int lcms_transform_sampler::dimensions_in()
{
    return m_dimensions_in;
}

int lcms_transform_sampler::dimensions_out()
{
    return m_dimensions_out;    
}

// 
// Perform a colour management operation
// 
void lcms_transform_sampler::sample( float *sample_in, float *sample_out )
{
    if (m_in_scale == 0)
    {
        // Use 8 bpp
        unsigned char sample_in_b[ 8 ];
        sample_in_b[0] = static_cast<unsigned char>(sample_in[0] * 255.0);
        sample_in_b[1] = static_cast<unsigned char>((sample_in[1] - 0.5) * 255.0);
        sample_in_b[2] = static_cast<unsigned char>((sample_in[2] - 0.5) * 255.0);
        
        cmsDoTransform(m_hTransform, sample_in_b, sample_out, 1);
    }
    else
    {
        float sample_in_scaled[ 8 ];
        for (int i = 0; i < m_dimensions_in; ++i)
        {
            sample_in_scaled[i] = sample_in[i] / m_in_scale;
        }

        cmsDoTransform(m_hTransform, sample_in_scaled, sample_out, 1);
    }
    
    if (m_out_scale == 0)
    {
        unsigned char sample_out_b[8];
        memcpy(sample_out_b, sample_out, m_dimensions_out );
        
        for (int i = 0; i < m_dimensions_out; ++i)
        {
            sample_out[i] = sample_out_b[i] / 255.0f;
        }        
    }
    else
    {
        for (int i = 0; i < m_dimensions_out; ++i)
        {
            sample_out[i] = sample_out[i] * m_out_scale;
        }
    }
    
}

