//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "timer.h"
#include "colour_management_tests.h"
#include "../GraphicsLibrary/jett.h"
#include <filesystem>
#include <math.h>
#include <cstdlib>
#include <set>
#include <system_error>
#include <vector>

void build_polygon(jett_point *points, int x, int y, int r, int innerR, int vertexCount, double startAngle);

typedef std::basic_string<TCHAR> output_string;

////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define TEST_IMAGE_ROOT "test_images\\"
#define FONT_FILE _T("C:\\Windows\\Fonts\\Arial.ttf")
#else
#define TEST_IMAGE_ROOT "test_images/"
#define FONT_FILE test_font_path()
#endif

#define TEST_ASSET_DIR TEST_IMAGE_ROOT
#define SRC_IMAGES TEST_ASSET_DIR
#define EXAMPLES_DIR TEST_IMAGE_ROOT
#define IMAGES_DIR TEST_ASSET_DIR
#define OUTPUT_DIR EXAMPLES_DIR

namespace
{
	namespace fs = std::filesystem;

	bool path_exists(const output_string &path)
	{
		std::error_code error;
		return fs::exists(fs::path(path), error);
	}

	output_string path_native_string(const fs::path &path)
	{
		return path.native();
	}

	output_string join_output_path(const output_string &base, const output_string &name)
	{
		if (base.empty())
		{
			return name;
		}

		if (name.empty())
		{
			return base;
		}

		TCHAR last = base[base.size() - 1];
		if (last == _T('\\') || last == _T('/'))
		{
			return base + name;
		}

	#ifdef _WIN32
		return base + _T("\\") + name;
	#else
		return base + _T("/") + name;
	#endif
	}

	void throw_output_error(const char *message)
	{
		throw jett_exception(JETT_INTERNAL_ERROR, 0, message);
	}

	bool directory_exists(const output_string &path)
	{
		std::error_code error;
		return fs::is_directory(fs::path(path), error);
	}

	bool find_profile_in_tree(const output_string &root, const output_string &profile_name, output_string &resolved_path)
	{
		std::error_code error;
		fs::recursive_directory_iterator end;
		for (fs::recursive_directory_iterator it(fs::path(root), fs::directory_options::skip_permission_denied, error); it != end; it.increment(error))
		{
			if (error)
			{
				error.clear();
				continue;
			}

			if (!it->is_regular_file(error))
			{
				error.clear();
				continue;
			}

			if (path_native_string(it->path().filename()) == profile_name)
			{
				resolved_path = path_native_string(it->path());
				return true;
			}
		}

		return false;
	}

	output_string resolve_standard_profile_path(const std::vector<output_string> &profile_names)
	{
		std::vector<output_string> search_roots;
	#ifdef _WIN32
		search_roots.push_back(_T("..\\GraphicsLibrary\\GraphicsLibrary\\trunk\\Adobe ICC Profiles"));
		search_roots.push_back(_T("Adobe ICC Profiles"));
		search_roots.push_back(_T("..\\Adobe ICC Profiles"));
		search_roots.push_back(_T("..\\..\\Adobe ICC Profiles"));
		search_roots.push_back(_T("C:\\src\\GraphicsLibrary\\GraphicsLibrary\\trunk\\Adobe ICC Profiles"));
		search_roots.push_back(_T("C:\\Windows\\System32\\spool\\drivers\\color"));
		search_roots.push_back(_T("C:\\Program Files\\Common Files\\Adobe\\Color\\Profiles"));
		search_roots.push_back(_T("C:\\Program Files (x86)\\Common Files\\Adobe\\Color\\Profiles"));
	#else
		const char *home = getenv("HOME");
		if (home && home[0] != 0)
		{
			search_roots.push_back(output_string(home) + _T("/Library/ColorSync/Profiles"));
			search_roots.push_back(output_string(home) + _T("/Library/Application Support/Adobe/Color/Profiles"));
			search_roots.push_back(output_string(home) + _T("/.local/share/color/icc"));
			search_roots.push_back(output_string(home) + _T("/.color/icc"));
			search_roots.push_back(output_string(home) + _T("/.fonts"));
			search_roots.push_back(output_string(home) + _T("/.local/share/fonts"));
		}

		search_roots.push_back(_T("/Library/ColorSync/Profiles"));
		search_roots.push_back(_T("/System/Library/ColorSync/Profiles"));
		search_roots.push_back(_T("/Network/Library/ColorSync/Profiles"));
		search_roots.push_back(_T("/Library/Application Support/Adobe/Color/Profiles"));
		search_roots.push_back(_T("/usr/share/color/icc"));
		search_roots.push_back(_T("/usr/local/share/color/icc"));
		search_roots.push_back(_T("/var/lib/color/icc"));
	#endif

		for (size_t i = 0; i < search_roots.size(); ++i)
		{
			if (!directory_exists(search_roots[i]))
			{
				continue;
			}

			for (size_t profile_index = 0; profile_index < profile_names.size(); ++profile_index)
			{
				output_string direct_match = join_output_path(search_roots[i], profile_names[profile_index]);
				if (path_exists(direct_match))
				{
					return direct_match;
				}

				output_string recursive_match;
				if (find_profile_in_tree(search_roots[i], profile_names[profile_index], recursive_match))
				{
					return recursive_match;
				}
			}
		}

		throw_output_error("Unable to locate required ICC profile in standard system locations");
		return output_string();
	}

	#ifdef _WIN32
	const TCHAR *cmyk_test_profile_path()
	{
		static output_string path = resolve_standard_profile_path(std::vector<output_string>{
			_T("USWebCoatedSWOP.icc"),
			_T("SWOP_TR005_coated_5.icc"),
			_T("SWOP_TR003_coated_3.icc"),
			_T("default_cmyk.icc"),
			_T("ps_cmyk.icc"),
			_T("FOGRA39L_coated.icc")
		});
		return path.c_str();
	}

	const TCHAR *rgb_test_profile_path()
	{
		static output_string path = resolve_standard_profile_path(std::vector<output_string>{
			_T("AdobeRGB1998.icc"),
			_T("compatibleWithAdobeRGB1998.icc"),
			_T("sRGB.icc")
		});
		return path.c_str();
	}
	#else
	output_string resolve_standard_font_path(const std::vector<output_string> &font_names)
	{
		std::vector<output_string> search_roots;
		const char *home = getenv("HOME");
		if (home && home[0] != 0)
		{
			search_roots.push_back(output_string(home) + _T("/Library/Fonts"));
			search_roots.push_back(output_string(home) + _T("/.fonts"));
			search_roots.push_back(output_string(home) + _T("/.local/share/fonts"));
		}

		search_roots.push_back(_T("/Library/Fonts"));
		search_roots.push_back(_T("/System/Library/Fonts"));
		search_roots.push_back(_T("/usr/share/fonts"));
		search_roots.push_back(_T("/usr/local/share/fonts"));

		for (size_t i = 0; i < search_roots.size(); ++i)
		{
			if (!directory_exists(search_roots[i]))
			{
				continue;
			}

			for (size_t font_index = 0; font_index < font_names.size(); ++font_index)
			{
				output_string direct_match = join_output_path(search_roots[i], font_names[font_index]);
				if (path_exists(direct_match))
				{
					return direct_match;
				}

				output_string recursive_match;
				if (find_profile_in_tree(search_roots[i], font_names[font_index], recursive_match))
				{
					return recursive_match;
				}
			}
		}

		throw_output_error("Unable to locate required font in standard system locations");
		return output_string();
	}

	const TCHAR *cmyk_test_profile_path()
	{
		static output_string path = resolve_standard_profile_path(std::vector<output_string>{
			_T("USWebCoatedSWOP.icc"),
			_T("SWOP_TR005_coated_5.icc"),
			_T("SWOP_TR003_coated_3.icc"),
			_T("default_cmyk.icc"),
			_T("ps_cmyk.icc"),
			_T("FOGRA39L_coated.icc")
		});
		return path.c_str();
	}

	const TCHAR *rgb_test_profile_path()
	{
		static output_string path = resolve_standard_profile_path(std::vector<output_string>{
			_T("AdobeRGB1998.icc"),
			_T("compatibleWithAdobeRGB1998.icc"),
			_T("sRGB.icc")
		});
		return path.c_str();
	}

	const TCHAR *test_font_path()
	{
		static output_string path = resolve_standard_font_path(std::vector<output_string>{
			_T("Arial.ttf"),
			_T("LiberationSans-Regular.ttf"),
			_T("DejaVuSans.ttf")
		});
		return path.c_str();
	}
#endif

	void ensure_output_directory(const output_string &path)
	{
		std::error_code error;
		if (fs::create_directory(fs::path(path), error) || !error || error == std::errc::file_exists)
		{
			return;
		}

		throw_output_error("Unable to create output directory");
	}

	bool required_colour_profiles_available()
	{
		try
		{
			return path_exists(output_string(cmyk_test_profile_path()))
				&& path_exists(output_string(rgb_test_profile_path()));
		}
		catch (...)
		{
			return false;
		}
	}

	const int kCompareByteTolerance = 2;

	void clear_output_directory(const output_string &path)
	{
		ensure_output_directory(path);

		std::error_code error;
		for (fs::directory_iterator it(fs::path(path), error); !error && it != fs::directory_iterator(); it.increment(error))
		{
			if (!it->is_regular_file(error))
			{
				error.clear();
				continue;
			}

			fs::remove(it->path(), error);
			if (error)
			{
				throw_output_error("Unable to clear output directory");
			}
		}
	}

	fs::file_time_type get_suite_start_marker()
	{
		return fs::file_time_type::clock::now();
	}

	int move_recent_outputs(const output_string &target_dir, const fs::file_time_type &suite_start, std::set<output_string> &output_names)
	{
		ensure_output_directory(target_dir);

		int moved = 0;
		std::error_code error;
		for (fs::directory_iterator it(fs::path(_T(EXAMPLES_DIR)), error); !error && it != fs::directory_iterator(); it.increment(error))
		{
			if (!it->is_regular_file(error))
			{
				error.clear();
				continue;
			}

			if (fs::last_write_time(it->path(), error) < suite_start)
			{
				error.clear();
				continue;
			}

			output_string source = path_native_string(it->path());
			output_string target = join_output_path(target_dir, path_native_string(it->path().filename()));

			fs::remove(fs::path(target), error);
			error.clear();
			fs::rename(fs::path(source), fs::path(target), error);
			if (error)
			{
				error.clear();
				fs::copy_file(fs::path(source), fs::path(target), fs::copy_options::overwrite_existing, error);
				if (error)
				{
					throw_output_error("Unable to move generated output file");
				}
				fs::remove(fs::path(source), error);
			}

			output_names.insert(path_native_string(it->path().filename()));
			++moved;
		}
		return moved;
	}

	bool compare_output_images(const output_string &cpu_path, const output_string &gpu_path, output_string &reason)
	{
		jett_image cpu_image;
		jett_image gpu_image;

		cpu_image.loadFromFile(cpu_path.c_str());
		gpu_image.loadFromFile(gpu_path.c_str());

		if (cpu_image.getType() != gpu_image.getType())
		{
			reason = _T("type mismatch");
			return false;
		}

		if (cpu_image.getWidth() != gpu_image.getWidth() || cpu_image.getHeight() != gpu_image.getHeight())
		{
			reason = _T("size mismatch");
			return false;
		}

		if (cpu_image.getStride() != gpu_image.getStride() || cpu_image.getComponents() != gpu_image.getComponents())
		{
			reason = _T("layout mismatch");
			return false;
		}

		unsigned char *cpu_data = cpu_image.lockData(true);
		unsigned char *gpu_data = gpu_image.lockData(true);
		size_t bytes = cpu_image.getStride() * cpu_image.getHeight();
		size_t mismatched_bytes = 0;
		int max_diff = 0;
		size_t first_mismatch = bytes;
		int first_cpu_value = 0;
		int first_gpu_value = 0;
		for (size_t i = 0; i < bytes; ++i)
		{
			int diff = abs(static_cast<int>(cpu_data[i]) - static_cast<int>(gpu_data[i]));
			if (diff > max_diff)
			{
				max_diff = diff;
			}

			if (diff > kCompareByteTolerance)
			{
				if (first_mismatch == bytes)
				{
					first_mismatch = i;
					first_cpu_value = cpu_data[i];
					first_gpu_value = gpu_data[i];
				}
				++mismatched_bytes;
			}
		}
		cpu_image.unlockData();
		gpu_image.unlockData();

		if (mismatched_bytes != 0)
		{
			size_t first_y = first_mismatch / cpu_image.getStride();
			size_t row_offset = first_mismatch % cpu_image.getStride();
			size_t first_x = row_offset / cpu_image.getPixelStride();
			size_t first_component = row_offset % cpu_image.getPixelStride();
			TCHAR detail[128];
			_stprintf_s(detail, _T("pixel mismatch bytes=%u max_diff=%d first=(%u,%u,c%u %d!=%d) tol=%d"), static_cast<unsigned>(mismatched_bytes), max_diff, static_cast<unsigned>(first_x), static_cast<unsigned>(first_y), static_cast<unsigned>(first_component), first_cpu_value, first_gpu_value, kCompareByteTolerance);
			reason = detail;
			return false;
		}

		return true;
	}

