//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "Image.h"
#include "timer.h"
#include "jett.h"
#include <math.h>

void build_polygon( jett_point *points, int x, int y, int r, int innerR, int vertexCount, double startAngle );


////////////////////////////////////////////////////////////////////////////////

#ifdef __MACH__
#define EXAMPLES_DIR "/Users/matt/Documents/riverbank/Products/GraphicsLibrary/trunk/src/GraphicsLibrary/docs/example images/"
#define IMAGES_DIR "/Users/matt/Documents/riverbank/Products/GraphicsLibrary/trunk/test images/"
#define CMYK_ICC_DIR "/Users/matt/Documents/riverbank/Products/GraphicsLibrary/trunk/Adobe ICC Profiles/CMYK Profiles/"
#define RGB_ICC_DIR "/Users/matt/Documents/riverbank/Products/GraphicsLibrary/trunk/Adobe ICC Profiles/RGB Profiles/"
#define FONT_FILE "/Library/Fonts/Arial.ttf"
#define SRC_IMAGES "/Users/matt/Documents/riverbank/Products/GraphicsLibrary/trunk/src/GraphicsLibrary/docs/src pictures/"
#else
#define IMAGES_DIR "C:\\Users\\Administrator\\Desktop\\mp\\test images\\"
#define CMYK_ICC_DIR "F:\\Adobe ICC Profiles\\CMYK Profiles\\"
#define RGB_ICC_DIR "F:\\Adobe ICC Profiles\\RGB Profiles\\"
#define FONT_FILE "C:\\Windows\\Fonts\\Arial.ttf"
#endif


void build_polygon( jett_point *points, int x, int y, int r, int innerR, int vertexCount, double startAngle )
{
    double PI = asin( 1.0 ) * 2.0;
    double addAngle=2*PI/vertexCount;
    startAngle = startAngle / 360.0 * 2 * PI;
    double angle=startAngle;
    double innerAngle=startAngle+PI/vertexCount;
    for (int i=0; i<vertexCount; i++) {
        points[i*2].x=(float)(r*cos(angle))+x;
        points[i*2].y=(float)(r*sin(angle))+y;
        angle+=addAngle;
        points[i*2+1].x=(float)(innerR*cos(innerAngle))+x;
        points[i*2+1].y=(float)(innerR*sin(innerAngle))+y;
        innerAngle+=addAngle;
    }
}

void example_1(jett &r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a line
    jett_point points[ 2 ];
    points[0] = jett_point( 10,10 );
    points[1] = jett_point( 240, 10 );
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    r.lines(image, cmyk_col, 1, points, 2, false, 0 );
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "001 simple_example.tiff" );
}


void example_2(jett &r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a line
    jett_point points[ 2 ];
    points[0] = jett_point( 10,10 );
    points[1] = jett_point( 240, 240 );
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    
    // Draw a line 4 pixels wide
    r.lines(image, cmyk_col, 4, points, 2, false, 0 );
    
    points[0] = jett_point( 240,10 );
    points[1] = jett_point( 10, 240 );
    r.lines(image, cmyk_col, 4, points, 2, false, 0 );

    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "002 line_example.tiff" );
}

void example_3( jett &r )
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a U shape
    jett_point points[ 4 ];
    points[0] = jett_point( 80,25 );
    points[1] = jett_point( 35, 225 );
    points[2] = jett_point( 215, 225 );
    points[3] = jett_point( 170, 25 );
    
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    
    // Draw a line 20 pixels wide
    r.lines(image, cmyk_col, 20, points, 4, false, line_join_bevel );

    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "003 line_example bevel.tiff" );
}

void example_4( jett &r )
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a U shape
    jett_point points[ 4 ];
    points[0] = jett_point( 80,25 );
    points[1] = jett_point( 35, 225 );
    points[2] = jett_point( 215, 225 );
    points[3] = jett_point( 170, 25 );
    
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    
    // Draw a line 20 pixels wide
    r.lines(image, cmyk_col, 20, points, 4, false, line_join_miter );
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "004 line_example miter.tiff" );
}

