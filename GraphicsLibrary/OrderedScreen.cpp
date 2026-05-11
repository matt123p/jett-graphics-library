//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "OrderedBitmap.h"
#include "OrderedScreen.h"

COrderedScreen::COrderedScreen(void)
{
	m_width = 0;
	m_height = 0;
	m_screen = NULL;
	m_quantizer = 1.0;
}

COrderedScreen::~COrderedScreen(void)
{
	delete[] m_screen;
}

void COrderedScreen::create(int width, int height, double sigma, int output_levels, jett_progress_callback callback)
{
	delete[] m_screen;
	m_screen = NULL;

	{
		// Checks
		int size = std::min(width, height);
		if (size < 8 || size > 4000)
		{
			throw jett_exception(JETT_INVALID_ARGUMENT, 0, "The size must be between 8 and 256 inclusive");
		}

		if (sigma < 0.2 || sigma > size)
		{
			throw jett_exception(JETT_INVALID_ARGUMENT, 0, "Sigma must be between 0.2 and the size");
		}

		if (output_levels < 2 || output_levels > 256)
		{
			throw jett_exception(JETT_INVALID_ARGUMENT, 0, "The output_levels must be between 2 and 256 inclusive");
		}
	}

	{
		// Make the screen
		COrderedBitmap b(callback);
		b.make_initial_pattern(width, height, 40, sigma);

		m_width = width;
		m_height = height;
		m_screen = new int[width * height];

		b.make_screen(this);
		normalize(256, output_levels);
	}
}

void COrderedScreen::set_rank(int x, int y, int rank)
{
	m_screen[y * m_width + x] = rank;
}

void COrderedScreen::dump()
{
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			printf("%d,", m_screen[y * m_width + x]);
		}
		printf("\n");
	}
}

void COrderedScreen::normalize(int input_levels, int output_levels)
{
	m_quantizer = static_cast<double>(input_levels - 1) / (output_levels - 1);
	double mn = m_width * m_height;
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			m_screen[y * m_width + x] = static_cast<int>(m_quantizer / mn * (m_screen[y * m_width + x] + 0.5));
		}
	}
}

const int *COrderedScreen::get_screen() const
{
	return m_screen;
}

// Save/Load a screen embedded in a file
void COrderedScreen::save(FILE *fout)
{
	fwrite(&m_width, sizeof(m_width), 1, fout);
	fwrite(&m_height, sizeof(m_height), 1, fout);
	fwrite(&m_quantizer, sizeof(m_quantizer), 1, fout);

	fwrite(m_screen, sizeof(int32_t), m_width * m_height, fout);
}

void COrderedScreen::load(FILE *fin)
{
	delete[] m_screen;
	m_screen = NULL;

	if (fread(&m_width, sizeof(m_width), 1, fin) != 1 || m_width < 8 || m_width > 256)
	{
		throw jett_exception(JETT_UNSUPPORTED_SCREEN, 0, "Not a valid ordered screen file (invalid width)");
	}
	if (fread(&m_height, sizeof(m_height), 1, fin) != 1 || m_height < 8 || m_height > 256)
	{
		throw jett_exception(JETT_UNSUPPORTED_SCREEN, 0, "Not a valid ordered screen file (invalid height)");
	}
	if (fread(&m_quantizer, sizeof(m_quantizer), 1, fin) != 1 || m_quantizer <= 0)
	{
		throw jett_exception(JETT_UNSUPPORTED_SCREEN, 0, "Not a valid ordered screen file (invalid quantizer)");
	}

	m_screen = new int[m_width * m_height];

	if (fread(m_screen, sizeof(int32_t), m_width * m_height, fin) != m_width * m_height)
	{
		throw jett_exception(JETT_UNSUPPORTED_SCREEN, 0, "Not a valid ordered screen file (invalid screen data)");
	}
}
