//
//  SubImageCollection.cpp
//  GraphicsLibrary
//
//  Created by Matt Pyne on 21/07/2013.
//
//

#include "StdAfx.h"
#include "SubImageCollection.h"
#include "Image.h"
#include "SubImage.h"


// Construction
CSubImageCollection::CSubImageCollection()
{
    
}

CSubImageCollection::~CSubImageCollection()
{
    
}


// Initialization
void CSubImageCollection::create( CImage *pImage, int y0, int y1 )
{
    m_sub_images.erase( m_sub_images.begin(), m_sub_images.end() );

    for (int index=0; index < pImage->getNumberSubImages(); ++ index)
    {
        subimage_ptr p( pImage->getSubImage(index));
        int sub_y0 = p->getStartY();
        int sub_y1 = sub_y0 + p->getHeight() - 1;
        
        // Is it NOT out of range?
        if (!(sub_y1 < y0 || sub_y0 > y1))
        {
            // In range
            m_sub_images[ sub_y0 ] = p;
        }
    }
}

subimage_ptr CSubImageCollection::getSubImage( int y )
{
    subimage_ptr r;
    
    subimageCollection::iterator i = m_sub_images.lower_bound(y);
    if (i != m_sub_images.end())
    {
        int sub_y0 = i->second->getStartY();
        int sub_y1 = sub_y0 + i->second->getHeight() - 1;
        if (y >= sub_y0 && y <= sub_y1)
        {
            r = i->second;
        }
    }
    
    return r;
}