void example_5( jett &r )
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a U shape
    jett_point points[ 4 ];
    points[0] = jett_point( 80,25 );
    points[1] = jett_point( 35, 225 );
    points[2] = jett_point( 215, 225 );
    points[3] = jett_point( 170, 25 );
    
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    
    // Draw a line 20 pixels wide
    r.lines(image, cmyk_col, 20, points, 4, false, line_join_none );
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "005 line_example none.tiff" );
}

void example_6( jett& r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a star
    jett_point points[ 12 ];
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    build_polygon(points, 125, 125, 50, 25, 6, 0 );
    r.lines(image, cmyk_col, 4, points, 12, true, line_join_miter );
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "006 line_example closed.tiff" );
}

void example_7( jett& r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a star
    jett_point points[ 12 ];
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    build_polygon(points, 125, 125, 50, 25, 6, 0 );
    r.lines(image, cmyk_col, 4, points, 12, true, line_join_miter | line_best );
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "007 line_example best.tiff" );
}

void example_8( jett & r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk, true );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a star
    jett_point points[ 12 ];
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    build_polygon(points, 125, 125, 50, 25, 6, 0 );
    r.polygon(image, cmyk_col, points, 12, polygon_best );

    // Save the bitmap for inspection
    image.saveToFile( IMAGES_DIR "008 polygon_example best 16 bit.tiff" );
}

void example_9( jett & r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk, true );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Draw a star
    jett_point points[ 12 ];
    unsigned char cmyk_col[4] = { 0,255,0,0 };  // (Magenta)
    build_polygon(points, 125, 125, 50, 25, 6, 0 );
    r.polygon(image, cmyk_col, points, 12, polygon_fast );
    
    // Save the bitmap for inspection
    image.saveToFile( IMAGES_DIR "009 polygon_example fast 16 bit.tiff" );
}

void example_10( jett & r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // We need a colour to draw with
    unsigned char colour[4] = { 192, 64, 0, 0 };
    
    // We a font to draw with
    jett_font f = r.create_font( FONT_FILE, 0, 20, true );
    
    // Draw some text
    
    CTimer t1;
    t1.start();

    r.text(f, colour, image, 125, 125, "Text at 0", string_rotate_0 );
    r.text(f, colour, image, 125, 125, "Text at 90",  string_rotate_90 );
    r.text(f, colour, image, 125, 125, "Text at 180", string_rotate_180 );
    r.text(f, colour, image, 125, 125, "Text at 270", string_rotate_270 );
    
    uint64_t ns1 = t1.elapsed();
    printf("text render %lld\n", ns1 );
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "010 text_example.tiff" );
}

void example_11( jett & r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk, true );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // We need a colour to draw with
    unsigned char colour[4] = { 192, 64, 0, 0 };
    
    // We a font to draw with
    jett_font f1 = r.create_font( FONT_FILE, 0, 20, false );
    jett_font f2 = r.create_font( FONT_FILE, 0, 20, true );
    
    // Draw some text
    r.text(f1, colour, image, 10, 50, "Text without anti-alias", string_rotate_0 );
    r.text(f2, colour, image, 10, 90, "Text with anti-alias", string_rotate_0 );
    
    
    // Save the bitmap for inspection
    image.saveToFile( IMAGES_DIR "011 text_example aa 16 bit.tiff" );
}

void example_12( jett & r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(250,250, image_cmyk );
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // We need a colour to draw with
    unsigned char colour[4] = { 192, 64, 0, 0 };
    
    // We a font to draw with
    jett_font f = r.create_font( FONT_FILE, 0, 20, true );
    
    const char *text = "Text drawn round in a circle";
    size_t len = strlen( text );
    double PI = asin( 1.0 ) * 2.0;
    
    jett_point centre( 125.0f, 125.0f );
    double angle = - PI/2;
    double radius = 50;
    jett_matrix identity;
    for (size_t i = 0; i < len; ++i)
    {
        char character[2];
        character[0] = text[i];
        character[1] = 0;
        
        // Determine the location of the start of the text
        jett_point p;
        p.x = sin(angle) * radius + centre.x;
        p.y = -cos(angle) * radius + centre.y;
        
        // Determine the width of the character
        r.set_font_matrix(f, identity );
        jett_point size = r.size_text(f, character);
        
        // Determine the angle this represents
        double delta_angle = atan2( size.x, radius );
        angle += delta_angle / 2;
        
        // Create a matrix at the correct angle to draw the text at
        jett_matrix m;
        m.rotate( angle );
        
        // Rotate the font by the angle
        r.set_font_matrix(f, m);
        
        
        // Now draw the text
        r.text(f, colour, image, p.x, p.y, character, string_rotate_0 );
        
        angle += delta_angle / 2;
    }
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "012 rotated text.tiff" );
}

