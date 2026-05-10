static const unsigned char Polygon_cl[] = R"CLC(
//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//


#pragma OPENCL FP_CONTRACT OFF


//  NOTE: public-domain code by Darel Rex Finley, 2007



__kernel void fill(  
                     __global unsigned char *dst_bitmap,
                     int dst_stride,
                     int dst_pixel_stride,
                     int dst_x1,
                     int dst_y1,
                     int dst_x2,
                     int dst_y2,
                     int top_edge_y,
                     unsigned int colour,
                     int dst_width,
                     int number_of_corners,
                     __constant const float* polyX,
                     __constant const float* polyY,
                     __local int *nodeX_in )
{
    //
    // group 0 = x pixel
    // group 1 = colour
    //
    int workgroup_size = get_local_size(0);
    __local int *nodeX = nodeX_in + 32 * get_local_id(0);

    for (int y = dst_y1 + get_global_id( 0 ); y <= dst_y2; y += get_global_size(0) )
    {
        //  Build a list of nodes.
        int nodes=0;
        int j=number_of_corners-1;
        for (int i=0; i<number_of_corners; i++)
        {
            // Any nodes actually on the image top also count...
            if (    ((polyY[i]<y && polyY[j]>=y)
                ||  (polyY[j]<y && polyY[i]>=y)
                ||  (y == top_edge_y && (polyY[j] == y || polyY[i]== y)) && polyY[j] != polyY[i]) )
            {
                float y_value = (float)y;
                float numerator = y_value - polyY[i];
                float denominator = polyY[j] - polyY[i];
                float span = polyX[j] - polyX[i];
                float ratio = numerator / denominator;
                float f = polyX[i] + ratio * span;
                if (f >= 0)
                {
                    nodeX[nodes++]=(int)( f + 0.5f );
                }
                else
                {
                    nodeX[nodes++]=(int)( f - 0.5f );
                }
            }
            j=i;
        }

        // Sort the nodes, via a simple �Bubble� sort.
        // This is normally fine as we will never have that many
        // nodes per row
        int i=0;
        while (i<nodes-1)
        {
            if (nodeX[i]>nodeX[i+1])
            {
                int swap=nodeX[i];
                nodeX[i]=nodeX[i+1];
                nodeX[i+1]=swap;

                if (i)
                {
                    i--;
                }
            }
            else
            {
                i++;
            }
        }

        barrier(CLK_LOCAL_MEM_FENCE);


        //  Fill the pixels between node pairs.
        for (int i=0; i<nodes-1; i+=2)
        {
            if (nodeX[i]>dst_x2)
                break;

            if (nodeX[i+1]>=dst_x1) 
            {
                if (nodeX[i]< dst_x1 )
                    nodeX[i]=dst_x1;

                if (nodeX[i+1]>dst_x2)
                    nodeX[i+1]=dst_x2;

                int x_start = nodeX[i];
                int x_end = nodeX[i+1];
                __global unsigned char *p = dst_bitmap + mad24( y, dst_stride, x_start * dst_pixel_stride );

                for (int x = x_start; x <= x_end; ++ x )
                {
                    switch (dst_pixel_stride)
                    {
                        case 1:
                            *p = colour;
                            break;
                        case 3:
                            p[2] = colour >> 16;
                            p[1] = colour >> 8;
                            p[0] = colour;
                            break;
                        case 4:
                            *(__global unsigned int*)p = colour;
                            break;
                    }
                    p += dst_pixel_stride;
                }

            }
        }
    }
}