	int compare_mode_outputs(const std::set<output_string> &cpu_outputs, const std::set<output_string> &gpu_outputs)
	{
		int failures = 0;

		for (std::set<output_string>::const_iterator i = cpu_outputs.begin(); i != cpu_outputs.end(); ++i)
		{
			if (gpu_outputs.find(*i) == gpu_outputs.end())
			{
				_tprintf(_T("FAIL compare: missing GPU output %s\n"), i->c_str());
				++failures;
			}
		}

		for (std::set<output_string>::const_iterator i = gpu_outputs.begin(); i != gpu_outputs.end(); ++i)
		{
			if (cpu_outputs.find(*i) == cpu_outputs.end())
			{
				_tprintf(_T("FAIL compare: missing CPU output %s\n"), i->c_str());
				++failures;
			}
		}

		for (std::set<output_string>::const_iterator i = cpu_outputs.begin(); i != cpu_outputs.end(); ++i)
		{
			if (gpu_outputs.find(*i) == gpu_outputs.end())
			{
				continue;
			}

			output_string reason;
			output_string cpu_path = join_output_path(join_output_path(_T(EXAMPLES_DIR), _T("cpu")), *i);
			output_string gpu_path = join_output_path(join_output_path(_T(EXAMPLES_DIR), _T("gpu")), *i);

			try
			{
				if (!compare_output_images(cpu_path, gpu_path, reason))
				{
					_tprintf(_T("FAIL compare: %s (%s)\n"), i->c_str(), reason.c_str());
					++failures;
				}
			}
			catch (const jett_exception &e)
			{
				_tprintf(_T("FAIL compare: %s (load error %s, code=%d subsys=%d)\n"), i->c_str(), e.m_message, e.m_code, e.m_subsys_code);
				++failures;
			}
		}

		_tprintf(_T("COMPARE COMPLETE failures=%d files=%u\n"), failures, static_cast<unsigned>(cpu_outputs.size()));
		return failures;
	}

	int run_suite_for_mode(bool use_gpu, const TCHAR *mode_name, std::set<output_string> &output_names);
}

void build_polygon(jett_point *points, int x, int y, int r, int innerR, int vertexCount, double startAngle)
{
	double PI = asin(1.0) * 2.0;
	double addAngle = 2 * PI / vertexCount;
	startAngle = startAngle / 360.0 * 2 * PI;
	double angle = startAngle;
	double innerAngle = startAngle + PI / vertexCount;
	for (int i = 0; i < vertexCount; i++)
	{
		points[i * 2].x = (float)(r * cos(angle)) + x;
		points[i * 2].y = (float)(r * sin(angle)) + y;
		angle += addAngle;
		points[i * 2 + 1].x = (float)(innerR * cos(innerAngle)) + x;
		points[i * 2 + 1].y = (float)(innerR * sin(innerAngle)) + y;
		innerAngle += addAngle;
	}
}

void example_1(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a line
	jett_point points[2];
	points[0] = jett_point(10, 10);
	points[1] = jett_point(240, 10);
	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)
	r.lines(image, cmyk_col, 4, points, 2, false, 0);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "001 simple_example.tiff");
}

void example_2(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a line
	jett_point points[2];
	points[0] = jett_point(10, 10);
	points[1] = jett_point(240, 240);
	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)

	// Draw a line 4 pixels wide
	r.lines(image, cmyk_col, 4, points, 2, false, 0);

	points[0] = jett_point(240, 10);
	points[1] = jett_point(10, 240);
	r.lines(image, cmyk_col, 4, points, 2, false, 0);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "002 line_example.tiff");
}

void example_3(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a U shape
	jett_point points[4];
	points[0] = jett_point(80, 25);
	points[1] = jett_point(35, 225);
	points[2] = jett_point(215, 225);
	points[3] = jett_point(170, 25);

	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)

	// Draw a line 20 pixels wide
	r.lines(image, cmyk_col, 20, points, 4, false, line_join_bevel);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "003 line_example bevel.tiff");
}

void example_4(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a U shape
	jett_point points[4];
	points[0] = jett_point(80, 25);
	points[1] = jett_point(35, 225);
	points[2] = jett_point(215, 225);
	points[3] = jett_point(170, 25);

	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)

	// Draw a line 20 pixels wide
	r.lines(image, cmyk_col, 20, points, 4, false, line_join_miter);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "004 line_example miter.tiff");
}

void example_5(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a U shape
	jett_point points[4];
	points[0] = jett_point(80, 25);
	points[1] = jett_point(35, 225);
	points[2] = jett_point(215, 225);
	points[3] = jett_point(170, 25);

	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)

	// Draw a line 20 pixels wide
	r.lines(image, cmyk_col, 20, points, 4, false, line_join_none);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "005 line_example none.tiff");
}

void example_6(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a star
	jett_point points[12];
	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)
	build_polygon(points, 125, 125, 50, 25, 6, 0);
	r.lines(image, cmyk_col, 4, points, 12, true, line_join_miter);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "006 line_example closed.tiff");
}

void example_7(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a star
	jett_point points[12];
	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)
	build_polygon(points, 125, 125, 50, 25, 6, 0);
	r.lines(image, cmyk_col, 4, points, 12, true, line_join_miter | line_best);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "007 line_example best.tiff");
}

void example_8(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a star
	jett_point points[12];
	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)
	build_polygon(points, 125, 125, 50, 25, 6, 0);

	CTimer t1;
	t1.start();
	r.polygon(image, cmyk_col, points, 12, polygon_best);
	printf("%llu\n", static_cast<unsigned long long>(t1.elapsed()));

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "008 polygon_example best.tiff");
}

void example_9(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a star
	jett_point points[12];
	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)
	build_polygon(points, 125, 125, 50, 25, 6, 0);
	r.polygon(image, cmyk_col, points, 12, polygon_fast);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "009 polygon_example fast.tiff");
}

void example_10(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// We need a colour to draw with
	unsigned char colour[4] = {192, 64, 0, 0};

	// We a font to draw with
	jett_font f = r.create_font(FONT_FILE, 0, 20, true);

	// Draw some text
	r.text(f, colour, image, 125, 125, "Text at 0", string_rotate_0);
	r.text(f, colour, image, 125, 125, "Text at 90", string_rotate_90);
	r.text(f, colour, image, 125, 125, "Text at 180", string_rotate_180);
	r.text(f, colour, image, 125, 125, "Text at 270", string_rotate_270);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "010 text_example.tiff");
}

void example_11(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// We need a colour to draw with
	unsigned char colour[4] = {192, 64, 0, 0};

	// We a font to draw with
	jett_font f1 = r.create_font(FONT_FILE, 0, 20, false);
	jett_font f2 = r.create_font(FONT_FILE, 0, 20, true);

	// Draw some text
	r.text(f1, colour, image, 10, 50, "Text without anti-alias", string_rotate_0);
	r.text(f2, colour, image, 10, 90, "Text with anti-alias", string_rotate_0);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "011 text_example aa.tiff");
}

void example_12(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(250, 250, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// We need a colour to draw with
	unsigned char colour[4] = {192, 64, 0, 0};

	// We a font to draw with
	jett_font f = r.create_font(FONT_FILE, 0, 20, true);

	const char *text = "Text drawn round in a circle";
	size_t len = strlen(text);
	double PI = asin(1.0) * 2.0;

	jett_point centre(125.0f, 125.0f);
	double angle = -PI / 2;
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
		r.set_font_matrix(f, identity);
		jett_point size = r.size_text(f, character);

		// Determine the angle this represents
		double delta_angle = atan2(static_cast<double>(size.x), radius);
		angle += delta_angle / 2;

		// Create a matrix at the correct angle to draw the text at
		jett_matrix m;
		m.rotate(angle);

		// Rotate the font by the angle
		r.set_font_matrix(f, m);

		// Now draw the text
		r.text(f, colour, image, p.x, p.y, character, string_rotate_0);

		angle += delta_angle / 2;
	}

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "012 rotated text.tiff");
}

void example_13(jett &r)
{
	// Generate the dither screens using the void-and-cluster technique
	int sizes[] = {32};
	jett_screens s = r.create_screens(1, sizes, 1.5, 2);

	// Load an RGB images
	jett_image rgb_in;
	rgb_in.loadFromFile(IMAGES_DIR "test_img_1.png");

	// Now convert to a Monochrome image
	jett_image mono_out;
	mono_out.createImage(rgb_in.getWidth(), rgb_in.getHeight(), image_mono);
	mono_out.set_profile_data(":mono");
	jett_transform t = r.build_transform(":srgb", mono_out, INTENT_PERCEPTUAL);
	r.bitblt(t, rgb_in, mono_out, 0, 0, 0);

	// Create the output images (one for each colour plane)
	jett_image outputs[1];
	outputs[0].createImage(mono_out.getWidth(), mono_out.getHeight(), image_mono1);

	// Perform the dithering
	r.dither(mono_out, outputs, s, NULL);

	// Now save the results
	outputs[0].saveToFile(EXAMPLES_DIR "013 output.bmp");
}

void example_14(jett &r)
{
	// Generate the dither screens using the void-and-cluster technique
	int sizes[] = {32, 33, 34, 35};
	jett_screens s = r.create_screens(4, sizes, 1.5, 2);

	// Load an RGB images
	jett_image rgb_in;
	rgb_in.loadFromFile(IMAGES_DIR "test_img_1.png");

	// Now convert to a CMYK image
	jett_image cmyk_out;
	cmyk_out.createImage(rgb_in.getWidth(), rgb_in.getHeight(), image_cmyk);
	cmyk_out.set_profile_data(cmyk_test_profile_path());
	jett_transform t = r.build_transform(":srgb", cmyk_out, INTENT_PERCEPTUAL);
	r.bitblt(t, rgb_in, cmyk_out, 0, 0, 0);

	// Create the output images (one for each colour plane)
	jett_image outputs[4];
	outputs[0].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1);
	outputs[1].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1);
	outputs[2].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1);
	outputs[3].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), image_mono1);

	// Perform the dithering
	r.dither(cmyk_out, outputs, s, NULL);

	// Now save the results
	outputs[0].saveToFile(EXAMPLES_DIR "014 output_C.bmp");
	outputs[1].saveToFile(EXAMPLES_DIR "014 output_M.bmp");
	outputs[2].saveToFile(EXAMPLES_DIR "014 output_Y.bmp");
	outputs[3].saveToFile(EXAMPLES_DIR "014 output_K.bmp");
}

void example_15(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(825, 300, image_cmyk);
	image.set_profile_data(cmyk_test_profile_path());

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Load some images to compose on to the background
	jett_image image_diamond, image_lilly, image_teddy;

	image_diamond.loadFromFile(SRC_IMAGES "test_img_6.png");
	image_lilly.loadFromFile(SRC_IMAGES "test_img_7.png");
	image_teddy.loadFromFile(SRC_IMAGES "test_img_8.png");

	// We need a colour transform
	jett_transform t = r.build_transform(":srgb", image, INTENT_PERCEPTUAL);

	// Now compose a new image
	r.bitblt(t, image_diamond, image, 25, 25, bitblt_use_alpha);
	r.bitblt(t, image_lilly, image, 300, 25, bitblt_use_alpha);
	r.bitblt(t, image_teddy, image, 550, 25, bitblt_use_alpha);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "015 composition.tiff");
}

void example_16(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(825, 300, image_cmyk);
	image.set_profile_data(cmyk_test_profile_path());

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Load some images to compose on to the background
	jett_image image_background, image_lilly, image_teddy;

	image_background.loadFromFile(SRC_IMAGES "test_img_4.jpg");
	image_lilly.loadFromFile(SRC_IMAGES "test_img_7.png");
	image_teddy.loadFromFile(SRC_IMAGES "test_img_8.png");

	// We need a colour transform
	jett_transform t = r.build_transform(":srgb", image, INTENT_PERCEPTUAL);

	// Draw the background image
	r.bitblt(t, image_background, 0, 0, image_background.getWidth(), image_background.getHeight(), image, 0, 0, image.getWidth(), image.getHeight(), bitblt_cubic_scaling);

	// Now compose some images over the top...
	r.bitblt(t, image_lilly, image, 150, 25, bitblt_use_alpha);
	r.bitblt(t, image_teddy, image, 550, 25, bitblt_use_alpha);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "016 alpha channel.tiff");
}