void example_13( jett& r)
{
    return;
    
    // Generate the dither screens using the void-and-cluster technique
    int sizes[] = { 1024 };
    CTimer t1;
    t1.start();
    jett_screens s = r.create_screens(1, sizes,1.5,2, NULL);
    uint64_t ns1 = t1.elapsed();
    printf("Screen create %lld\n", ns1 );

    
    // Load an RGB images
    jett_image rgb_in;
    rgb_in.loadFromFile( IMAGES_DIR "Lenna.png" );
    
    // Now convert to a Monochrome image
    jett_image mono_out;
    mono_out.createImage(rgb_in.getWidth(), rgb_in.getHeight(), image_mono );
    mono_out.set_profile_data( ":mono" );
    jett_transform t = r.build_transform( ":srgb", mono_out, INTENT_PERCEPTUAL );
    r.bitblt(t, rgb_in, mono_out, 0, 0, 0 );
    
    // Create the output images (one for each colour plane)
    jett_image outputs[1];
    outputs[0].createImage( mono_out.getWidth(), mono_out.getHeight(), image_mono1 );
    
    // Perform the dithering
    r.dither( mono_out, outputs, s, NULL );
    
    // Now save the results
    outputs[0].saveToFile( EXAMPLES_DIR "013 output.bmp");
}

void example_14( jett& r)
{
    // Generate the dither screens using the void-and-cluster technique
    int sizes[] = { 32,33,34,35 };
    jett_screens s = r.create_screens(4, sizes,1.5,2,NULL);
    
    // Load an RGB images
    jett_image rgb_in;
    rgb_in.loadFromFile( IMAGES_DIR "Lenna.png" );
    
    // Now convert to a CMYK image
    jett_image cmyk_out;
    cmyk_out.createImage(rgb_in.getWidth(), rgb_in.getHeight(), image_cmyk );
    cmyk_out.set_profile_data(CMYK_ICC_DIR "USWebCoatedSWOP.icc");
    jett_transform t = r.build_transform( ":srgb", cmyk_out, INTENT_PERCEPTUAL );
    r.bitblt(t, rgb_in, cmyk_out, 0, 0, 0 );
    
    // Create the output images (one for each colour plane)
    jett_image outputs[4];
    outputs[0].createImage( cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1 );
    outputs[1].createImage( cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1 );
    outputs[2].createImage( cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1 );
    outputs[3].createImage( cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1 );
    
    // Perform the dithering
    r.dither( cmyk_out, outputs, s, NULL );
    
    // Now save the results
    outputs[0].saveToFile( EXAMPLES_DIR "014 output_C.bmp");
    outputs[1].saveToFile( EXAMPLES_DIR "014 output_M.bmp");
    outputs[2].saveToFile( EXAMPLES_DIR "014 output_Y.bmp");
    outputs[3].saveToFile( EXAMPLES_DIR "014 output_K.bmp");
    
}


void example_15(jett &r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(825,300, image_cmyk, true );
    image.set_profile_data(CMYK_ICC_DIR "USWebCoatedSWOP.icc");

    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Load some images to compose on to the background
    jett_image image_diamond, image_lilly, image_teddy;
    
    image_diamond.loadFromFile(SRC_IMAGES "Mysecret_10icons_by_Artdesigner/diamond_by_Artdesigner.lv.png");
    image_lilly.loadFromFile( SRC_IMAGES "Mysecret_10icons_by_Artdesigner/lily_by_Artdesigner.lv.png");
    image_teddy.loadFromFile( SRC_IMAGES "Mysecret_10icons_by_Artdesigner/teddy_by_Artdesigner.lv.png" );
    
    // We need a colour transform
    jett_transform t = r.build_transform( ":srgb", image, INTENT_PERCEPTUAL );
    
    // Now compose a new image
    r.bitblt(t, image_diamond, image, 25, 25, bitblt_use_alpha );
    r.bitblt(t, image_lilly, image, 300, 25, bitblt_use_alpha );
    r.bitblt(t, image_teddy, image, 550, 25, bitblt_use_alpha );
    
    
    
    // Save the bitmap for inspection
    image.saveToFile( IMAGES_DIR "015 composition 16 bit.tiff" );
}