__kernel void fill_aa(
                   __global unsigned char *dst_bitmap,
                   int dst_stride,
                   int dst_pixel_stride,
                   int dst_x1,
                   int dst_y1,
                   int dst_x2,
                   int dst_y2,
                   int top_edge_y,
                   unsigned int colour,
                   int dst_width,
                   int number_of_corners,
                   __constant const float* polyX,
                   __constant const float* polyY,
                   __local int *nodeX_in,
                   __local int   *accumulator )
{
    //
    // group 0 = y pixel
    //
    int id = get_global_id( 0 ) % OVERSAMPLE;
    int base = get_local_id(0);
    __local int *nodeX = nodeX_in + 32 * get_local_id(0);

    for (int y = dst_y1 + get_global_id( 0 ); y <= dst_y2; y += get_global_size(0) )
    {
        //  Build a list of nodes.
        int nodes=0;
        int j=number_of_corners-1;
        for (int i=0; i<number_of_corners; i++)
        {
            // Any nodes actually on the image top also count...
            if (    ((polyY[i]<y && polyY[j]>=y)
                ||  (polyY[j]<y && polyY[i]>=y)
                ||  (y == top_edge_y && (polyY[j] == y || polyY[i]== y)) && polyY[j] != polyY[i]) )
            {
                float y_value = (float)y;
                float numerator = y_value - polyY[i];
                float denominator = polyY[j] - polyY[i];
                float span = polyX[j] - polyX[i];
                float ratio = numerator / denominator;
                float f = polyX[i] + ratio * span;
                if (f >= 0)
                {
                    nodeX[nodes++]=(int)( f + 0.5f );
                }
                else
                {
                    nodeX[nodes++]=(int)( f - 0.5f );
                }
            }
            j=i;
        }

        // Sort the nodes, via a simple �Bubble� sort.
        // This is normally fine as we will never have that many
        // nodes per row
        int i=0;
        while (i<nodes-1)
        {
            if (nodeX[i]>nodeX[i+1])
            {
                int swap=nodeX[i];
                nodeX[i]=nodeX[i+1];
                nodeX[i+1]=swap;

                if (i)
                {
                    i--;
                }
            }
            else
            {
                i++;
            }
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        //  Fill the pixels between node pairs.
        int plot = 0;
        int acc = 0;
        int cc = 0;
        i = 0;
        __global unsigned char *p = dst_bitmap + mad24( y / OVERSAMPLE, dst_stride, (dst_x1 / OVERSAMPLE) * dst_pixel_stride );
        for (int x = dst_x1; x <= dst_x2; ++ x)
        {
            // Have we crossed the node boundry?
            if (i < nodes && !plot && x >= nodeX[i])
            {
                plot = 1 - plot;
                ++ i;
            }
            else if (i < nodes && plot && x > nodeX[i] + ADJUST)
            {
                plot = 1 - plot;
                ++ i;
            }

            acc += plot;

            ++ cc;
            if (cc == OVERSAMPLE)
            {
                cc = 0;
                accumulator[ base ] = acc;
                barrier(CLK_LOCAL_MEM_FENCE);

                if (id == 0)
                {
                    for (int j = 1; j < OVERSAMPLE; ++ j)
                        acc += accumulator[ base + j ];

                    int acc2 = OVERSAMPLE2 - acc;
                    switch (dst_pixel_stride)
                    {
                        case 1:
                            p[0] = clamp((int)(p[0] * acc2 + colour * acc) / OVERSAMPLE2,0, 255);
                            break;
                        case 3:
                            p[2] = clamp((int)(p[2] * acc2 + ((colour>>16)&0xff) * acc) / OVERSAMPLE2,0, 255);
                            p[1] = clamp((int)(p[1] * acc2 + ((colour>>8)&0xff) * acc) / OVERSAMPLE2,0, 255);
                            p[0] = clamp((int)(p[0] * acc2 + ((colour)&0xff) * acc) / OVERSAMPLE2,0, 255);
                            break;
                        case 4:
                            p[3] = clamp((int)(p[3] * acc2 + ((colour>>24)&0xff) * acc) / OVERSAMPLE2,0, 255);
                            p[2] = clamp((int)(p[2] * acc2 + ((colour>>16)&0xff) * acc) / OVERSAMPLE2,0, 255);
                            p[1] = clamp((int)(p[1] * acc2 + ((colour>>8)&0xff) * acc) / OVERSAMPLE2,0, 255);
                            p[0] = clamp((int)(p[0] * acc2 + ((colour)&0xff) * acc) / OVERSAMPLE2,0, 255);
                            break;
                    }
                    p += dst_pixel_stride;
                }
                acc = 0;
                barrier(CLK_LOCAL_MEM_FENCE);
            }
        }
    }
}
)CLC";