void example_17(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(825, 300, image_cmyk);
	image.set_profile_data(cmyk_test_profile_path());

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Load some images to compose on to the background
	jett_image image_teddy;

	image_teddy.loadFromFile(SRC_IMAGES "test_img_8.png");

	// We need a colour transform
	jett_transform t = r.build_transform(":srgb", image, INTENT_PERCEPTUAL);

	// Define PI
	double PI = asin(1.0) * 2.0;

	// Roate the matrix (in radians)
	for (int i = 0; i < 8; ++i)
	{
		jett_matrix matrix;

		// Translate out a bit
		matrix.translate(jett_point(50, 0));

		// Now rotate
		matrix.rotate(2.0 * PI / 8.0 * i);

		// Move to the centre of the image
		matrix.translate(jett_point(image.getWidth() / 2, image.getHeight() / 2));

		// Now draw the image
		r.bitblt(t, image_teddy, 0, 0, image_teddy.getWidth(), image_teddy.getHeight(), image, matrix, bitblt_use_alpha);
	}

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "017 matrix bitblt.tiff");
}

void make_patch_image(jett &r, jett_image &image, int patch_width, unsigned char *colours, int number_of_patches)
{
	// The dimensions
	int patch_size = 50;
	int border = 75;
	int gap = 10;
	int width = (patch_width) * (patch_size + gap) + border * 2;
	int height = (number_of_patches / patch_width + 1) * (patch_size) + border * 2;

	image.createImage(width, height, image_cmyk);
	image.erase();

	// Now layout the patches
	for (int patch = 0; patch < number_of_patches; ++patch)
	{
		// Locate the patch
		int grid_x = patch % patch_width;
		int grid_y = patch / patch_width;

		int x = border + (patch_size + gap) * grid_x;
		int y = border + (patch_size)*grid_y;

		unsigned char *colour = colours + patch * 4;

		int brightness = (colour[0] + colour[1] + colour[2] + colour[3]);

		// Draw the patch
		r.rectangle(image, colour, x, y, patch_size, patch_size);

		if (brightness < 128 || colour[3] == 0)
		{
			unsigned char colour[4] = {0, 0, 0, 255};
			r.rectangle(image, colour, x + patch_size, y, gap, patch_size);
		}
	}

	// Add the text
	unsigned char colour[4] = {0, 0, 0, 255};
	jett_font f = r.create_font(FONT_FILE, 0, 20, true);
	for (int y = 0; y < (number_of_patches / patch_width) + 1; ++y)
	{
		char text[32];
		sprintf(text, "%d", y + 1);
		r.text(f, colour, image, border / 2, border + patch_size * y + patch_size / 2 + 10, text, string_rotate_0);
	}

	for (int x = 0; x < patch_width; ++x)
	{
		char text[32];
		sprintf(text, "%c", x + 'A');
		r.text(f, colour, image, border + x * (patch_size + gap) + (patch_size + gap) / 2 - 10, border - 4, text, string_rotate_0);
	}
}

void example_18(jett &r)
{
	// Create the image
	jett_image image_out;

	int patches_per_colour = 10;
	int number_of_colours = 4;
	int number_of_patches = patches_per_colour * number_of_colours;
	unsigned char *colours = new unsigned char[number_of_patches * number_of_colours];
	for (int patch = 0; patch < number_of_patches; ++patch)
	{
		unsigned char *colour = colours + patch * 4;
		colour[0] = 0;
		colour[1] = 0;
		colour[2] = 0;
		colour[3] = 0;

		int col = patch / patches_per_colour;
		int brightness = (patch % patches_per_colour) * 25.5;
		colour[col] = brightness;
	}
	make_patch_image(r, image_out, 14, colours, number_of_patches);

	jett_point p[5] = {
		jett_point(0, 0),
		jett_point(90, 56),
		jett_point(150, 40),
		jett_point(200, 100),
		jett_point(255, 200)};
	jett_linearization l = r.create_linearization();
	r.import_linearization(l, p, 5, 0);
	r.import_linearization(l, p, 5, 0);
	r.import_linearization(l, p, 5, 0);
	r.import_linearization(l, p, 5, 0);

	// Generate the dither screens using the void-and-cluster technique
	int sizes[] = {32, 33, 34, 35};
	jett_screens s = r.create_screens(4, sizes, 1.5, 256);

	// Now save the results
	image_out.saveToFile(EXAMPLES_DIR "018 No Linearization.tiff");

	// Now apply the linearization
	r.dither(image_out, s, l);

	// Now save the results
	image_out.saveToFile(EXAMPLES_DIR "018 With Linearization.tiff");

	delete[] colours;
}

void test_d_11(jett &r)
{
	/*
	 *	Check multilines work
	 */

	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(256, 256, image_cmyk);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a multi point line (unclosed)
	jett_point points[4];
	points[0] = jett_point(128, 128);
	points[1] = jett_point(128, 200);
	points[2] = jett_point(200, 200);
	points[3] = jett_point(200, 128);
	unsigned char cmyk_col[4] = {0, 255, 0, 0}; // (Magenta)
	r.lines(image, cmyk_col, 4, points, 4, false, line_best);

	// Draw same shape closed
	int dx = -100;
	int dy = -100;
	points[0] = jett_point(128 + dx, 128 + dy);
	points[1] = jett_point(128 + dx, 200 + dy);
	points[2] = jett_point(200 + dx, 200 + dy);
	points[3] = jett_point(200 + dx, 128 + dy);
	cmyk_col[0] = 255;
	cmyk_col[1] = 255;
	r.lines(image, cmyk_col, 4, points, 4, true, line_best);

	// Draw same shape, shifted, but with incorrect no of points
	/*
	dx = 0;
	dy = -100;
	points[0] = jett_point( 128+dx, 128+dy );
	points[1] = jett_point( 128+dx, 200+dy );
	points[2] = jett_point( 200+dx, 200+dy );
	points[3] = jett_point( 200+dx, 128+dy );
	r.lines(image, cmyk_col, 4, points, 32, true, 0 );
	*/

	// Draw shape so that part of it will be clipped
	dx = 100;
	dy = 100;
	points[0] = jett_point(128 + dx, 128 + dy);
	points[1] = jett_point(128 + dx, 200 + dy);
	points[2] = jett_point(200 + dx, 200 + dy);
	points[3] = jett_point(200 + dx, 128 + dy);
	cmyk_col[0] = 255;
	cmyk_col[1] = 0;
	r.lines(image, cmyk_col, 4, points, 4, true, line_best);

	// Draw large square so that only part of one leg will be in the drawing
	dx = 64;
	dy = -20;
	points[0] = jett_point(0 + dx, 0 + dy); // CRASHES HERE!
	points[1] = jett_point(500 + dx, 0 + dy);
	points[2] = jett_point(500 + dx, 500 + dy);
	points[3] = jett_point(0 + dx, 500 + dy);
	cmyk_col[0] = 255;
	cmyk_col[1] = 0;
	cmyk_col[2] = 255;
	r.lines(image, cmyk_col, 4, points, 4, true, line_best);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "011 test_d.tiff");
}

void test_bs_19(jett &r)
{
	/*
	 * test btlt with selective copy from source image
	 */

	// bitblt (jett_transform trans, jett_image &src_bitmap, int src_x1, int src_y1, int src_width, int src_height,
	//                              jett_image &dst_bitmap, int dst_x1, int dst_y1, int flags)
	jett_image image_base, image_over;
	int iflag[32] = {0, 1, 2, 3, 4, 5, 8, 9,
					 16, 17, 18, 19, 20, 21, 24, 25,
					 32, 33, 34, 35, 36, 37, 40, 41,
					 48, 49, 50, 51, 52, 53, 56, 57};

	for (int i = 0; i < 8; i++)
	{
		image_base.loadFromFile(_T(TEST_ASSET_DIR "test_img_9.tiff"));
		image_over.loadFromFile(_T(TEST_ASSET_DIR "test_img_10.tiff"));
		int over_width = image_over.getWidth();
		int over_height = image_over.getHeight();

		jett_transform t = NULL;

		// simple btblt
		r.bitblt(t, image_over, 0, 0, over_width, over_height, image_base, 100, 100, iflag[i]);

		// half of the overlay image
		r.bitblt(t, image_over, 0, 0, over_width / 2, over_height / 2, image_base, 100, 200, iflag[i]);

		// half of the overlay image with offset
		r.bitblt(t, image_over, 10, 20, over_width / 2, over_height / 2, image_base, 100, 300, iflag[i]);

		TCHAR filename[256];
		TCHAR name[50];
		_tcscpy_s(filename, _T(OUTPUT_DIR));
		_stprintf(name, _T("019 test_bs_%d.tiff"), iflag[i]);
		_tcscat_s(filename, name);
		image_base.saveToFile(filename);
	}
}

void test_bs_20(jett &r)
{
	/*
	 * test btlt with selective copy from source image. Case where not enough source image for destination
	 */
	jett_image image_base, image_over;
	image_base.loadFromFile(_T(TEST_ASSET_DIR "test_img_9.tiff"));
	image_over.loadFromFile(_T(TEST_ASSET_DIR "test_img_10.tiff"));
	int over_width = image_over.getWidth();
	int over_height = image_over.getHeight();

	// bitblt (jett_transform trans, jett_image &src_bitmap, int src_x1, int src_y1, int src_width, int src_height,
	//                              jett_image &dst_bitmap, int dst_x1, int dst_y1, int flags)

	jett_transform t = NULL;

	// simple btblt
	r.bitblt(t, image_over, 0, 0, over_width, over_height, image_base, 100, 100, 0);

	// full overlay image with offset
	r.bitblt(t, image_over, 10, 20, over_width, over_height, image_base, 100, 200, 0);
	image_base.saveToFile(EXAMPLES_DIR "020 test_bs.tiff");
}

void example_19(jett &r)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(825, 300, image_cmyk);
	image.set_profile_data(cmyk_test_profile_path());

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Load some images to compose on to the background
	jett_image image_background;

	image_background.loadFromFile(SRC_IMAGES "test_img_1.png");

	// We need a colour transform
	jett_transform t = r.build_transform(":srgb", image, INTENT_PERCEPTUAL);

	// Draw the background image
	image.set_clip_rect(0, 0, image.getWidth() - 50, image.getHeight() - 50);
	r.bitblt(t, image_background, 0, 0, image_background.getWidth(), image_background.getHeight(), image, 0, 0, image.getWidth(), image.getHeight(), bitblt_cubic_scaling | bitblt_rotate_0);

	// Save the bitmap for inspection
	image.saveToFile(EXAMPLES_DIR "019 clipping.tiff");
}
void build_rectangle(jett_point *points, int x, int y, int width, int height)
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