void example_16(jett &r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(825,300, image_cmyk );
    image.set_profile_data(CMYK_ICC_DIR "USWebCoatedSWOP.icc");
    
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Load some images to compose on to the background
    jett_image image_background, image_lilly, image_teddy;
    
    image_background.loadFromFile(SRC_IMAGES "/freedigitalphotos/pebbles.jpg");
    image_lilly.loadFromFile( SRC_IMAGES "Mysecret_10icons_by_Artdesigner/lily_by_Artdesigner.lv.png");
    image_teddy.loadFromFile( SRC_IMAGES "Mysecret_10icons_by_Artdesigner/teddy_by_Artdesigner.lv.png" );
    
    // We need a colour transform
    jett_transform t = r.build_transform( ":srgb", image, INTENT_PERCEPTUAL );
    
    // Draw the background image
    r.bitblt(t, image_background, 0, 0, image_background.getWidth(), image_background.getHeight(), image, 0, 0, image.getWidth(), image.getHeight(), bitblt_cubic_scaling );
    
    // Now compose some images over the top...
    r.bitblt(t, image_lilly, image, 150, 25, bitblt_use_alpha );
    r.bitblt(t, image_teddy, image, 550, 25, bitblt_use_alpha );
    
    
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "016 alpha channel.tiff" );
}

void example_17(jett &r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(825,300, image_cmyk, true );
    image.set_profile_data(CMYK_ICC_DIR "USWebCoatedSWOP.icc");
    
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Load some images to compose on to the background
    jett_image image_teddy;
    
    image_teddy.loadFromFile( SRC_IMAGES "Mysecret_10icons_by_Artdesigner/teddy_by_Artdesigner.lv.png" );
    
    // We need a colour transform
    jett_transform t = r.build_transform( ":srgb", image, INTENT_PERCEPTUAL );
    
    // Define PI
    double PI = asin( 1.0 ) * 2.0;

    // Roate the matrix (in radians)
    for (int i = 0; i < 8; ++ i)
    {
        jett_matrix matrix;
        
        // Translate out a bit
        matrix.translate( jett_point( 50,0 ) );
        
        // Now rotate
        matrix.rotate( 2.0*PI / 8.0 * i);

        // Move to the centre of the image
        matrix.translate( jett_point( image.getWidth()/2, image.getHeight()/2) );
        
        // Now draw the image
        r.bitblt(t, image_teddy, 0, 0, image_teddy.getWidth(), image_teddy.getHeight(), image, matrix, bitblt_use_alpha );
    }
    
    
    
    
    // Save the bitmap for inspection
    image.saveToFile( IMAGES_DIR "017 matrix bitblt 16 bit.tiff" );
}

void make_patch_image( jett& r, jett_image& image, int patch_width, unsigned char *colours, int number_of_patches )
{
    // The dimensions
    int patch_size = 50;
    int border = 75;
    int gap = 10;
    int width = (patch_width) * (patch_size+gap) + border * 2;
    int height = (number_of_patches / patch_width + 1) * (patch_size) + border * 2;
    
    image.createImage(width, height, image_cmyk );
    image.erase();
    
    // Now layout the patches
    for (int patch = 0; patch < number_of_patches; ++ patch)
    {
        // Locate the patch
        int grid_x = patch % patch_width;
        int grid_y = patch / patch_width;
        
        int x = border + (patch_size+gap)* grid_x;
        int y = border + (patch_size) * grid_y;

        unsigned char *colour = colours + patch*4;
        
        int brightness = (colour[0] + colour[1] + colour[2] + colour[3]);
        
        // Draw the patch
        r.rectangle(image, colour, x, y, patch_size, patch_size);
        
        if (brightness < 128 || colour[3] == 0)
        {
            unsigned char colour[4] = { 0,0,0,255 };
            r.rectangle(image, colour, x+patch_size, y, gap, patch_size);
        }
    }
    
    // Add the text
    unsigned char colour[4] = { 0,0,0,255 };
    jett_font f = r.create_font( FONT_FILE, 0, 20, true );
    for (int y = 0; y < (number_of_patches / patch_width) + 1; ++ y)
    {
        char text[ 32 ];
        sprintf( text, "%d", y + 1);
        r.text(f, colour, image, border/2, border + patch_size*y + patch_size/2 + 10, text, string_rotate_0 );
    }

    for (int x = 0; x < patch_width; ++ x)
    {
        char text[ 32 ];
        sprintf( text, "%c", x + 'A');
        r.text(f, colour, image, border + x * (patch_size+gap) + (patch_size+gap)/2 - 10, border - 4, text, string_rotate_0 );
    }
    
}

