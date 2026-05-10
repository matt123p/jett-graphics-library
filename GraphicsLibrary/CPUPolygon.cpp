//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#include "StdAfx.h"
#include "CPUPolygon.h"
#include "Image.h"
#include "transform.h"
#include "k_only_transform_sampler.h"

CCPUPolygon::CCPUPolygon(CCPUProcessor* cl, int oversample)
{
    m_cl = cl;
    m_oversample = oversample;
}

CCPUPolygon::~CCPUPolygon()
{
    
}

void CCPUPolygon::fill( CImage *dst_image, unsigned char *colour, jett_point *points, int n )
{
    // Choose the right algorithm to do the work
    if (m_oversample > 1)
    {
        fill_oversample(dst_image, colour, points, n);
    }
    else
    {
        fill_quick(dst_image, colour, points, n);
    }
    
}

template<class dst_t> void _fill_quick( CImage *dst_image, unsigned char *colour, jett_point *points, int n )
{

	int image_top = dst_image->getHeight() - 1;
    int image_bottom = 0;
    
    dst_t* img = (dst_t*)dst_image->lockData( false );
    int dst_stride = static_cast<int>(dst_image->getStride() / sizeof(dst_t));
    size_t pixel_stride = dst_image->getPixelStride();
    
    // Scan for image top and image bottom
    for (int i=0; i<n; i++)
    {
        image_top = std::min( image_top, static_cast<int>(points[i].y));
        image_bottom = std::max( image_bottom, static_cast<int>(points[i].y + 0.999f)+1);
    }

    int clip_x1, clip_y1, clip_x2, clip_y2;
    dst_image->clipping( clip_x1, clip_y1, clip_x2, clip_y2 );
    
    image_top = std::max( clip_y1, image_top );
    image_bottom = std::min( image_bottom, clip_y2);
    
    //  Loop through the rows of the image.
    for (int pixelY=image_top; pixelY<=image_bottom; pixelY++)
    {
        int nodes;
        int nodeX[ 256 ];

        //  Build a list of nodes.
        nodes=0;
        int j=n-1;
        
        for (int i=0; i<n; i++)
        {
			// Any nodes actually on the image top also count...
            if (    ((points[i].y<(float) pixelY && points[j].y>=(float) pixelY)
                ||  (points[j].y<(float) pixelY && points[i].y>=(float) pixelY)
				||  ((pixelY == image_top && (points[j].y == pixelY || points[i].y== pixelY)) && points[j].y != points[i].y)) )
            {
                float numerator = static_cast<float>(pixelY) - points[i].y;
                float denominator = points[j].y - points[i].y;
                float span = points[j].x - points[i].x;
                volatile float f_exact = points[i].x + (numerator / denominator) * span;
                float f = f_exact;
				if (f >= 0)
				{
					nodeX[nodes++]=(int) ( f + 0.5f );
				}
				else
				{
					nodeX[nodes++]=(int) ( f - 0.5f );
				}
            }
            j=i;
        }
        
        //  Sort the nodes, via a simple “Bubble” sort.
        {
            int i=0;
            while (i<nodes-1)
            {
                if (nodeX[i]>nodeX[i+1])
                {
                    int swap=nodeX[i];
                    nodeX[i]=nodeX[i+1];
                    nodeX[i+1]=swap;
                    
                    if (i)
                        i--;
                }
                else
                {
                    i++;
                }
            }
        }
        //  Fill the pixels between node pairs.
        for (int i=0; i<nodes-1; i+=2)
        {
            if   (nodeX[i]>clip_x2)
                break;
            if   (nodeX[i+1]>=clip_x1 )
            {
                if (nodeX[i]< clip_x1 )
                    nodeX[i]=clip_x1;
                if (nodeX[i+1]>clip_x2)
                    nodeX[i+1]=clip_x2;
                
                dst_t *p = img + pixelY * dst_stride + nodeX[i] * pixel_stride;
                for (j=nodeX[i]; j<=nodeX[i+1]; j++)
                {
                    pixel_convert<dst_t,unsigned char>(p, colour, pixel_stride );
                    p += pixel_stride;
                }
            }
        }
    }
    
    // Release the image
    dst_image->unlockData();
}