const TCHAR *image_type_as_string(image_t type)
{
	/*
	 * returns the type of image as a string
	 */

	switch (type)
	{
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

void test_d_12_draw_lines_over_patches(jett &r, unsigned char *cmyk_back, unsigned char *cmyk_line, int index, image_t imagetype)
{
	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(256, 256, imagetype);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a shaded background
	jett_point points[4];

	unsigned char colour[4] = {0, 255, 0, 0};

	for (int ix = 0; ix < 16; ix++)
	{
		for (int iy = 0; iy < 16; iy++)
		{
			for (int i = 0; i < 4; i++)
			{
				colour[i] = (cmyk_back[i] / 255) * ((ix * 16) + iy);
			}

			build_rectangle(points, 16 * ix, 16 * iy, 16, 16);
			r.polygon(image, colour, points, 4, polygon_fast);
		}
	}

	// Draw a set of anti-aliased lines over the top

	points[0] = jett_point(5, 5);
	points[1] = jett_point(255 - 5, 255 - 5);

	r.lines(image, cmyk_line, 4, points, 2, false, line_best);
	for (int i = 1; i < 8; i++)
	{
		int ix = i * 32;
		points[0] = jett_point(ix, 5);
		points[1] = jett_point(255 - 5, 255 - ix);
		r.lines(image, cmyk_line, 4, points, 2, false, line_best);
	}

	for (int i = 1; i < 8; i++)
	{
		int iy = i * 32;
		points[0] = jett_point(5, iy);
		points[1] = jett_point(255 - iy, 250);
		r.lines(image, cmyk_line, 4, points, 2, false, line_best);
	}

	// Save the bitmap for inspection
	TCHAR filename[256];
	TCHAR name[50];
	_tcscpy_s(filename, _T(OUTPUT_DIR));
	_tcscat_s(filename, image_type_as_string(imagetype));
	_stprintf(name, _T(" 012 test_d_%d.tiff"), index);
	_tcscat_s(filename, name);
	image.saveToFile(filename);
}

void set_colour(unsigned char *colour, unsigned char c, unsigned char m, unsigned char y, unsigned char k)
{
	colour[0] = c;
	colour[1] = m;
	colour[2] = y;
	colour[3] = k;
}

namespace
{
	struct GeneratedAssetSpec
	{
		const TCHAR *relative_path;
		const char *pillow_format;
		const char *pillow_mode;
		image_t expected_type;
		int expected_bit_depth;
		int width;
		int height;
		int max_diff;
		double max_average_diff;
	};

	output_string join_generated_path(const output_string &base, const output_string &name)
	{
		if (base.empty())
		{
			return name;
		}

		if (name.empty())
		{
			return base;
		}

		TCHAR last = base[base.size() - 1];
		if (last == _T('\\') || last == _T('/'))
		{
			return base + name;
		}

	#ifdef _WIN32
		return base + _T("\\") + name;
	#else
		return base + _T("/") + name;
	#endif
	}

	std::string narrow_path(const output_string &path)
	{
		std::string result;
		result.reserve(path.size());
		for (size_t i = 0; i < path.size(); ++i)
		{
			result.push_back(static_cast<char>(path[i]));
		}
		return result;
	}

	void throw_asset_error(const char *message)
	{
		throw jett_exception(JETT_INTERNAL_ERROR, 0, message);
	}

	void ensure_single_directory(const output_string &path)
	{
		if (path.empty())
		{
			return;
		}

		std::error_code error;
		if (fs::create_directory(fs::path(path), error) || !error || error == std::errc::file_exists)
		{
			return;
		}

		throw_asset_error("Unable to create generated asset directory");
	}

	void ensure_directory_tree(const output_string &path)
	{
		output_string current;
		for (size_t i = 0; i < path.size(); ++i)
		{
			current.push_back(path[i]);
			if (path[i] == _T('\\') || path[i] == _T('/'))
			{
				if (current.size() > 1)
				{
					ensure_single_directory(current.substr(0, current.size() - 1));
				}
			}
		}

		if (!path.empty() && path[path.size() - 1] != _T('\\') && path[path.size() - 1] != _T('/'))
		{
			ensure_single_directory(path);
		}
	}

	output_string generated_asset_path(const TCHAR *relative_path)
	{
		return join_generated_path(_T(TEST_ASSET_DIR), relative_path);
	}

	void ensure_generated_asset_parent(const TCHAR *relative_path)
	{
		output_string path = generated_asset_path(relative_path);
		size_t slash = path.find_last_of(_T("\\/"));
		if (slash != output_string::npos)
		{
			ensure_directory_tree(path.substr(0, slash));
		}
	}

	unsigned char clamp_byte(int value)
	{
		if (value < 0)
		{
			return 0;
		}
		if (value > 255)
		{
			return 255;
		}
		return static_cast<unsigned char>(value);
	}

	unsigned short expand_byte_to_u16(unsigned char value)
	{
		return static_cast<unsigned short>(value) * 257u;
	}

	void fill_rgb_scene(jett_image &image, int width, int height, bool image_16bpp = false)
	{
		image.createImage(width, height, image_rgb, image_16bpp);
		unsigned char *data = image.lockData(false);
		int stride = image.getStride();

		for (int y = 0; y < height; ++y)
		{
			unsigned char *row = data + y * stride;
			unsigned short *row16 = reinterpret_cast<unsigned short *>(row);
			for (int x = 0; x < width; ++x)
			{
				double nx = (static_cast<double>(x) / std::max(1, width - 1)) * 2.0 - 1.0;
				double ny = (static_cast<double>(y) / std::max(1, height - 1)) * 2.0 - 1.0;
				double radius = sqrt(nx * nx + ny * ny);
				int red = static_cast<int>(110 + 90 * sin((x + 20) * 0.018) + 55 * cos((y + 17) * 0.024));
				int green = static_cast<int>(115 + 85 * sin((x + y) * 0.011) + 45 * cos((y + 90) * 0.031));
				int blue = static_cast<int>(135 + 95 * cos((x - y) * 0.013) - 60 * radius);

				if (((x / 48) + (y / 48)) % 2 == 0)
				{
					red += 18;
					blue += 12;
				}

				if (radius < 0.35)
				{
					green += 55;
					blue += 40;
				}

				if ((x - width / 3) * (x - width / 3) + (y - height / 2) * (y - height / 2) < (width / 6) * (height / 6))
				{
					red += 50;
					green -= 25;
				}

				unsigned char red8 = clamp_byte(red);
				unsigned char green8 = clamp_byte(green);
				unsigned char blue8 = clamp_byte(blue);

				if (image_16bpp)
				{
					row16[x * 3 + 0] = expand_byte_to_u16(red8);
					row16[x * 3 + 1] = expand_byte_to_u16(green8);
					row16[x * 3 + 2] = expand_byte_to_u16(blue8);
				}
				else
				{
					row[x * 3 + 0] = red8;
					row[x * 3 + 1] = green8;
					row[x * 3 + 2] = blue8;
				}
			}
		}

		image.unlockData();
	}

	void fill_rgba_icon(jett_image &image, int variant, int width, int height)
	{
		image.createImage(width, height, image_rgba);
		unsigned char *data = image.lockData(false);
		int stride = image.getStride();

		for (int y = 0; y < height; ++y)
		{
			unsigned char *row = data + y * stride;
			for (int x = 0; x < width; ++x)
			{
				double fx = (static_cast<double>(x) / std::max(1, width - 1)) * 2.0 - 1.0;
				double fy = (static_cast<double>(y) / std::max(1, height - 1)) * 2.0 - 1.0;
				double alpha_shape = 0.0;
				int red = 0;
				int green = 0;
				int blue = 0;

				switch (variant)
				{
				case 0:
					alpha_shape = std::max(0.0, 1.0 - (fabs(fx) + fabs(fy)) * 1.15);
					red = 60 + static_cast<int>(150 * alpha_shape);
					green = 165 + static_cast<int>(60 * (1.0 - fabs(fy)));
					blue = 210;
					break;
				case 1:
				{
					double theta = atan2(fy, fx);
					double petal = fabs(sin(theta * 3.0));
					double radius = sqrt(fx * fx + fy * fy);
					alpha_shape = std::max(0.0, petal - radius * 0.85);
					red = 180 + static_cast<int>(60 * alpha_shape);
					green = 80 + static_cast<int>(140 * (1.0 - radius));
					blue = 140 + static_cast<int>(80 * petal);
				}
				break;
				default:
				{
					double head = 0.8 - sqrt(fx * fx + (fy + 0.1) * (fy + 0.1));
					double ear_left = 0.35 - sqrt((fx + 0.42) * (fx + 0.42) + (fy + 0.45) * (fy + 0.45));
					double ear_right = 0.35 - sqrt((fx - 0.42) * (fx - 0.42) + (fy + 0.45) * (fy + 0.45));
					alpha_shape = std::max(head, std::max(ear_left, ear_right)) * 1.8;
					if (alpha_shape < 0.0)
					{
						alpha_shape = 0.0;
					}
					red = 165 + static_cast<int>(55 * (1.0 - fabs(fx)));
					green = 105 + static_cast<int>(35 * alpha_shape);
					blue = 70 + static_cast<int>(30 * alpha_shape);
				}
				break;
				}

				int alpha = clamp_byte(static_cast<int>(255 * std::min(1.0, alpha_shape * 1.35)));
				row[x * 4 + 0] = clamp_byte(red);
				row[x * 4 + 1] = clamp_byte(green);
				row[x * 4 + 2] = clamp_byte(blue);
				row[x * 4 + 3] = static_cast<unsigned char>(alpha);
			}
		}

		image.unlockData();
	}

	void fill_cmyk_grid(jett_image &image, int width, int height, int cell_width, int cell_height)
	{
		image.createImage(width, height, image_cmyk);
		unsigned char *data = image.lockData(false);
		int stride = image.getStride();

		for (int y = 0; y < height; ++y)
		{
			unsigned char *row = data + y * stride;
			for (int x = 0; x < width; ++x)
			{
				int cell_x = x / std::max(1, cell_width);
				int cell_y = y / std::max(1, cell_height);
				int index = (cell_x + cell_y * 3) % 9;
				unsigned char c = static_cast<unsigned char>((index * 37 + x / 6) % 256);
				unsigned char m = static_cast<unsigned char>((index * 53 + y / 5) % 256);
				unsigned char yv = static_cast<unsigned char>((index * 29 + (x + y) / 7) % 256);
				unsigned char k = static_cast<unsigned char>((cell_x * 41 + cell_y * 27) % 180);

				if (x % cell_width < 2 || y % cell_height < 2)
				{
					k = 220;
				}

				if (((x + y) % 31) < 6)
				{
					c = clamp_byte(c + 28);
					m = clamp_byte(m / 2);
				}

				row[x * 4 + 0] = c;
				row[x * 4 + 1] = m;
				row[x * 4 + 2] = yv;
				row[x * 4 + 3] = k;
			}
		}

		image.unlockData();
	}

	void fill_cmyk_scene(jett_image &image, int width, int height)
	{
		image.createImage(width, height, image_cmyk);
		unsigned char *data = image.lockData(false);
		int stride = image.getStride();

		for (int y = 0; y < height; ++y)
		{
			unsigned char *row = data + y * stride;
			for (int x = 0; x < width; ++x)
			{
				double nx = static_cast<double>(x) / std::max(1, width - 1);
				double ny = static_cast<double>(y) / std::max(1, height - 1);
				row[x * 4 + 0] = clamp_byte(static_cast<int>(200 * nx + 35 * sin(y * 0.03)));
				row[x * 4 + 1] = clamp_byte(static_cast<int>(180 * ny + 30 * cos(x * 0.025)));
				row[x * 4 + 2] = clamp_byte(static_cast<int>(160 * (1.0 - nx) + 45 * sin((x + y) * 0.02)));
				row[x * 4 + 3] = clamp_byte(static_cast<int>(40 + 120 * fabs(nx - ny)));
			}
		}

		image.unlockData();
	}

	void fill_greyscale_gradient(jett_image &image, int width, int height)
	{
		image.createImage(width, height, image_mono);
		unsigned char *data = image.lockData(false);
		int stride = image.getStride();

		for (int y = 0; y < height; ++y)
		{
			unsigned char *row = data + y * stride;
			for (int x = 0; x < width; ++x)
			{
				int tone = static_cast<int>((255.0 * x) / std::max(1, width - 1));
				tone = clamp_byte(tone + static_cast<int>(25 * sin((x + y) * 0.04)));
				row[x] = static_cast<unsigned char>(tone);
			}
		}

		image.unlockData();
	}

	void fill_greyscale_bars(jett_image &image, int width, int height, int variant)
	{
		image.createImage(width, height, image_mono);
		unsigned char *data = image.lockData(false);
		int stride = image.getStride();

		for (int y = 0; y < height; ++y)
		{
			unsigned char *row = data + y * stride;
			for (int x = 0; x < width; ++x)
			{
				int tone = 0;
				switch (variant)
				{
				case 0:
					tone = ((x / 32) % 8) * 32;
					break;
				case 1:
					tone = ((y / 24) % 8) * 32;
					break;
				case 2:
					tone = (((x + y) / 28) % 8) * 32;
					break;
				default:
					tone = (((x * 2 + y) / 36) % 8) * 32;
					break;
				}
				row[x] = clamp_byte(tone + ((x + y) % 17));
			}
		}

		image.unlockData();
	}

	void build_cmy_grey_patch_fixture(jett &r, jett_image &image, int step)
	{
		const int patches_per_channel = 4;
		const int patch_width = 4;
		const int patch_count = patches_per_channel * 4;
		std::vector<unsigned char> colours(static_cast<size_t>(patch_count) * 4);

		for (int patch = 0; patch < patch_count; ++patch)
		{
			unsigned char *colour = &colours[patch * 4];
			colour[0] = 0;
			colour[1] = 0;
			colour[2] = 0;
			colour[3] = 0;

			int channel = patch / patches_per_channel;
			int level = (patch % patches_per_channel) * step;
			colour[channel] = clamp_byte(level);
		}

		make_patch_image(r, image, patch_width, &colours[0], patch_count);
	}

	void build_page_of_a_fixture(jett &r, jett_image &image)
	{
		image.createImage(720, 960, image_rgb);
		image.erase();

		unsigned char grid_colour[3] = {230, 230, 240};
		for (int y = 40; y < image.getHeight(); y += 80)
		{
			r.rectangle(image, grid_colour, 0, y, image.getWidth(), 2);
		}

		for (int x = 40; x < image.getWidth(); x += 80)
		{
			r.rectangle(image, grid_colour, x, 0, 2, image.getHeight());
		}

		unsigned char text_colour[3] = {15, 15, 15};
		jett_font font = r.create_font(FONT_FILE, 0, 22, false);
		for (int row = 0; row < 10; ++row)
		{
			for (int column = 0; column < 5; ++column)
			{
				char text[48];
				sprintf_s(text, "A%02d%02d AAAAA AAAA", row, column);
				r.text(font, text_colour, image, 90 + column * 120, 90 + row * 80, text, string_rotate_0);
			}
		}
		r.destroy_font(font);
	}

	void compare_generated_asset_pixels(const GeneratedAssetSpec &spec, jett_image &expected, jett_image &loaded)
	{
		if (loaded.getType() != spec.expected_type)
		{
			throw_asset_error("Generated asset reloaded as an unexpected image type");
		}

		if (loaded.is16bpp() != expected.is16bpp())
		{
			throw_asset_error("Generated asset reloaded with the wrong channel depth");
		}

		if (loaded.getWidth() != expected.getWidth() || loaded.getHeight() != expected.getHeight())
		{
			throw_asset_error("Generated asset reloaded with the wrong dimensions");
		}

		if (loaded.getPixelStride() != expected.getPixelStride())
		{
			throw_asset_error("Generated asset reloaded with the wrong pixel stride");
		}

		unsigned char *expected_data = expected.lockData(true);
		unsigned char *loaded_data = loaded.lockData(true);
		size_t bytes_per_channel = expected.is16bpp() ? 2u : 1u;
		size_t row_bytes = static_cast<size_t>(expected.getWidth()) * expected.getPixelStride() * bytes_per_channel;
		long long total_diff = 0;
		int max_diff = 0;
		size_t compared = 0;

		for (int y = 0; y < expected.getHeight(); ++y)
		{
			unsigned char *expected_row = expected_data + y * expected.getStride();
			unsigned char *loaded_row = loaded_data + y * loaded.getStride();
			for (size_t index = 0; index < row_bytes; ++index)
			{
				int diff = abs(static_cast<int>(expected_row[index]) - static_cast<int>(loaded_row[index]));
				total_diff += diff;
				max_diff = std::max(max_diff, diff);
				++compared;
			}
		}

		expected.unlockData();
		loaded.unlockData();

		double average_diff = compared == 0 ? 0.0 : static_cast<double>(total_diff) / compared;
		if (max_diff > spec.max_diff || average_diff > spec.max_average_diff)
		{
			throw_asset_error("Generated asset pixel content drifted beyond the allowed round-trip tolerance");
		}
	}

	void append_manifest_entry(FILE *manifest, const GeneratedAssetSpec &spec)
	{
		std::string relative_path = narrow_path(spec.relative_path);
		fprintf(manifest, "%s\t%s\t%s\t%d\t%d\t%d\n", relative_path.c_str(), spec.pillow_format, spec.pillow_mode, spec.expected_bit_depth, spec.width, spec.height);
	}

	void save_and_validate_generated_asset(FILE *manifest, const GeneratedAssetSpec &spec, jett_image &image)
	{
		ensure_generated_asset_parent(spec.relative_path);
		output_string path = generated_asset_path(spec.relative_path);
		image.saveToFile(path.c_str());

		jett_image loaded;
		loaded.loadFromFile(path.c_str());
		compare_generated_asset_pixels(spec, image, loaded);
		append_manifest_entry(manifest, spec);
	}

	void generate_generated_assets(jett &r)
	{
		ensure_directory_tree(_T(TEST_ASSET_DIR));
		output_string manifest_path = generated_asset_path(_T("generated_assets_manifest.tsv"));
		FILE *manifest = NULL;
		_tfopen_s(&manifest, manifest_path.c_str(), _T("wb"));
		if (!manifest)
		{
			throw_asset_error("Unable to create the generated asset manifest");
		}

		fprintf(manifest, "path\tformat\tmode\tbit_depth\twidth\theight\n");

		try
		{
			jett_image rgb_scene;
			fill_rgb_scene(rgb_scene, 512, 512);
			GeneratedAssetSpec lenna_png = {_T("test_img_1.png"), "PNG", "RGB", image_rgb, 8, 512, 512, 0, 0.0};
			save_and_validate_generated_asset(manifest, lenna_png, rgb_scene);

			jett_image rgb_scene_16;
			fill_rgb_scene(rgb_scene_16, 512, 512, true);
			GeneratedAssetSpec lenna16_png = {_T("test_img_2.png"), "PNG", "RGB", image_rgb, 16, 512, 512, 0, 0.0};
			save_and_validate_generated_asset(manifest, lenna16_png, rgb_scene_16);

			GeneratedAssetSpec rgb_jpeg = {_T("test_img_3.jpeg"), "JPEG", "RGB", image_rgb, 8, 512, 512, 72, 4.0};
			save_and_validate_generated_asset(manifest, rgb_jpeg, rgb_scene);

			jett_image background_scene;
			fill_rgb_scene(background_scene, 900, 540);
			GeneratedAssetSpec background_jpeg = {_T("test_img_4.jpg"), "JPEG", "RGB", image_rgb, 8, 900, 540, 72, 4.0};
			save_and_validate_generated_asset(manifest, background_jpeg, background_scene);

			jett_image alpha_test;
			fill_rgba_icon(alpha_test, 0, 256, 256);
			GeneratedAssetSpec alpha_png = {_T("test_img_5.png"), "PNG", "RGBA", image_rgba, 8, 256, 256, 0, 0.0};
			save_and_validate_generated_asset(manifest, alpha_png, alpha_test);

			jett_image diamond_icon;
			fill_rgba_icon(diamond_icon, 0, 220, 220);
			GeneratedAssetSpec diamond_png = {_T("test_img_6.png"), "PNG", "RGBA", image_rgba, 8, 220, 220, 0, 0.0};
			save_and_validate_generated_asset(manifest, diamond_png, diamond_icon);

			jett_image lily_icon;
			fill_rgba_icon(lily_icon, 1, 220, 220);
			GeneratedAssetSpec lily_png = {_T("test_img_7.png"), "PNG", "RGBA", image_rgba, 8, 220, 220, 0, 0.0};
			save_and_validate_generated_asset(manifest, lily_png, lily_icon);

			jett_image teddy_icon;
			fill_rgba_icon(teddy_icon, 2, 220, 220);
			GeneratedAssetSpec teddy_png = {_T("test_img_8.png"), "PNG", "RGBA", image_rgba, 8, 220, 220, 0, 0.0};
			save_and_validate_generated_asset(manifest, teddy_png, teddy_icon);

			jett_image medium_grid;
			fill_cmyk_grid(medium_grid, 640, 480, 64, 48);
			GeneratedAssetSpec medium_tiff = {_T("test_img_9.tiff"), "TIFF", "CMYK", image_cmyk, 8, 640, 480, 0, 0.0};
			save_and_validate_generated_asset(manifest, medium_tiff, medium_grid);

			jett_image small_grid;
			fill_cmyk_grid(small_grid, 192, 144, 24, 24);
			GeneratedAssetSpec small_tiff = {_T("test_img_10.tiff"), "TIFF", "CMYK", image_cmyk, 8, 192, 144, 0, 0.0};
			save_and_validate_generated_asset(manifest, small_tiff, small_grid);

			jett_image cmyk_jpeg_scene;
			fill_cmyk_scene(cmyk_jpeg_scene, 384, 256);
			GeneratedAssetSpec cmyk_jpeg = {_T("test_img_11.jpg"), "JPEG", "CMYK", image_cmyk, 8, 384, 256, 84, 6.0};
			save_and_validate_generated_asset(manifest, cmyk_jpeg, cmyk_jpeg_scene);

			jett_image patch_l64;
			build_cmy_grey_patch_fixture(r, patch_l64, 64);
			GeneratedAssetSpec patch64_tiff = {_T("test_img_12.tiff"), "TIFF", "CMYK", image_cmyk, 8, patch_l64.getWidth(), patch_l64.getHeight(), 0, 0.0};
			save_and_validate_generated_asset(manifest, patch64_tiff, patch_l64);

			jett_image patch_l32;
			build_cmy_grey_patch_fixture(r, patch_l32, 32);
			GeneratedAssetSpec patch32_tiff = {_T("test_img_13.tiff"), "TIFF", "CMYK", image_cmyk, 8, patch_l32.getWidth(), patch_l32.getHeight(), 0, 0.0};
			save_and_validate_generated_asset(manifest, patch32_tiff, patch_l32);

			jett_image greyscale;
			fill_greyscale_gradient(greyscale, 512, 256);
			GeneratedAssetSpec greyscale_tiff = {_T("test_img_14.tiff"), "TIFF", "L", image_mono, 8, 512, 256, 0, 0.0};
			save_and_validate_generated_asset(manifest, greyscale_tiff, greyscale);

			jett_image greyscale_bar;
			fill_greyscale_bars(greyscale_bar, 512, 256, 0);
			GeneratedAssetSpec greyscale_bar_tiff = {_T("test_img_15.tiff"), "TIFF", "L", image_mono, 8, 512, 256, 0, 0.0};
			save_and_validate_generated_asset(manifest, greyscale_bar_tiff, greyscale_bar);

			jett_image greyscale_bar1;
			fill_greyscale_bars(greyscale_bar1, 512, 256, 1);
			GeneratedAssetSpec greyscale_bar1_tiff = {_T("test_img_16.tiff"), "TIFF", "L", image_mono, 8, 512, 256, 0, 0.0};
			save_and_validate_generated_asset(manifest, greyscale_bar1_tiff, greyscale_bar1);

			jett_image greyscale_bar2;
			fill_greyscale_bars(greyscale_bar2, 512, 256, 2);
			GeneratedAssetSpec greyscale_bar2_tiff = {_T("test_img_17.tiff"), "TIFF", "L", image_mono, 8, 512, 256, 0, 0.0};
			save_and_validate_generated_asset(manifest, greyscale_bar2_tiff, greyscale_bar2);

			jett_image greyscale_bar3;
			fill_greyscale_bars(greyscale_bar3, 512, 256, 3);
			GeneratedAssetSpec greyscale_bar3_tiff = {_T("test_img_18.tiff"), "TIFF", "L", image_mono, 8, 512, 256, 0, 0.0};
			save_and_validate_generated_asset(manifest, greyscale_bar3_tiff, greyscale_bar3);

			jett_image page_of_a;
			build_page_of_a_fixture(r, page_of_a);
			GeneratedAssetSpec page_tiff = {_T("test_img_19.tiff"), "TIFF", "RGB", image_rgb, 8, page_of_a.getWidth(), page_of_a.getHeight(), 0, 0.0};
			save_and_validate_generated_asset(manifest, page_tiff, page_of_a);

			jett_image bmp_reference;
			fill_greyscale_bars(bmp_reference, 320, 192, 0);
			GeneratedAssetSpec bmp_spec = {_T("test_img_20.bmp"), "BMP", "L", image_mono, 8, 320, 192, 0, 0.0};
			save_and_validate_generated_asset(manifest, bmp_spec, bmp_reference);

			fclose(manifest);
		}
		catch (...)
		{
			fclose(manifest);
			throw;
		}
	}

	int run_python_generated_asset_validation()
	{
	#ifdef _WIN32
		struct python_command_t
		{
			const char *check;
			const char *run;
		};

		const python_command_t commands[] = {
			{"python3 --version >nul 2>&1", "python3 GraphicsTest/verify_generated_images.py test_images/generated_assets_manifest.tsv"},
			{"py -3 --version >nul 2>&1", "py -3 GraphicsTest/verify_generated_images.py test_images/generated_assets_manifest.tsv"},
			{"python --version >nul 2>&1", "python GraphicsTest/verify_generated_images.py test_images/generated_assets_manifest.tsv"}
		};
	#else
		struct python_command_t
		{
			const char *check;
			const char *run;
		};

		const python_command_t commands[] = {
			{"python3 --version >/dev/null 2>&1", "python3 GraphicsTest/verify_generated_images.py test_images/generated_assets_manifest.tsv"},
			{"python --version >/dev/null 2>&1", "python GraphicsTest/verify_generated_images.py test_images/generated_assets_manifest.tsv"}
		};
	#endif

		for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i)
		{
			if (system(commands[i].check) == 0)
			{
				return system(commands[i].run);
			}
		}

		return -1;
	}

	int run_generated_asset_tests()
	{
		printf("RUN generated_asset_tests\n");
		fflush(stdout);

		try
		{
			jett r;
			r.init(false);
			generate_generated_assets(r);
			int python_validation = run_python_generated_asset_validation();
			if (python_validation < 0)
			{
				printf("SKIP generated_asset_tests: Python interpreter not available for image validation\n");
				return 0;
			}

			if (python_validation != 0)
			{
				throw_asset_error("Python validation of generated assets failed");
			}

			printf("GENERATED ASSET TESTS COMPLETE failures=0\n");
			return 0;
		}
		catch (const jett_exception &e)
		{
			printf("FAIL generated_asset_tests: %s (code=%d subsys=%d)\n", e.m_message, e.m_code, e.m_subsys_code);
			return 1;
		}
		catch (...)
		{
			printf("FAIL generated_asset_tests: unknown exception\n");
			return 1;
		}
	}

}

void test_d_12(jett &r, image_t imagetype)
{
	/*
	 * Check anti-aliasing over different shades of background
	 */

	unsigned char cmyk_back[4] = {0, 255, 0, 0};
	unsigned char cmyk_line[4] = {255, 0, 0, 0};
	test_d_12_draw_lines_over_patches(r, cmyk_back, cmyk_line, 1, imagetype);

	set_colour(cmyk_line, 0, 0, 0, 0);
	test_d_12_draw_lines_over_patches(r, cmyk_back, cmyk_line, 2, imagetype);

	set_colour(cmyk_back, 0, 255, 255, 0);
	set_colour(cmyk_line, 255, 0, 0, 0);
	test_d_12_draw_lines_over_patches(r, cmyk_back, cmyk_line, 3, imagetype);

	set_colour(cmyk_back, 255, 255, 255, 0);
	test_d_12_draw_lines_over_patches(r, cmyk_back, cmyk_line, 4, imagetype);

	set_colour(cmyk_back, 0, 0, 0, 255);
	test_d_12_draw_lines_over_patches(r, cmyk_back, cmyk_line, 5, imagetype);

	set_colour(cmyk_back, 255, 255, 255, 255);
	test_d_12_draw_lines_over_patches(r, cmyk_back, cmyk_line, 6, imagetype);

	set_colour(cmyk_back, 255, 0, 0, 0);
	test_d_12_draw_lines_over_patches(r, cmyk_back, cmyk_line, 7, imagetype);
}

void test_d_1(jett &r, image_t imagetype)
{
	/*
	 *	Check line clipping works for line
	 */

	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(256, 256, imagetype);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a lines on the edge of the image
	jett_point points[2];
	points[0] = jett_point(0, 0);
	points[1] = jett_point(255, 0);
	unsigned char colour[4] = {0, 255, 0, 0}; // (Magenta)
#if 1
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(255, 0);
	points[1] = jett_point(255, 255);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(255, 255);
	points[1] = jett_point(0, 255);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(0, 255);
	points[1] = jett_point(0, 0);
	r.lines(image, colour, 1, points, 2, false, 0);

	// Draw a lines one pixel in from the edge of the image
	colour[0] = 255; // blue
	points[0] = jett_point(1, 1);
	points[1] = jett_point(254, 1);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(254, 1);
	points[1] = jett_point(254, 254);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(254, 254);
	points[1] = jett_point(1, 254);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(1, 254);
	points[1] = jett_point(1, 1);
	r.lines(image, colour, 1, points, 2, false, 0);

	// Draw a lines two pixels in from the edge of the image
	colour[1] = 0; //
	points[0] = jett_point(2, 2);
	points[1] = jett_point(253, 2);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(253, 2);
	points[1] = jett_point(253, 253);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(253, 253);
	points[1] = jett_point(2, 253);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(2, 253);
	points[1] = jett_point(2, 2);
	r.lines(image, colour, 1, points, 2, false, 0);

#endif

	// Draw lines one pixel outside the edge of the image
	colour[1] = 0; //
	colour[0] = 0;
	colour[2] = 255;
	points[0] = jett_point(-1, -1);
	points[1] = jett_point(256, -1);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(256, -1);
	points[1] = jett_point(256, 256);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(256, 256);
	points[1] = jett_point(-1, 256);
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(-1, 256);
	points[1] = jett_point(-1, -1);
	r.lines(image, colour, 1, points, 2, false, 0);

	/*
	// Draw a second line out of bounds
	points[0] = jett_point( 0,0 );
	points[1] = jett_point( 5000, 5000 );
	colour[0] = 255;  // (Cyan)
	colour[1] = 0;
	r.lines(image, colour, 1, points, 2, false, 0 );

	// Draw a third line out of bounds the other way
	points[0] = jett_point( 255,0 );
	points[1] = jett_point( -5000+255, 5000 );
	r.lines(image, colour, 1, points, 2, false, 0 );

	// Draw line out of bounds - check clipping is correct and slope is correct
	points[0] = jett_point( 0, 128 );
	points[1] = jett_point( 5000, 5128 );
	r.lines(image, colour, 1, points, 2, false, 0 );

	// Draw line with both points out of bounds
	points[0] = jett_point( -64, 0 );
	points[1] = jett_point( 936, 1000 );
	colour[0] = 255;
	colour[1] = 255;
	r.lines(image, colour, 1, points, 2, false, 0 );

	// Draw a line that does not cross drawing area
	points[0] = jett_point( -64, 0 );
	points[1] = jett_point( -64, 1000 );
	r.lines(image, colour, 1, points, 2, false, 0 );

	// Draw final line
	points[0] = jett_point( 150, 150 );
	points[1] = jett_point( 200, 200 );
	r.lines(image, colour, 1, points, 2, false, 0 );
	*/

	// Save the bitmap for inspection
	TCHAR filename[256];
	_tcscpy_s(filename, _T(OUTPUT_DIR));
	_tcscat_s(filename, image_type_as_string(imagetype));
	_tcscat_s(filename, _T(" 001 test_d.tiff"));
	image.saveToFile(filename);
}

void test_f_57(jett &r)
{
	/*
	 * test font clipping at different rotations and positions
	 *
	 */
	TCHAR filename[256];
	TCHAR name[50];
	TCHAR text[250];

	int size = 40;
	bool aliasing[] = {true, false};
	jett_font f;

	// Create the bitmap we are going to draw on
	jett_image image[8];

	// We need a colour to draw with
	unsigned char colour[] = {0, 0, 0, 255};

	for (int aliase = 0; aliase < 2; aliase++)
	{
		int iy = 10;

		// We need a font to draw with
		f = r.create_font(FONT_FILE, 0, size, aliasing[aliase]);

		// create background for each rotation
		for (int irot = 0; irot < 4; irot++)
		{
			image[irot].loadFromFile(_T(TEST_ASSET_DIR "test_img_9.tiff"));
		}

		// Draw some text
		_stprintf(text, _T("A One small step for man"));
		int cx = image[0].getWidth() / 2;
		int cy = image[0].getHeight() / 2;

		r.text(f, colour, image[0], cx, cy, text, string_rotate_0); // center
		text[0]++;
		r.text(f, colour, image[0], cx, size / 2, text, string_rotate_0); // mid top
		text[0]++;
		r.text(f, colour, image[0], -100, cy - 2 * size, text, string_rotate_0); // far left center
		text[0]++;
		r.text(f, colour, image[0], 2 * cx + 100, cy + size + 10, text, string_rotate_0); // far right center
		text[0]++;
		r.text(f, colour, image[0], cx, 2 * cy + size / 2, text, string_rotate_0); // mid bottom center
		text[0]++;
		r.text(f, colour, image[0], -200, -200, text, string_rotate_0); // offsite
		text[0]++;

		r.text(f, colour, image[1], cx, cy, text, string_rotate_90); // center
		text[0]++;
		r.text(f, colour, image[1], 2 * cx - size / 2, cy, text, string_rotate_90); // mid right
		text[0]++;
		r.text(f, colour, image[1], -size / 2, cy, text, string_rotate_90); // far left center
		text[0]++;
		r.text(f, colour, image[1], cx + 2 * size, -100, text, string_rotate_90); // top
		text[0]++;
		// r.text(f, colour, image[1], cx, 2*cy+size/2, text, string_rotate_90);   // mid bottom center
		r.text(f, colour, image[1], -200, -200, text, string_rotate_90); // offsite
		text[0]++;

		r.text(f, colour, image[2], cx, cy, text, string_rotate_180); // center
		text[0]++;
		r.text(f, colour, image[2], cx, 2 * cy - size / 2, text, string_rotate_180); // mid bottom
		text[0]++;
		// r.text(f, colour, image[2], 2*cx + 100, cy- 2 *size, text, string_rotate_180);  // far left center
		r.text(f, colour, image[2], 2 * cx + 100, cy + size + 10, text, string_rotate_180); // far right center
		text[0]++;
		r.text(f, colour, image[2], cx, -size / 2, text, string_rotate_180); // mid top center
		text[0]++;
		r.text(f, colour, image[2], -200, -200, text, string_rotate_0); // offsite
		text[0]++;

		r.text(f, colour, image[3], cx, cy, text, string_rotate_270); // center
		text[0]++;
		r.text(f, colour, image[3], 2 * cx + size / 2, cy, text, string_rotate_270); // mid right
		text[0]++;
		r.text(f, colour, image[3], size / 2, cy, text, string_rotate_270); // far left center
		text[0]++;
		r.text(f, colour, image[3], cx + 2 * size, 2 * cx + 100, text, string_rotate_270); // top
		text[0]++;
		// r.text(f, colour, image[1], cx, 2*cy+size/2, text, string_rotate_90);   // mid bottom center
		r.text(f, colour, image[3], -200, -200, text, string_rotate_270); // offsite
		text[0]++;

		for (int irot = 0; irot < 4; irot++)
		{
			// save to file
			_tcscpy_s(filename, _T(OUTPUT_DIR));
			_stprintf(name, _T("057 test_f_%d_%d.tiff"), aliase, irot);
			_tcscat_s(filename, name);
			image[irot].saveToFile(filename);
		}
	}
}

void test_bs_21(jett &r)
{
	/*
	 * test bitblt with selective copy from source image. Negative offset in src image
	 */

	// bitblt (jett_transform trans, jett_image &src_bitmap, int src_x1, int src_y1, int src_width, int src_height,
	//                              jett_image &dst_bitmap, int dst_x1, int dst_y1, int flags)

	jett_image image_base, image_over;
	int iflag[32] = {0, 1, 2, 3, 4, 5, 8, 9,
					 16, 17, 18, 19, 20, 21, 24, 25,
					 32, 33, 34, 35, 36, 37, 40, 41,
					 48, 49, 50, 51, 52, 53, 56, 57};

	for (int i = 0; i < 8; i++)
	{
		image_base.loadFromFile(_T(TEST_ASSET_DIR "test_img_9.tiff"));
		image_over.loadFromFile(_T(TEST_ASSET_DIR "test_img_10.tiff"));
		int over_width = image_over.getWidth();
		int over_height = image_over.getHeight();

		jett_transform t = NULL;

		// simple btblt
		r.bitblt(t, image_over, 0, 0, over_width, over_height, image_base, 100, 100, iflag[i]);

		// full overlay image with offset
		r.bitblt(t, image_over, -5, -5, over_width, over_height, image_base, 100, 200, iflag[i]);

		TCHAR filename[256];
		TCHAR name[50];
		_tcscpy_s(filename, _T(OUTPUT_DIR));
		_stprintf(name, _T("021 test_bs_%d.tiff"), iflag[i]);
		_tcscat_s(filename, name);
		image_base.saveToFile(filename);
	}
}

void test_s_68(jett &r)
{
	/*
	 *	test screening of an image with different depth screeners and different mono image bit depths
	 *	This is a simplification of test 49 which crashes
	 */

	TCHAR filename[256];
	TCHAR name[50];
	printf("Starting test 68\n");

	// Generate the dither screens using the void-and-cluster technique
	int sizes[] = {32, 33, 34, 35};
	int screen_depth = 2;
	jett_screens s = r.create_screens(4, sizes, 1.5, 2);

	image_t imgmode = image_mono1;
	int image_depth = 2;

	// Load an RGB images
	jett_image rgb_in;
	rgb_in.loadFromFile(IMAGES_DIR "test_img_1.png");

	// Now convert to a CMYK image
	jett_image cmyk_out;
	cmyk_out.createImage(rgb_in.getWidth(), rgb_in.getHeight(), image_cmyk);
	cmyk_out.set_profile_data(cmyk_test_profile_path());
	jett_transform t = r.build_transform(":srgb", cmyk_out, INTENT_PERCEPTUAL);
	r.bitblt(t, rgb_in, cmyk_out, 0, 0, 0);

	// Create the output images
	jett_image output[4];

	// only do dither if there are enough levels in the output image
	if (screen_depth <= image_depth)
	{
		for (int i = 0; i < 4; i++)
		{
			output[i].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), imgmode);
		}
	}

	// Perform the dithering
	try
	{
		r.dither(cmyk_out, output, s, NULL); // GPU crashes here
	}
	catch (jett_exception e)
	{
		printf("   Test 68, r.dither exception: %s, %d \n", e.m_message, e.m_code);
		return;
	}

	for (int icolour = 0; icolour < 4; icolour++)
	{
		_tcscpy_s(filename, _T(OUTPUT_DIR));
		_stprintf(name, _T("068 test_s_screener_%d.bmp"), icolour);
		_tcscat_s(filename, name);

		output[icolour].saveToFile(filename);
	}
}