void example_18( jett& r )
{
    // Create the image
    jett_image image_out;
    
    int patches_per_colour = 10;
    int number_of_colours = 4;
    int number_of_patches = patches_per_colour * number_of_colours;
    unsigned char* colours = new unsigned char( number_of_patches * number_of_colours );
    for (int patch = 0; patch < number_of_patches; ++ patch)
    {
        unsigned char *colour = colours + patch*4;
        colour[0] = 0;
        colour[1] = 0;
        colour[2] = 0;
        colour[3] = 0;
        
        int col = patch / patches_per_colour;
        int brightness = (patch % patches_per_colour) * 25.5;
        colour[col] = brightness;
    }
    make_patch_image( r, image_out, 14, colours, number_of_patches );

    
    jett_point p[5] = {
        jett_point( 0, 0 ),
        jett_point( 90, 56 ),
        jett_point( 150, 40 ),
        jett_point( 200, 100 ),
        jett_point( 255, 200 )
    };
    jett_linearization l = r.create_linearization();
    r.import_linearization(l, p, 5, 0 );
    r.import_linearization(l, p, 5, 0 );
    r.import_linearization(l, p, 5, 0 );
    r.import_linearization(l, p, 5, 0 );
    
    // Generate the dither screens using the void-and-cluster technique
    int sizes[] = { 32, 33, 34, 35 };
    jett_screens s = r.create_screens(4, sizes,1.5,256,NULL);
    
    // Now save the results
    image_out.saveToFile( EXAMPLES_DIR "018 No Linearization.tiff");

    // Now apply the linearization
    r.dither(image_out, s, l );
    
    // Now save the results
    image_out.saveToFile( EXAMPLES_DIR "018 With Linearization.tiff");

}



void example_19(jett &r)
{    
    // Load some images to compose on to the background
    jett_image image_lab;
    
    image_lab.loadFromFile("/Users/matt/Documents/riverbank/Products/GraphicsLibrary/trunk/test images/lab_test_image.tif");
    
    image_lab.saveToFile(EXAMPLES_DIR "019 lab file.tiff");
    
    jett_image image;
    image.createImage(image_lab.getWidth(),image_lab.getHeight(), image_rgb );
    image.set_profile_data(":srgb");

    // We need a colour transform
    jett_transform t = r.build_transform( ":lab", image, INTENT_RELATIVE_COLORIMETRIC );
    
    // Now compose a new image
    r.bitblt(t, image_lab, image, 0, 0, 0 );
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "019 lab conversion.tiff" );
}

void set_colour( unsigned char *colour, unsigned char c, unsigned char m, unsigned char y, unsigned char k)
{
	colour[0] = c;
	colour[1] = m;
	colour[2] = y;
	colour[3] = k;
}

void multiply_colour(unsigned char *colour_out, int mult, unsigned char *colour_in)
{
	for( int i =0; i<4; i++)
	{
		colour_out[i] = mult * colour_in[i];
	}
}

void add_colour (unsigned char *colour_out, unsigned char *colour_in, int mult, unsigned char *colour_to_add)
{
	for( int i =0; i<4; i++)
	{
		colour_out[i] = colour_in[i] + mult * colour_to_add[i];
	}
}

void build_rectangle( jett_point *points, int x, int y, int width, int height)
{
	points[0].x = x;
	points[0].y = y;
    
	points[1].x = x + width;
	points[1].y = y;
    
	points[2].x = x + width;
	points[2].y = y + height;
    
	points[3].x = x;
	points[3].y = y + height;
}


