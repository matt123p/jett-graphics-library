#include "StdAfx.h"
#include "Processor.h"
#include "Image.h"



CBitblt::CBitblt()
{
}

CBitblt::~CBitblt()
{
}


// Utility functions
bool CBitblt::clip( bitblt_params &params, CImage *src_bitmap, CImage* dst_bitmap,  bool ignore_src_dims )
{
	//
	// Get the clipping rectangle
	//
	int clip_x1, clip_y1, clip_x2, clip_y2;
	dst_bitmap->clipping( clip_x1, clip_y1, clip_x2, clip_y2 );


	// Extract the src data
	int src_x2, src_y2;
	if ((params.flags & perform_rotate90) != 0)
	{
		// 90 degree rotation clockwise
		src_x2 = params.src_y + static_cast<int>((params.dst_y2 - params.dst_y1) * params.scale_src_x);
		src_y2 = params.src_x + static_cast<int>((params.dst_x2 - params.dst_x1) * params.scale_src_y);
	}
	else if ((params.flags & perform_rotate180) != 0)
	{
		// 180 degree rotation clockwise
		src_x2 = params.src_x + static_cast<int>((params.dst_x2 - params.dst_x1) * params.scale_src_x);
		src_y2 = params.src_y + static_cast<int>((params.dst_y2 - params.dst_y1) * params.scale_src_y);
	}
	else if ((params.flags & perform_rotate270) != 0)
	{
		// 270 degree rotation clockwise
		src_x2 = params.src_y + static_cast<int>((params.dst_y2 - params.dst_y1) * params.scale_src_x);
		src_y2 = params.src_x + static_cast<int>((params.dst_x2 - params.dst_x1) * params.scale_src_y);
	}
	else
	{
		// No rotation
		src_x2 = params.src_x + static_cast<int>((params.dst_x2 - params.dst_x1) * params.scale_src_x);
		src_y2 = params.src_y + static_cast<int>((params.dst_y2 - params.dst_y1) * params.scale_src_y);
	}



	//
    // Now range check the output values
    //
    if (params.dst_x1 < clip_x1)
    {
        int delta = clip_x1-params.dst_x1;
        params.dst_x1 += delta;

        if ((params.flags & perform_rotate90) != 0)
        {
            // 90 degree rotation clockwise
			src_y2 -= static_cast<int>(delta * params.scale_src_y);
        }
        else if ((params.flags & perform_rotate180) != 0)
        {
            // 180 degree rotation clockwise
			src_x2 -= static_cast<int>(delta * params.scale_src_x);
        }
        else if ((params.flags & perform_rotate270) != 0)
        {
            // 270 degree rotation clockwise
			params.src_y += static_cast<int>(delta * params.scale_src_y);
        }
        else
        {
            // No rotation
	        params.src_x += static_cast<int>(delta * params.scale_src_x);
        }
    }
    if (params.dst_x2 < clip_x1)
    {
        // This is not going to result in any work...
        return false;
    }
    if (params.dst_x1 >= clip_x2)
    {
        // This is not going to result in any work...
        return false;
    }
    if (params.dst_x2 > clip_x2)
    {
        int delta = clip_x2 - params.dst_x2;
        params.dst_x2 += delta;
        if ((params.flags & perform_rotate90) != 0)
        {
            // 90 degree rotation clockwise
			params.src_y -= static_cast<int>(delta * params.scale_src_y);
        }
        else if ((params.flags & perform_rotate180) != 0)
        {
            // 180 degree rotation clockwise
			params.src_x -= static_cast<int>(delta * params.scale_src_x);
        }
        else if ((params.flags & perform_rotate270) != 0)
        {
            // 270 degree rotation clockwise
			src_y2 += static_cast<int>(delta * params.scale_src_y);
        }
        else
        {
            // No rotation
			src_x2 += static_cast<int>(delta * params.scale_src_x);
        }
    }
    if (params.dst_y1 < clip_y1)
    {
        int delta = clip_y1-params.dst_y1;
        params.dst_y1 += delta;
        if ((params.flags & perform_rotate90) != 0)
        {
            // 90 degree rotation clockwise
			params.src_x += static_cast<int>(delta * params.scale_src_x);
        }
        else if ((params.flags & perform_rotate180) != 0)
        {
            // 180 degree rotation clockwise
			src_y2 -= static_cast<int>(delta * params.scale_src_y);
        }
        else if ((params.flags & perform_rotate270) != 0)
        {
            // 270 degree rotation clockwise
	        src_x2 -= static_cast<int>(delta * params.scale_src_x);
        }
        else
        {
            // No rotation
	        params.src_y += static_cast<int>(delta * params.scale_src_y);
        }
    }
    if (params.dst_y2 < clip_y1)
    {
        // This is not going to result in any work...
        return false;
    }
    if (params.dst_y1 > clip_y2)
    {
        // This is not going to result in any work...
        return false;
    }
    if (params.dst_y2 > clip_y2)
    {
		int delta = clip_y2 - params.dst_y2;
        params.dst_y2 += delta;
        if ((params.flags & perform_rotate90) != 0)
        {
            // 90 degree rotation clockwise
			src_x2 += static_cast<int>(delta * params.scale_src_x);
        }
        else if ((params.flags & perform_rotate180) != 0)
        {
            // 180 degree rotation clockwise
			params.src_y -= static_cast<int>(delta * params.scale_src_y);
        }
        else if ((params.flags & perform_rotate270) != 0)
        {
            // 270 degree rotation clockwise
            params.src_x -= static_cast<int>(delta * params.scale_src_x);
        }
        else
        {
            // No rotation
            src_y2 += static_cast<int>(delta * params.scale_src_y);
        }
    }
    
    if (   params.dst_x1 > params.dst_x2
        || params.dst_y1 > params.dst_y2)
    {
        // This is not going to result in any work...
        return false;        
    }

	//
    // Now range check the input values
    //
	if (!ignore_src_dims)
	{
		if (params.src_x < 0)
		{
			int delta = -params.src_x;
			params.src_x += delta;
			if ((params.flags & perform_rotate90) != 0)
			{
				// 90 degree rotation clockwise
				params.dst_y2 -= static_cast<int>(delta / params.scale_src_y);
			}
			else if ((params.flags & perform_rotate180) != 0)
			{
				// 180 degree rotation clockwise
				params.dst_x2 -= static_cast<int>(delta / params.scale_src_x);
			}
			else if ((params.flags & perform_rotate270) != 0)
			{
				// 270 degree rotation clockwise
				params.dst_y1 += static_cast<int>(delta / params.scale_src_y);
			}
			else
			{
				// No rotation
				params.dst_x1 += static_cast<int>(delta / params.scale_src_x);            
			}
		}
		if (params.src_y < 0)
		{
			int delta = -params.src_y;
			params.src_y += delta;
			if ((params.flags & perform_rotate90) != 0)
			{
				// 90 degree rotation clockwise
				params.dst_x2 -= static_cast<int>(delta / params.scale_src_x);
			}
			else if ((params.flags & perform_rotate180) != 0)
			{
				// 180 degree rotation clockwise
				params.dst_y2 -= static_cast<int>(delta / params.scale_src_y);
			}
			else if ((params.flags & perform_rotate270) != 0)
			{
				// 270 degree rotation clockwise
				params.dst_x1 += static_cast<int>(delta / params.scale_src_x);
			}
			else
			{
				// No rotation
				params.dst_y1 += static_cast<int>(delta / params.scale_src_y);
			}
		}

		if (src_x2 >= src_bitmap->getWidth() )
		{
			int delta = src_bitmap->getWidth() - src_x2 - 1;
			src_x2 += delta;
			if ((params.flags & perform_rotate90) != 0)
			{
				// 90 degree rotation clockwise
				params.dst_y1 -= static_cast<int>(delta / params.scale_src_y);
			}
			else if ((params.flags & perform_rotate180) != 0)
			{
				// 180 degree rotation clockwise
				params.dst_x1 -= static_cast<int>(delta / params.scale_src_x);
			}
			else if ((params.flags & perform_rotate270) != 0)
			{
				// 270 degree rotation clockwise
				params.dst_y2 += static_cast<int>(delta / params.scale_src_y);
			}
			else
			{
				// No rotation
				params.dst_x2 += static_cast<int>(delta / params.scale_src_x);
			}
		}
		if (src_y2 >= src_bitmap->getHeight() )
		{
			int delta = src_bitmap->getHeight() - src_y2 - 1;
			src_y2 += delta;
			if ((params.flags & perform_rotate90) != 0)
			{
				// 90 degree rotation clockwise
				params.dst_x1 -= static_cast<int>(delta / params.scale_src_x);
			}
			else if ((params.flags & perform_rotate180) != 0)
			{
				// 180 degree rotation clockwise
				params.dst_y1 -= static_cast<int>(delta / params.scale_src_y);
			}
			else if ((params.flags & perform_rotate270) != 0)
			{
				// 270 degree rotation clockwise
				params.dst_x2 += static_cast<int>(delta / params.scale_src_x);
			}
			else
			{
				// No rotation
				params.dst_y2 += static_cast<int>(delta / params.scale_src_y);
			}
		}
    
    
		if ((params.flags & perform_rotate90) != 0)
		{
			// 90 degree rotation clockwise
			params.scale_src_x = static_cast<float>(src_x2 - params.src_x) / (params.dst_y2 - params.dst_y1);
			params.scale_src_y = static_cast<float>(src_y2 - params.src_y) / (params.dst_x2 - params.dst_x1);
		}
		else if ((params.flags & perform_rotate180) != 0)
		{
			// 180 degree rotation clockwise
			params.scale_src_y = static_cast<float>(src_y2 - params.src_y) / (params.dst_y2 - params.dst_y1);
			params.scale_src_x = static_cast<float>(src_x2 - params.src_x) / (params.dst_x2 - params.dst_x1);
		}
		else if ((params.flags & perform_rotate270) != 0)
		{
			// 270 degree rotation clockwise
			params.scale_src_x = static_cast<float>(src_x2 - params.src_x) / (params.dst_y2 - params.dst_y1);
			params.scale_src_y = static_cast<float>(src_y2 - params.src_y) / (params.dst_x2 - params.dst_x1);
		}
		else
		{
			// No rotation
			params.scale_src_y = static_cast<float>(src_y2 - params.src_y) / (params.dst_y2 - params.dst_y1);
			params.scale_src_x = static_cast<float>(src_x2 - params.src_x) / (params.dst_x2 - params.dst_x1);
		}
	}

	return true;
}