void test_s_49(jett &r)
{
	/*
	 *	test screening of an image with different depth screeners and different mono image bit depths
	 *
	 */

	TCHAR filename[256];
	TCHAR name[50];
	printf("Starting test 49\n");

	// Generate the dither screens using the void-and-cluster technique
	int sizes[] = {32, 33, 34, 35};
	int screen_depth[] = {2, 4, 8};
	jett_screens s[] = {r.create_screens(4, sizes, 1.5, 2),
						r.create_screens(4, sizes, 1.5, 4),
						r.create_screens(4, sizes, 1.5, 8)};
	image_t imgmode[] = {image_mono1, image_mono2, image_mono4, image_mono};
	int image_depth[] = {2, 4, 16, 256};

	// Load an RGB images
	jett_image rgb_in;
	rgb_in.loadFromFile(IMAGES_DIR "test_img_1.png");

	// Now convert to a CMYK image
	jett_image cmyk_out;
	cmyk_out.createImage(rgb_in.getWidth(), rgb_in.getHeight(), image_cmyk);
	cmyk_out.set_profile_data(cmyk_test_profile_path());
	jett_transform t = r.build_transform(":srgb", cmyk_out, INTENT_PERCEPTUAL);
	r.bitblt(t, rgb_in, cmyk_out, 0, 0, 0);

	// Create the output images, screener depth x mono bit depth x colour
	jett_image outputs[3][4][4];

	// for every screener level x output depth
	for (int i_screen_depth = 0; i_screen_depth < 3; i_screen_depth++)
	{
		for (int i_mono_depth = 0; i_mono_depth < 4; i_mono_depth++)
		{
			// only do dither if there are enough levels in the output image
			if (screen_depth[i_screen_depth] <= image_depth[i_mono_depth])
			{
				for (int icolour = 0; icolour < 4; icolour++)
				{
					outputs[i_screen_depth][i_mono_depth][icolour].createImage(cmyk_out.getWidth(), cmyk_out.getHeight(), imgmode[i_mono_depth]);
				}

				// Perform the dithering
				try
				{
					int scale = image_depth[i_mono_depth] / screen_depth[i_screen_depth];
					r.dither(cmyk_out, outputs[i_screen_depth][i_mono_depth], s[i_screen_depth], NULL, scale);
				}
				catch (jett_exception e)
				{
					printf("   Test 49, r.dither exception: %s, %d \n", e.m_message, e.m_code);
					return;
				}
				for (int icolour = 0; icolour < 4; icolour++)
				{
					_tcscpy_s(filename, _T(OUTPUT_DIR));
					_stprintf(name, _T("049 test_s_screener_%d_%d_%d.bmp"), screen_depth[i_screen_depth], image_depth[i_mono_depth], icolour);
					_tcscat_s(filename, name);

					outputs[i_screen_depth][i_mono_depth][icolour].saveToFile(filename);
				}
			}
		}
	}
}