char* image_type_as_string( image_t type)
{
	/*
     * returns the type of image as a string
     */
    
	switch (type){
		case image_mono:
			return _T("mono");
			break;
		case image_rgb:
			return _T("rgb");
			break;
		case image_bgr:
			return _T("bgr");
			break;
		case image_rgba:
			return _T("rgba");
			break;
		case image_bgra:
			return _T("bgra");
			break;
		case image_cmyk:
			return _T("cmyk");
			break;
		case image_lab:
			return _T("lab");
			break;
	}
	return _T("unknown");
}

void test_d_14(jett &r )
{
	/*
     * check colours are in the correct order in the different image types
     */
    
	image_t imagetype;
	image_t imagetypes[7] = {image_cmyk, image_rgb, image_bgr, image_rgba, image_bgra};
	unsigned char colours[5][7][4];
    
	//define colours for CMYK image type
	set_colour(colours[0][0], 0, 1, 1, 0);		//R
	set_colour(colours[0][1], 1, 0, 1, 0);		//G
	set_colour(colours[0][2], 1, 1, 0, 0);		//B
	set_colour(colours[0][3], 1, 0, 0, 0);		//C
	set_colour(colours[0][4], 0, 1, 0, 0);		//M
	set_colour(colours[0][5], 0, 0, 1, 0);		//Y
	set_colour(colours[0][6], 1, 1, 1, 0);		//W
	
	// define colours for RGB image type
	set_colour(colours[1][0], 1, 0, 0, 0);		//R
	set_colour(colours[1][1], 0, 1, 0, 0);		//G
	set_colour(colours[1][2], 0, 0, 1, 0);		//B
	set_colour(colours[1][3], 0, 1, 1, 0);		//C
	set_colour(colours[1][4], 1, 0, 1, 0);		//M
	set_colour(colours[1][5], 1, 1, 0, 0);		//Y
	set_colour(colours[1][6], 1, 1, 1, 0);		//W
    
	// define colours for BGR image type
	set_colour(colours[2][0], 0, 0, 1, 0);		//R
	set_colour(colours[2][1], 0, 1, 0, 0);		//G
	set_colour(colours[2][2], 1, 0, 0, 0);		//B
	set_colour(colours[2][3], 1, 1, 0, 0);		//C
	set_colour(colours[2][4], 1, 0, 1, 0);		//M
	set_colour(colours[2][5], 0, 1, 1, 0);		//Y
	set_colour(colours[2][6], 1, 1, 1, 0);		//W
    
	// define colours for RGBA image type
	set_colour(colours[3][0], 1, 0, 0, 0);		//R
	set_colour(colours[3][1], 0, 1, 0, 0);		//G
	set_colour(colours[3][2], 0, 0, 1, 0);		//B
	set_colour(colours[3][3], 0, 1, 1, 0);		//C
	set_colour(colours[3][4], 1, 0, 1, 0);		//M
	set_colour(colours[3][5], 1, 1, 0, 0);		//Y
	set_colour(colours[3][6], 1, 1, 1, 0);		//W
    
	// define colours for BGRA image type
	set_colour(colours[4][0], 0, 0, 1, 0);		//R
	set_colour(colours[4][1], 0, 1, 0, 0);		//G
	set_colour(colours[4][2], 1, 0, 0, 0);		//B
	set_colour(colours[4][3], 1, 1, 0, 0);		//C
	set_colour(colours[4][4], 1, 0, 1, 0);		//M
	set_colour(colours[4][5], 0, 1, 1, 0);		//Y
	set_colour(colours[4][6], 1, 1, 1, 0);		//W
    
	// Create the bitmap we are going to draw on
    jett_image image;
    
    jett_point points[ 4 ];
    
	int c;
	unsigned char colour[4];
    
	// cycle through different image types
	for(int ics=1; ics<5; ics++)
	{
		imagetype = imagetypes[ics];
		image.createImage(256,512, imagetype );
		image.erase();
        
		//draw the colour bars
		for(int icol = 0; icol<7; icol++)
		{
			for(int ix = 0; ix<9; ix++)
			{
				build_rectangle( points, 16*ix, 16*icol, 16, 16);
				multiply_colour(colour, std::min(255,ix*32), colours[ics][icol]);
				r.polygon(image, colour, points, 4, polygon_fast );
			}
		}
        
		// draw additional patch charts for image types that support transparencies
		if(imagetype == image_rgba | imagetype == image_bgra)
		{
			//draw colour bars with transparency settings
			unsigned char colour_transparency[4];
			unsigned char a_channel[4] = {0,0,0,1};
            
			// draw for three transparencies
			for (int itrans = 1; itrans<4; itrans++)
			{
				int alpha = itrans * 85;
				int iy_offset = itrans * 128;
                
				// draw the colour bars
				for(int icol = 0; icol<7; icol++)
				{
					for(int ix = 0; ix<9; ix++)
					{
						build_rectangle( points, 16*ix, iy_offset + 16*icol, 16, 16);
						multiply_colour(colour, std::min(255,ix*32), colours[ics][icol]);
						add_colour(colour_transparency, colour, alpha, a_channel);
						r.polygon(image, colour_transparency, points, 4, polygon_fast );
					}
				}
			}
		}
        
		// save to file
		TCHAR  filename[ 256 ];
		strcpy(filename, EXAMPLES_DIR);
		strcat(filename, image_type_as_string( imagetype ));
		strcat(filename, _T(" 114 test_d.tiff"));
		image.saveToFile( filename );
	}
}