void CCPUPolygon::fill_quick( CImage *dst_image, unsigned char *colour, jett_point *points, int n )
{
    if (dst_image->is16bpp())
    {
        _fill_quick<unsigned short>( dst_image, colour, points, n );
    }
    else
    {
        _fill_quick<unsigned char>( dst_image, colour, points, n );
    }
}

struct nodes_t
{
    int nodes;
    int nodeX[ 256 ];
    int acc;
    int i;
    int plot;
    
    nodes_t()
    {
        nodes = 0;
        acc = 0;
        i = 0;
        plot = 0;
    }
};

inline int clamp( int v, int min_v, int max_v )
{
    if (v < min_v)
    {
        return min_v;
    }
    if (v > max_v)
    {
        return max_v;
    }
    return v;
}


template<class dst_t, int dst_max> void _fill_oversample( CImage *dst_image, unsigned char *colour, jett_point *points, int number_of_corners, int oversample )
{
    
	int image_top = dst_image->getHeight() - 1;
    int image_bottom = 0;
	int image_left = dst_image->getWidth() - 1;
	int image_right = 0;
    
    dst_t* img = (dst_t*)dst_image->lockData( false );
    int dst_stride = static_cast<int>(dst_image->getStride() / sizeof(dst_t));
    size_t pixel_stride = dst_image->getPixelStride();
   
    // Get the clipping rect
    int clip_x1, clip_y1, clip_x2, clip_y2;
    dst_image->clipping( clip_x1, clip_y1, clip_x2, clip_y2 );
	clip_x1 *= oversample;
    clip_x2 = clip_x2 * oversample + (oversample - 1);
	clip_y1 *= oversample;
	clip_y2 *= oversample;

    // We need to pre-parse the polygon data

	// TODO: use boost auto_ptr
    float *polyX = new float[ number_of_corners ];
    float *polyY = new float[ number_of_corners ];
    for (int i = 0; i < number_of_corners; ++ i)
    {
        polyX[i] = points[i].x * oversample;
        polyY[i] = points[i].y * oversample;
		image_top = std::min( image_top, static_cast<int>(polyY[i]));
		image_bottom = std::max( image_bottom, static_cast<int>(polyY[i]));
		image_left = std::min( image_left, static_cast<int>(polyX[i]));
		image_right = std::max( image_right, static_cast<int>(polyX[i]));
    }
        
	// Is there any work to do?
	// (does the polygon fall outside of the clip rectangle).
	if (image_top > clip_y2 || image_bottom < clip_y1 || image_left > clip_x2 || image_right < clip_x1)
	{
		return;
	}

	// Apply the clipping rectangle
	image_top = std::max( clip_y1, image_top );
	image_bottom = std::min( clip_y2, image_bottom );
    int adjust = std::max(0,oversample - 1);
    image_left = std::max( clip_x1, image_left );
    image_right = std::min( clip_x2, image_right );
    image_left = (image_left / oversample) * oversample;
    image_right = (image_right / oversample) * oversample + adjust;
    int y1 = image_top / oversample;
    int y2 = image_bottom / oversample;
    int x1 = image_left / oversample;
    
    dst_t dst_hold[8];
    pixel_convert<dst_t,unsigned char>(dst_hold, colour, pixel_stride );

    
    //  Loop through the rows of the image.
#ifdef _OPENMP
    bool use_openmp = (image_bottom - image_top) > OPENMP_CUTOFF;
#endif
#pragma omp parallel for if (use_openmp)
    for (int y_in=y1; y_in<=y2; y_in++)
    {
        nodes_t ndx_store[ 4 ];
        
        //  Build a list of nodes.
        for (int id = 0; id < oversample; ++ id)
        {
            int y = y_in * oversample + id;
            nodes_t *ndx = ndx_store + id;
            ndx->nodes=0;
            ndx->i = 0;
            ndx->acc = 0;
            ndx->plot = 0;
            int j=number_of_corners-1;
            for (int i=0; i<number_of_corners; i++)
            {
                // Any nodes actually on the image top also count...
                if (    ((polyY[i]<y && polyY[j]>=y)
                         ||  (polyY[j]<y && polyY[i]>=y)
                         ||  ((y == image_top && (polyY[j] == y || polyY[i]== y)) && polyY[j] != polyY[i])) )
                {
                    float numerator = static_cast<float>(y) - polyY[i];
                    float denominator = polyY[j] - polyY[i];
                    float span = polyX[j] - polyX[i];
                    volatile float f_exact = polyX[i] + (numerator / denominator) * span;
                    float f = f_exact;
                    if (f >= 0)
                    {
                        ndx->nodeX[ndx->nodes++]=(int)( f + 0.5f );
                    }
                    else
                    {
                        ndx->nodeX[ndx->nodes++]=(int)( f - 0.5f );
                    }
                }
                j=i;
            }
            
            //  Sort the nodes, via a simple “Bubble” sort.
            {
                int i=0;
                while (i<ndx->nodes-1)
                {
                    if (ndx->nodeX[i]>ndx->nodeX[i+1])
                    {
                        int swap=ndx->nodeX[i];
                        ndx->nodeX[i]=ndx->nodeX[i+1];
                        ndx->nodeX[i+1]=swap;
                        
                        if (i)
                            i--;
                    }
                    else
                    {
                        i++;
                    }
                }
            }
        }
        
        
        int cc = 0;
        int oversample2 = oversample*oversample;
        dst_t *p = img + y_in * dst_stride + x1 * pixel_stride;
        for (int x = image_left; x <= image_right; ++ x)
        {
            for (int id = 0; id < oversample; ++ id)
            {
                nodes_t *ndx = ndx_store + id;
                
                // Have we crossed the node boundry?
                if (ndx->i < ndx->nodes && !ndx->plot && x >= ndx->nodeX[ndx->i])
                {
                    ndx->plot = 1 - ndx->plot;
                    ++ ndx->i;
                }
                else if (ndx->i < ndx->nodes && ndx->plot && x > ndx->nodeX[ndx->i] + adjust)
                {
                    ndx->plot = 1 - ndx->plot;
                    ++ ndx->i;
                }
                
                ndx->acc += ndx->plot;
            }
            
            ++ cc;
            if (cc == oversample)
            {
                cc = 0;
                int acc = 0;
                for (int id = 0; id < oversample; ++ id)
                {
                    acc += ndx_store[id].acc;
                }
                
                int acc2 = oversample2 - acc;
                switch (pixel_stride)
                {
                    case 1:
                        p[0] = clamp((int)(p[0] * acc2 + dst_hold[0] * acc) / oversample2,0, dst_max);
                        break;
                    case 3:
                        p[2] = clamp((int)(p[2] * acc2 + dst_hold[2] * acc) / oversample2,0, dst_max);
                        p[1] = clamp((int)(p[1] * acc2 + dst_hold[1] * acc) / oversample2,0, dst_max);
                        p[0] = clamp((int)(p[0] * acc2 + dst_hold[0] * acc) / oversample2,0, dst_max);
                        break;
                    case 4:
                        p[3] = clamp((int)(p[3] * acc2 + dst_hold[3] * acc) / oversample2,0, dst_max);
                        p[2] = clamp((int)(p[2] * acc2 + dst_hold[2] * acc) / oversample2,0, dst_max);
                        p[1] = clamp((int)(p[1] * acc2 + dst_hold[1] * acc) / oversample2,0, dst_max);
                        p[0] = clamp((int)(p[0] * acc2 + dst_hold[0] * acc) / oversample2,0, dst_max);
                        break;
                }
                p += pixel_stride;
                
                for (int id = 0; id < oversample; ++ id)
                {
                    ndx_store[id].acc = 0;
                }
            }
        }
    }
    
    // Cleanup
    delete[] polyX;
    delete[] polyY;
}

void CCPUPolygon::fill_oversample( CImage *dst_image, unsigned char *colour, jett_point *points, int number_of_corners )
{
    if (dst_image->is16bpp())
    {
        _fill_oversample<unsigned short, 65535>( dst_image, colour, points, number_of_corners, m_oversample );
    }
    else
    {
        _fill_oversample<unsigned char,255>( dst_image, colour, points, number_of_corners, m_oversample );
    }
}