void screen_inplace(jett &r, jett_screens s[], const TCHAR *filein, const TCHAR *fileoutroot)
{
	/*
	 * test screener in place function:
	 * dither (jett_image &src_image, jett_screens screens, jett_linearization linearization, int scale=0)
	 *
	 */

	TCHAR filename[256];
	TCHAR name[50];
	jett_image image_in;
	jett_image cmyk_out;

	int screen_depth[] = {2, 4, 8};

	try
	{
		for (int idepth = 0; idepth < 3; idepth++)
		{
			// Load an images
			image_in.loadFromFile(filein);
			cmyk_out.createImage(image_in.getWidth(), image_in.getHeight(), image_cmyk);
			cmyk_out.set_profile_data(cmyk_test_profile_path());

			// if RGB convert to CMYK
			if (image_in.getType() == image_rgb)
			{
				jett_transform t = r.build_transform(":srgb", cmyk_out, INTENT_PERCEPTUAL);
				r.bitblt(t, image_in, cmyk_out, 0, 0, 0);
				image_in.discard();
			}
			else
			{
				r.bitblt(NULL, image_in, cmyk_out, 0, 0, 0);
			}

			// Perform the dithering
			r.dither(cmyk_out, s[idepth], NULL, 0);

			// Now save the results
			_tcscpy_s(filename, _T(OUTPUT_DIR));
			_tcscat_s(filename, fileoutroot);
			_stprintf(name, _T("%d.tiff"), screen_depth[idepth]);
			_tcscat_s(filename, name);
			cmyk_out.saveToFile(filename);
		}
	}
	catch (jett_exception e)
	{
		printf("\n jett_exception thrown: %d, %s", e.m_code, e.m_message);
	}
}