void example_20(jett &r)
{
    // Create the bitmap we are going to draw on
    jett_image image;
    image.createImage(825,300, image_cmyk );
    image.set_profile_data(CMYK_ICC_DIR "USWebCoatedSWOP.icc");
    
    
    // Make the image white - the image is normally created unitialised as this
    // is the fastest.
    image.erase();
    
    // Load some images to compose on to the background
    jett_image image_background;
    
    image_background.loadFromFile(SRC_IMAGES "Lenna.png");
    
    // We need a colour transform
    jett_transform t = r.build_transform( ":srgb", image, INTENT_PERCEPTUAL );
    
    // Draw the background image
	// image.set_clip_rect( 50,50, 400,150 );
	r.bitblt(t, image_background, -100, -100, image_background.getWidth()+200, image_background.getHeight()+200, image, 0, 0, image.getWidth(), image.getHeight(), bitblt_cubic_scaling | bitblt_rotate_270 );
    
    // Save the bitmap for inspection
    image.saveToFile( EXAMPLES_DIR "019 clipping.tiff" );
}


int main (int argc, const char * argv[])
{
    jett r;
    r.init(false);

    example_8( r );
    example_9( r );
    example_15( r );
    example_17( r );
    example_11( r );

    exit(0);
    
    CTimer t1;
    t1.start();


    uint64_t ns1 = t1.elapsed();
    printf("Font render %lld\n", ns1 );


    // example_20( r );
    
#if 0
    test_d_14( r );
#endif
    
#if 0
    example_1( r );
    example_2( r );
    example_3( r );
    example_4( r );
    example_5( r );
    example_6( r );
    example_7( r );
    example_8( r );
    example_9( r );
    example_10( r );
    example_11( r );
    example_12( r );
    example_13( r );
    example_14( r );
    example_15( r );
    example_16( r );
    example_17( r );
    example_18( r );
    example_19( r );
#endif
    
#if 0
    
    // Create a LCMS transform
    jett_transform t = NULL;
    jett_transform t2 = NULL;
    jett_transform t3 = NULL;
    
    jett_matrix a( rand(), rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f );
    jett_matrix b = a.invert();
    
    jett_point p1( rand(), rand() );
    jett_point ip = a.apply(p1);
    jett_point op = b.apply(ip);
    
    //t2 = r.build_transform(":mono", CMYK_ICC_DIR "USWebCoatedSWOP.icc", INTENT_PERCEPTUAL );
    
    unsigned char colour[4] = { 192, 64, 0, 0 };

    //t = r.build_transform( CMYK_ICC_DIR  "USWebCoatedSWOP.icc", RGB_ICC_DIR "AppleRGB.icc", INTENT_PERCEPTUAL );
    // t = r.build_transform(":srgb", ":mono", INTENT_PERCEPTUAL );
    // t = r.build_transform(":mono", ":srgb", INTENT_PERCEPTUAL );

	jett_image image_in, image_2;

    // Create a font
    jett_font f1 = r.create_font( FONT_FILE, 0, 50, true );
    jett_font f2 = r.create_font( FONT_FILE, 0, 50, false );
    
	image_in.loadFromFile( IMAGES_DIR "rgb_test_image.jpeg", image_mode_gpu_copy );
	image_2.loadFromFile(IMAGES_DIR "alphatest.png", image_mode_gpu_copy );
	// image_in.loadFromFile(IMAGES_DIR "mono_test_image.tif");
	// image_in.loadFromFile(IMAGES_DIR "cmyk_test_image.tif");

	// Now load an image a transform it
	{
		jett_image image_out;
    
	    
        // t = r.build_transform(image_in, CMYK_ICC_DIR "USWebCoatedSWOP.icc", INTENT_PERCEPTUAL );
        // t = r.build_transform(":srgb",":srgb", INTENT_PERCEPTUAL );

        
		// image_out.createImage(image_in.getWidth(), image_in.getHeight(), image_mono );
		image_out.createImage(image_in.getWidth(),image_in.getHeight(),  image_cmyk, false, image_mode_default );
        image_out.set_profile_data( CMYK_ICC_DIR "USWebCoatedSWOP.icc" );
        
        t = r.build_transform( ":srgb", image_out, INTENT_PERCEPTUAL );
        t3 =r.build_transform( ":srgb", image_out, INTENT_PERCEPTUAL );


		// Perform the transform using GPU
		CTimer t1;
        
        r.cache_image(image_in, true);
        r.cache_image(image_out, false);
        
        t1.start();
        r.bitblt(t, image_in, 0, 0, image_in.getWidth(), image_in.getHeight(),
                 image_out, 0, 0, image_out.getWidth(), image_out.getHeight(), bitblt_cubic_scaling );
        uint64_t ns1 = t1.elapsed();
        printf("Background copy %lld\n", ns1 );


        
        //
        // Convert a single colour
        //
        unsigned char rgb_col[3] = { 255,0,0 };
        unsigned char cmyk_col[4];
        r.convert(t, rgb_col, cmyk_col );
        
        //
        // Draw a polygon
        //
        jett_point points[ 12 ];
        build_polygon(points, 250, 250, 100, 50, 6, 0 );
		r.polygon(image_out, cmyk_col, points, 12, polygon_best );

        //r.flush();
        
        build_polygon(points, 650, 250, 100, 50, 6, 90 );
		// r.polygon(image_out, cmyk_col, points, 12, polygon_best );
        r.lines(image_out, cmyk_col, 5, points,12, true, line_join_miter | line_best);
        
        double PI = asin( 1.0 ) * 2.0;
        double q = PI / 7;
        jett_matrix m( cos(q),sin(q),-sin(q),cos(q), 1400, 300);
        r.bitblt(t3, image_2, 0, 0, image_2.getWidth(), image_2.getHeight(),
                image_out, m, bitblt_cubic_scaling | bitblt_use_alpha );
        
        r.flush();

        r.rectangle(image_out, colour, 750, 1000, 150, 100);
        
        t1.start();
        double q2 = PI / 9;
        jett_matrix m2( cos(q2),sin(q2),-sin(q2),cos(q2), 0, 0);
        r.set_font_matrix(f1, m2 );
        r.text(f1, colour, image_out, 450, 450, "Text at 0 AWAY", string_rotate_0 );
        r.text(f1, colour, image_out, 450, 450, "Text at 90",  string_rotate_90 );
        r.text(f1, colour, image_out, 450, 450, "Text at 180", string_rotate_180 );
        r.text(f1, colour, image_out, 450, 450, "Text at 270", string_rotate_270 );

        r.set_font_matrix(f2, m2 );
        r.text(f2, colour, image_out, 850, 450, "Text at 0", string_rotate_0 );
        r.text(f2, colour, image_out, 850, 450, "Text at 90",  string_rotate_90 );
        r.text(f2, colour, image_out, 850, 450, "Text at 180", string_rotate_180 );
        r.text(f2, colour, image_out, 850, 450, "Text at 270", string_rotate_270 );
        uint64_t ns = t1.elapsed();
        printf("Font took %lld\n", ns );
        
		// .. and save
		image_out.saveToFile(IMAGES_DIR "output.tiff");

        r.destroy_font(f1);
        r.destroy_font(f2);
        r.destroy_transform(t);
        r.destroy_transform(t2);
    }
#endif

	return 0;
}

