//
//  Linearization.h
//  GraphicsLibrary
//
//  Created by Matt Pyne on 13/02/2013.
//
//

#ifndef __GraphicsLibrary__Linearization__
#define __GraphicsLibrary__Linearization__

struct jett_point;

//
// This class encapsulates a 256 level array which is used
// to linearize an image
//
class CLinearization
{
private:
    // Here is the actual linearization table
    unsigned char m_linearization[ 256 ];
    
public:
    
    // Default constructor (makes a identity linearization)
    CLinearization();
    
    // Import linearization from a set of points
    void Import( jett_point* curve, size_t points, int options );
        
    // Return the actual data
    unsigned char* get_linearization();
};


#endif /* defined(__GraphicsLibrary__Linearization__) */