void test_s_51(jett &r)
{
	/*
	 * test screener. CMY grey images. Look for structure, banding etc
	 *
	 */

	int sizes[] = {32, 33, 34, 35};
	jett_screens s[] = {r.create_screens(4, sizes, 1.5, 2),
						r.create_screens(4, sizes, 1.5, 4),
						r.create_screens(4, sizes, 1.5, 8)};
	printf("Starting test 51\n");

	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_12.tiff"), _T("051 test_s_img_12_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_13.tiff"), _T("051 test_s_img_13_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_1.png"), _T("051 test_s_img_1_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_1.png"), _T("051 test_s_img_1_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_14.tiff"), _T("051 test_s_img_14_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_15.tiff"), _T("051 test_s_img_15_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_16.tiff"), _T("051 test_s_img_16_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_17.tiff"), _T("051 test_s_img_17_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_18.tiff"), _T("051 test_s_img_18_"));
	screen_inplace(r, s, _T(TEST_ASSET_DIR "test_img_19.tiff"), _T("051 test_s_img_19_"));
}

void test_d_7(jett &r, image_t imagetype)
{
	/*
	 *	Test line antialising
	 */

	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(256, 256, imagetype);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	jett_point points[4];
	unsigned char colour[4] = {0, 255, 0, 0};

	// r.rectangle(image, colour, 0, 0, 128, 128);

	// draw plain line
	points[0] = jett_point(0, 0);
	points[1] = jett_point(256, 256);
	colour[1] = 0;
	colour[3] = 255;
	// r.lines(image, colour, 4, points, 2, false, 0 );

	// draw anti-aliased line
	points[0] = jett_point(20, 0);
	points[1] = jett_point(276, 256);
	r.lines(image, colour, 4, points, 2, false, line_best);

	// Save the bitmap for inspection
	TCHAR filename[256];
	_tcscpy_s(filename, _T(OUTPUT_DIR));
	_tcscat_s(filename, image_type_as_string(imagetype));
	_tcscat_s(filename, _T(" 007 test_d.tiff"));
	image.saveToFile(filename);
}

void test_d_9(jett &r, image_t imagetype)
{
	/*
	 *	Check line widths are correct with anti-aliasing
	 */

	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(256, 256, imagetype);
	image.set_clip_rect(60, 60, 100, 100);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a simple line segments of different widths
	jett_point points[2];
	unsigned char colour[4] = {0, 255, 0, 0}; // (Magenta)

	TCHAR id[2];
	id[0] = 'A';
	id[1] = 0;
	for (int i = 0; i < 10; i++)
	{
		int ix = 10 + i * 25;
		float width = (i + 1);

		CTimer t1;
		t1.start();

		points[0] = jett_point(ix, 10);
		points[1] = jett_point(ix, 200);
		r.lines(image, colour, width, points, 2, false, line_best);

		points[0] = jett_point(ix, 210);
		points[1] = jett_point(ix, 220);
		r.lines(image, colour, 1.0, points, 2, false, 0);

		printf("%llu\n", static_cast<unsigned long long>(t1.elapsed()));

		TCHAR filename[256];
		_tcscpy_s(filename, _T(OUTPUT_DIR));
		_tcscat_s(filename, image_type_as_string(imagetype));
		_tcscat_s(filename, id);
		_tcscat_s(filename, _T(" 009 test_d.tiff"));
		image.saveToFile(filename);
		++id[0];
	}

	// Draw a diagonal so we can see pixel sizes
	points[0] = jett_point(0, 256);
	points[1] = jett_point(256, 0);
	colour[0] = 255;
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(0, 0);
	points[1] = jett_point(256, 256);
	r.lines(image, colour, 1, points, 2, false, 0);

	// Save the bitmap for inspection
	TCHAR filename[256];
	_tcscpy_s(filename, _T(OUTPUT_DIR));
	_tcscat_s(filename, image_type_as_string(imagetype));
	_tcscat_s(filename, _T(" 009 test_d.tiff"));
	image.saveToFile(filename);
}

void test_d_3(jett &r, image_t imagetype)
{
	/*
	 *	Check line widths are correct
	 */

	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(256, 256, imagetype);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a simple line segments of different widths
	jett_point points[2];
	unsigned char colour[4] = {0, 255, 0, 0}; // (Magenta)

#if 0

	for (int i = 0; i < 10; i++)
	{
		int ix = 10 + i * 20;
		float width = (i + 1);
		points[0] = jett_point(ix, 10);
		points[1] = jett_point(ix, 200);
		r.lines(image, colour, width, points, 2, false, 0);

		points[0] = jett_point(ix, 210);
		points[1] = jett_point(ix, 220);
		r.lines(image, colour, 1.0, points, 2, false, 0);
	}

#endif

	// Draw a diagonal so we can see pixel sizes
	points[0] = jett_point(0, 256);
	points[1] = jett_point(256, 0);
	colour[0] = 255;
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(0, 0);
	points[1] = jett_point(256, 256);
	// r.lines(image, colour, 1, points, 2, false, 0 );

	// Save the bitmap for inspection
	TCHAR filename[256];
	_tcscpy_s(filename, _T(OUTPUT_DIR));
	_tcscat_s(filename, image_type_as_string(imagetype));
	_tcscat_s(filename, _T(" 003 test_d.tiff"));
	image.saveToFile(filename);
}

void draw_wagon_wheel(jett &r, jett_image *image, int cx, int cy, int linewidth, int antialias)
{
	// draw a waggon wheel for checking anti-aliased line behaviour

	const double PI = asin(1.0) * 2.0;

	float length = 60;
	unsigned char colour[4] = {0, 255, 0, 0}; // Magenta
	int inner_radius = 20;
	int outer_radius = 60;

	jett_point points[2];

	for (int i = 0; i < 16; i++)
	{
		double angle = (i * 2.0 * PI) / 16;
		int ix1 = cx + inner_radius * cos(angle);
		int ix2 = cx + outer_radius * cos(angle);
		int iy1 = cy + inner_radius * sin(angle);
		int iy2 = cy + outer_radius * sin(angle);

		points[0] = jett_point(ix1, iy1);
		points[1] = jett_point(ix2, iy2);
		r.lines(*image, colour, linewidth, points, 2, false, antialias);
	}
}

void test_d_10(jett &r, image_t imagetype)
{
	/*
	 *	Check line widths at different angles are correct with anti-aliasing
	 */

	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(500, 700, imagetype);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	// Draw a simple line segments of different widths
	jett_point points[2];
	unsigned char colour[4] = {0, 255, 0, 0}; // (Magenta)

	int cx = 100;
	int cy = 100;

	for (int i = 1; i < 5; i++)
	{
		cy = 100 + 150 * (i - 1);
		draw_wagon_wheel(r, &image, 150, cy, i, 0);
		draw_wagon_wheel(r, &image, 350, cy, i, 1);
	}

	for (int i = 1; i < 5; i++)
	{
		// Draw a diagonals so we can see pixel sizes
		points[0] = jett_point(0, 256 * i);
		points[1] = jett_point(256, 256 * (i - 1));
		colour[0] = 255;
		r.lines(image, colour, 1, points, 2, false, 0);
		points[0] = jett_point(0, 256 * (i - 1));
		points[1] = jett_point(256, 256 * i);
		r.lines(image, colour, 1, points, 2, false, 0);
	}

	// Save the bitmap for inspection
	TCHAR filename[256];
	_tcscpy_s(filename, _T(OUTPUT_DIR));
	_tcscat_s(filename, image_type_as_string(imagetype));
	_tcscat_s(filename, _T(" 010 test_d.tiff"));
	image.saveToFile(filename);
}

