//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#ifndef GraphicsLibrary_CPUDither_h
#define GraphicsLibrary_CPUDither_h

#include "Processor.h"

//
// Dither an image using the ordered screen
//

class COrderedScreenCollection;
class CImage;

class CCPUDither : public CDither
{
public:
	CCPUDither(void);
	virtual ~CCPUDither(void);

	virtual void dither( CImage *src_image, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale );
	virtual void dither( CImage *src_image, CImage** dst_images, COrderedScreenCollection& screens, CLinearizationCollection* linearization, int scale );
};

#endif
