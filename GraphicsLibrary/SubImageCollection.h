//
//  SubImageCollection.h
//  GraphicsLibrary
//
//  Created by Matt Pyne on 21/07/2013.
//
//

#ifndef GraphicsLibrary_SubImageCollection_h
#define GraphicsLibrary_SubImageCollection_h

#include "Image.h"

class CSubImageCollection
{
private:
    typedef std::map<int,subimage_ptr> subimageCollection;
    subimageCollection  m_sub_images;
    
public:
    // Construction/Destruction
    CSubImageCollection();
    ~CSubImageCollection();
    
    // Initialization
    void create( CImage *pImage, int y0, int y1 );
    
    // Extract a sub-image which contains the y-offset
    subimage_ptr getSubImage( int y );
};



#endif
