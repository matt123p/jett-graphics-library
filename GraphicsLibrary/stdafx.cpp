//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

#ifdef __MACH__
int _tfopen_s( FILE** pFile, const char *filename, const char *mode )
{
    *pFile = fopen( filename, mode );
    return 0;
}
#endif