void draw_cross(jett &r, jett_image *image, int cx, int cy, int len, int linewidth, int antialias)
{
	// Draw a simple cross
	jett_point points[2];
	unsigned char colour[4] = {0, 255, 0, 0}; // (Magenta)

	points[0] = jett_point(cx, cy);
	points[1] = jett_point(cx + len, cy);
	r.lines(*image, colour, linewidth, points, 2, false, antialias);

	points[1] = jett_point(cx - len, cy);
	r.lines(*image, colour, linewidth, points, 2, false, antialias);

	points[1] = jett_point(cx, cy - len);
	r.lines(*image, colour, linewidth, points, 2, false, antialias);

	points[1] = jett_point(cx, cy + len);
	r.lines(*image, colour, linewidth, points, 2, false, antialias);
}

void test_d_10A(jett &r, image_t imagetype)
{
	/*
	 *	Check vertical & horizontal anti-aliased lines
	 */

	// Create the bitmap we are going to draw on
	jett_image image;
	image.createImage(500, 700, imagetype);

	// Make the image white - the image is normally created unitialised as this
	// is the fastest.
	image.erase();

	int cx = 100;
	int cy = 100;
	for (int lwidth = 1; lwidth < 5; lwidth++)
	{
		cy = 100 + 150 * (lwidth - 1);
		draw_cross(r, &image, cx, cy, 60, lwidth, 0);
		draw_cross(r, &image, cx + 200, cy, 60, lwidth, 1);
	}

	// Draw a diagonal so we can see pixel sizes
	jett_point points[2];
	unsigned char colour[4] = {0, 255, 0, 0};
	points[0] = jett_point(0, 256);
	points[1] = jett_point(256, 0);
	colour[0] = 255;
	r.lines(image, colour, 1, points, 2, false, 0);
	points[0] = jett_point(0, 0);
	points[1] = jett_point(256, 256);
	r.lines(image, colour, 1, points, 2, false, 0);

	// Save the bitmap for inspection
	TCHAR filename[256];
	_tcscpy_s(filename, _T(OUTPUT_DIR));
	_tcscat_s(filename, image_type_as_string(imagetype));
	_tcscat_s(filename, _T(" 010A test_d.tiff"));
	image.saveToFile(filename);
}

typedef void (*graphics_test_fn)(jett &);

struct graphics_test_case
{
	const char *name;
	graphics_test_fn fn;
};

void run_test_d_3_cmyk(jett &r)
{
	test_d_3(r, image_cmyk);
}

void run_test_d_10_cmyk(jett &r)
{
	test_d_10(r, image_cmyk);
}

void run_test_d_10A_cmyk(jett &r)
{
	test_d_10A(r, image_cmyk);
}

int run_default_test_suite(jett &r)
{
	const graphics_test_case tests[] = {
		{"example_1", example_1},
		{"example_2", example_2},
		{"example_3", example_3},
		{"example_4", example_4},
		{"example_5", example_5},
		{"example_6", example_6},
		{"example_7", example_7},
		{"example_8", example_8},
		{"example_9", example_9},
		{"example_10", example_10},
		{"example_11", example_11},
		{"example_12", example_12},
		{"example_13", example_13},
		{"example_14", example_14},
		{"example_15", example_15},
		{"example_16", example_16},
		{"example_17", example_17},
		{"example_18", example_18},
		{"example_19", example_19},
		{"test_d_3_cmyk", run_test_d_3_cmyk},
		{"test_d_10_cmyk", run_test_d_10_cmyk},
		{"test_d_10A_cmyk", run_test_d_10A_cmyk}};

	int failures = 0;

	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i)
	{
		printf("RUN %s\n", tests[i].name);

		try
		{
			tests[i].fn(r);
		}
		catch (const jett_exception &e)
		{
			printf("FAIL %s: %s (code=%d subsys=%d)\n", tests[i].name, e.m_message, e.m_code, e.m_subsys_code);
			++failures;
		}
		catch (...)
		{
			printf("FAIL %s: unknown exception\n", tests[i].name);
			++failures;
		}
	}

	printf("TESTS COMPLETE failures=%d total=%u\n", failures, (unsigned)(sizeof(tests) / sizeof(tests[0])));

	return failures;
}

namespace
{
	int run_suite_for_mode(bool use_gpu, const TCHAR *mode_name, std::set<output_string> &output_names)
	{
		output_names.clear();

		output_string target_dir = join_output_path(_T(EXAMPLES_DIR), mode_name);
		clear_output_directory(target_dir);

		fs::file_time_type suite_start = get_suite_start_marker();
		int failures = 0;
		const char *label = use_gpu ? "GPU" : "CPU";
		srand(1);

		printf("RUN %s SUITE\n", label);

		if (!required_colour_profiles_available())
		{
			printf("SKIP %s suite: required ICC profiles are not available\n", label);
			output_names.clear();
			return 0;
		}

		try
		{
			jett r;
			r.init(use_gpu);
			failures = run_default_test_suite(r);
		}
		catch (const jett_exception &e)
		{
			if (use_gpu && e.m_code == JETT_OPENCL_FAILURE && e.m_subsys_code == -1001)
			{
				printf("SKIP %s suite: no OpenCL platform is available on this machine\n", label);
				output_names.clear();
				return 0;
			}

			printf("FAIL %s suite: %s (code=%d subsys=%d)\n", label, e.m_message, e.m_code, e.m_subsys_code);
			++failures;
		}
		catch (...)
		{
			printf("FAIL %s suite: unknown exception\n", label);
			++failures;
		}

		int moved = move_recent_outputs(target_dir, suite_start, output_names);
		printf("%s SUITE OUTPUTS moved=%d\n", label, moved);

		if (failures == 0 && moved == 0)
		{
			printf("FAIL %s suite: no output files were captured\n", label);
			++failures;
		}

		return failures;
	}
}

int main(int argc, const char *argv[])
{
	std::set<output_string> gpu_outputs;
	std::set<output_string> cpu_outputs;
	int failures = run_generated_asset_tests();
	failures += run_colour_management_tests();

	ensure_output_directory(_T(EXAMPLES_DIR));
	ensure_output_directory(join_output_path(_T(EXAMPLES_DIR), _T("gpu")));
	ensure_output_directory(join_output_path(_T(EXAMPLES_DIR), _T("cpu")));

	if (failures == 0)
	{
		failures += run_suite_for_mode(true, _T("gpu"), gpu_outputs);
		failures += run_suite_for_mode(false, _T("cpu"), cpu_outputs);
	}
	else
	{
		printf("SKIP suites because generated asset tests failed\n");
	}

	if (failures == 0 && !gpu_outputs.empty())
	{
		failures += compare_mode_outputs(cpu_outputs, gpu_outputs);
	}
	else if (failures == 0)
	{
		printf("SKIP compare because no GPU outputs were produced\n");
	}
	else
	{
		printf("SKIP compare because one or more suites failed\n");
	}

	return failures;

#if 0
	test_d_3(r, image_cmyk);
	test_d_10(r, image_cmyk);
	test_d_10A(r, image_cmyk);
#endif

#if 0
	example_1(r);
	example_2(r);
	example_3(r);
	example_4(r);
	example_5(r);
	example_6(r);
	example_7(r);
	example_8(r);
	example_9(r);
	example_10(r);
	example_11(r);
	example_12(r);
	example_13(r);
	example_14(r);
	example_15(r);
	example_16(r);
	example_17(r);
#endif

#if 0

	// Create a LCMS transform
	jett_transform t = NULL;
	jett_transform t2 = NULL;
	jett_transform t3 = NULL;
#endif

#if 0
	jett_matrix a(rand(), rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f, rand() / 1000.0f);
	jett_matrix b = a.invert();

	jett_point p1(rand(), rand());
	jett_point ip = a.apply(p1);
	jett_point op = b.apply(ip);

	//t2 = r.build_transform(":mono", CMYK_ICC_DIR "USWebCoatedSWOP.icc", INTENT_PERCEPTUAL );

	unsigned char colour[4] = { 192, 64, 0, 0 };

	//t = r.build_transform( CMYK_ICC_DIR  "USWebCoatedSWOP.icc", RGB_ICC_DIR "AppleRGB.icc", INTENT_PERCEPTUAL );
	// t = r.build_transform(":srgb", ":mono", INTENT_PERCEPTUAL );
	// t = r.build_transform(":mono", ":srgb", INTENT_PERCEPTUAL );

	jett_image image_in, image_2;

	// Create a font
	jett_font f1 = r.create_font(FONT_FILE, 0, 50, true);
	jett_font f2 = r.create_font(FONT_FILE, 0, 50, false);

	image_in.loadFromFile(IMAGES_DIR "test_img_3.jpeg", image_mode_gpu_copy);
	image_2.loadFromFile(IMAGES_DIR "test_img_5.png", image_mode_gpu_copy);
	// image_in.loadFromFile(IMAGES_DIR "mono_test_image.tif");
	// image_in.loadFromFile(IMAGES_DIR "test_img_11.jpg");

	// Now load an image a transform it
	{
		jett_image image_out;

		// t = r.build_transform(image_in, CMYK_ICC_DIR "USWebCoatedSWOP.icc", INTENT_PERCEPTUAL );
		// t = r.build_transform(":srgb",":srgb", INTENT_PERCEPTUAL );

		// image_out.createImage(image_in.getWidth(), image_in.getHeight(), image_mono );
		image_out.createImage(image_in.getWidth(), image_in.getHeight(), image_cmyk, image_mode_default);
		image_out.set_profile_data(CMYK_ICC_DIR "USWebCoatedSWOP.icc");

		t = r.build_transform(":srgb", image_out, INTENT_PERCEPTUAL);
		t3 = r.build_transform(":srgb", image_out, INTENT_PERCEPTUAL);

		// Perform the transform using GPU
		CTimer t1;

		r.cache_image(image_in, true);
		r.cache_image(image_out, false);

		t1.start();
		r.bitblt(t, image_in, 0, 0, image_in.getWidth(), image_in.getHeight(),
			image_out, 0, 0, image_out.getWidth(), image_out.getHeight(), bitblt_cubic_scaling);
		uint64_t ns1 = t1.elapsed();
		printf("Background copy %lld\n", ns1);

		//
		// Convert a single colour
		//
		unsigned char rgb_col[3] = { 255,0,0 };
		unsigned char cmyk_col[4];
		r.convert(t, rgb_col, cmyk_col);

		//
		// Draw a polygon
		//
		jett_point points[12];
		build_polygon(points, 250, 250, 100, 50, 6, 0);
		r.polygon(image_out, cmyk_col, points, 12, polygon_best);

		//r.flush();

		build_polygon(points, 650, 250, 100, 50, 6, 90);
		// r.polygon(image_out, cmyk_col, points, 12, polygon_best );
		r.lines(image_out, cmyk_col, 5, points, 12, true, line_join_miter | line_best);

		double PI = asin(1.0) * 2.0;
		double q = PI / 7;
		jett_matrix m(cos(q), sin(q), -sin(q), cos(q), 1400, 300);
		r.bitblt(t3, image_2, 0, 0, image_2.getWidth(), image_2.getHeight(),
			image_out, m, bitblt_cubic_scaling | bitblt_use_alpha);

		r.flush();

		r.rectangle(image_out, colour, 750, 1000, 150, 100);

		t1.start();
		double q2 = PI / 9;
		jett_matrix m2(cos(q2), sin(q2), -sin(q2), cos(q2), 0, 0);
		r.set_font_matrix(f1, m2);
		r.text(f1, colour, image_out, 450, 450, "Text at 0 AWAY", string_rotate_0);
		r.text(f1, colour, image_out, 450, 450, "Text at 90", string_rotate_90);
		r.text(f1, colour, image_out, 450, 450, "Text at 180", string_rotate_180);
		r.text(f1, colour, image_out, 450, 450, "Text at 270", string_rotate_270);

		r.set_font_matrix(f2, m2);
		r.text(f2, colour, image_out, 850, 450, "Text at 0", string_rotate_0);
		r.text(f2, colour, image_out, 850, 450, "Text at 90", string_rotate_90);
		r.text(f2, colour, image_out, 850, 450, "Text at 180", string_rotate_180);
		r.text(f2, colour, image_out, 850, 450, "Text at 270", string_rotate_270);
		uint64_t ns = t1.elapsed();
		printf("Font took %lld\n", ns);

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