#include "StdAfx.h"
#include "colour_management_tests.h"

#include "../GraphicsLibrary/jett.h"

#include <filesystem>
#include <math.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <system_error>

#ifdef _WIN32
#define TEST_ASSET_DIR "test_images\\"
#define cmyk_test_profile_path() _T("..\\..\\Adobe ICC Profiles\\CMYK Profiles\\USWebCoatedSWOP.icc")
#define rgb_test_profile_path() _T("..\\..\\Adobe ICC Profiles\\RGB Profiles\\AdobeRGB1998.icc")
#else
#define TEST_ASSET_DIR "test_images/"
#endif

typedef std::basic_string<TCHAR> output_string;

namespace
{
	namespace fs = std::filesystem;

	output_string path_native_string(const fs::path &path)
	{
		return path.native();
	}

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

	void throw_colour_management_error(const char *message)
	{
		throw jett_exception(JETT_INTERNAL_ERROR, 0, message);
	}

	#ifndef _WIN32
	bool path_exists(const output_string &path)
	{
		std::error_code error;
		return fs::exists(fs::path(path), error);
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
		const char *home = getenv("HOME");
		if (home && home[0] != 0)
		{
			search_roots.push_back(output_string(home) + _T("/Library/ColorSync/Profiles"));
			search_roots.push_back(output_string(home) + _T("/Library/Application Support/Adobe/Color/Profiles"));
			search_roots.push_back(output_string(home) + _T("/.local/share/color/icc"));
			search_roots.push_back(output_string(home) + _T("/.color/icc"));
			search_roots.push_back(output_string(home) + _T("/.fonts"));
		}

		search_roots.push_back(_T("/Library/ColorSync/Profiles"));
		search_roots.push_back(_T("/System/Library/ColorSync/Profiles"));
		search_roots.push_back(_T("/Network/Library/ColorSync/Profiles"));
		search_roots.push_back(_T("/Library/Application Support/Adobe/Color/Profiles"));
		search_roots.push_back(_T("/usr/share/color/icc"));
		search_roots.push_back(_T("/usr/local/share/color/icc"));
		search_roots.push_back(_T("/var/lib/color/icc"));

		for (size_t i = 0; i < search_roots.size(); ++i)
		{
			if (!directory_exists(search_roots[i]))
			{
				continue;
			}

			for (size_t profile_index = 0; profile_index < profile_names.size(); ++profile_index)
			{
				output_string direct_match = join_generated_path(search_roots[i], profile_names[profile_index]);
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

		throw_colour_management_error("Unable to locate required ICC profile in standard system locations");
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
#endif

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

		throw_colour_management_error("Unable to create generated asset directory");
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

	void fill_rgb_scene(jett_image &image, int width, int height)
	{
		image.createImage(width, height, image_rgb);
		unsigned char *data = image.lockData(false);
		int stride = image.getStride();

		for (int y = 0; y < height; ++y)
		{
			unsigned char *row = data + y * stride;
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

				row[x * 3 + 0] = clamp_byte(red);
				row[x * 3 + 1] = clamp_byte(green);
				row[x * 3 + 2] = clamp_byte(blue);
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

	std::vector<unsigned char> read_binary_file(const TCHAR *path)
	{
		FILE *file = NULL;
		_tfopen_s(&file, path, _T("rb"));
		if (!file)
		{
			throw_colour_management_error("Unable to open file for colour-management validation");
		}

		if (fseek(file, 0, SEEK_END) != 0)
		{
			fclose(file);
			throw_colour_management_error("Unable to size file for colour-management validation");
		}

		long size = ftell(file);
		if (size < 0)
		{
			fclose(file);
			throw_colour_management_error("Unable to size file for colour-management validation");
		}

		if (fseek(file, 0, SEEK_SET) != 0)
		{
			fclose(file);
			throw_colour_management_error("Unable to rewind file for colour-management validation");
		}

		std::vector<unsigned char> data(static_cast<size_t>(size));
		if (!data.empty() && fread(&data[0], 1, data.size(), file) != data.size())
		{
			fclose(file);
			throw_colour_management_error("Unable to read file for colour-management validation");
		}

		fclose(file);
		return data;
	}

	#ifdef _WIN32
	std::vector<unsigned char> read_binary_file(const char *path)
	{
		FILE *file = NULL;
		fopen_s(&file, path, "rb");
		if (!file)
		{
			throw_colour_management_error("Unable to open file for colour-management validation");
		}

		if (fseek(file, 0, SEEK_END) != 0)
		{
			fclose(file);
			throw_colour_management_error("Unable to size file for colour-management validation");
		}

		long size = ftell(file);
		if (size < 0)
		{
			fclose(file);
			throw_colour_management_error("Unable to size file for colour-management validation");
		}

		if (fseek(file, 0, SEEK_SET) != 0)
		{
			fclose(file);
			throw_colour_management_error("Unable to rewind file for colour-management validation");
		}

		std::vector<unsigned char> data(static_cast<size_t>(size));
		if (!data.empty() && fread(&data[0], 1, data.size(), file) != data.size())
		{
			fclose(file);
			throw_colour_management_error("Unable to read file for colour-management validation");
		}

		fclose(file);
		return data;
	}
#endif

	void expect_condition(bool condition, const char *message)
	{
		if (!condition)
		{
			throw_colour_management_error(message);
		}
	}

	void expect_profile_bytes(jett_image &image, const std::vector<unsigned char> &expected, const char *message)
	{
		if (image.get_profile_size() != expected.size())
		{
			char buffer[256];
			sprintf_s(buffer, "%s (expected_size=%zu actual_size=%zu)", message, expected.size(), image.get_profile_size());
			throw_colour_management_error(buffer);
		}

		if (!expected.empty())
		{
			const unsigned char *actual = static_cast<const unsigned char *>(image.get_profile_data());
			for (size_t index = 0; index < expected.size(); ++index)
			{
				if (actual[index] != expected[index])
				{
					char buffer[256];
					sprintf_s(buffer, "%s (first_diff=%zu expected=%u actual=%u)", message, index, static_cast<unsigned int>(expected[index]), static_cast<unsigned int>(actual[index]));
					throw_colour_management_error(buffer);
				}
			}
		}
	}

	void expect_pixels_equal(const unsigned char *actual, const unsigned char *expected, int channels, const char *message)
	{
		for (int index = 0; index < channels; ++index)
		{
			if (actual[index] != expected[index])
			{
				throw_colour_management_error(message);
			}
		}
	}

	void expect_pixels_near(const unsigned char *actual, const unsigned char *expected, int channels, int tolerance, const char *message)
	{
		for (int index = 0; index < channels; ++index)
		{
			int diff = abs(static_cast<int>(actual[index]) - static_cast<int>(expected[index]));
			if (diff > tolerance)
			{
				throw_colour_management_error(message);
			}
		}
	}

	void validate_profile_roundtrip(const TCHAR *relative_path, jett_image &image, const std::vector<unsigned char> &expected_profile)
	{
		ensure_generated_asset_parent(relative_path);
		output_string path = generated_asset_path(relative_path);
		image.saveToFile(path.c_str());

		jett_image loaded;
		loaded.loadFromFile(path.c_str());
		std::string message = "Embedded ICC profile changed during save/load round-trip: ";
		message += narrow_path(relative_path);
		expect_profile_bytes(loaded, expected_profile, message.c_str());
	}

	void validate_transform_overloads(jett &r)
	{
		jett_image src_image;
		src_image.createImage(8, 8, image_rgb);
		src_image.set_profile_data(":srgb");

		jett_image dst_image;
		dst_image.createImage(8, 8, image_cmyk);
		dst_image.set_profile_data(cmyk_test_profile_path());

		const unsigned char src_pixel[3] = {12, 120, 220};
		unsigned char expected_cmyk[4] = {0, 0, 0, 0};
		unsigned char actual_cmyk[4] = {0, 0, 0, 0};
		unsigned char expected_mono[1] = {0};
		unsigned char actual_mono[1] = {0};
		unsigned char expected_lab[3] = {0, 0, 0};
		unsigned char actual_lab[3] = {0, 0, 0};

		jett_transform reference = r.build_transform(":srgb", cmyk_test_profile_path(), INTENT_PERCEPTUAL);
		jett_transform from_src_image = r.build_transform(src_image, cmyk_test_profile_path(), INTENT_PERCEPTUAL);
		jett_transform from_dst_image = r.build_transform(":srgb", dst_image, INTENT_PERCEPTUAL);
		jett_transform from_both_images = r.build_transform(src_image, dst_image, INTENT_PERCEPTUAL);

		r.convert(reference, src_pixel, expected_cmyk);
		r.convert(from_src_image, src_pixel, actual_cmyk);
		expect_pixels_equal(actual_cmyk, expected_cmyk, 4, "Source-image transform overload diverged from file/file transform");
		r.convert(from_dst_image, src_pixel, actual_cmyk);
		expect_pixels_equal(actual_cmyk, expected_cmyk, 4, "Destination-image transform overload diverged from file/file transform");
		r.convert(from_both_images, src_pixel, actual_cmyk);
		expect_pixels_equal(actual_cmyk, expected_cmyk, 4, "Image/image transform overload diverged from file/file transform");

		r.destroy_transform(reference);
		r.destroy_transform(from_src_image);
		r.destroy_transform(from_dst_image);
		r.destroy_transform(from_both_images);

		jett_image mono_image;
		mono_image.createImage(4, 4, image_mono);
		mono_image.set_profile_data(":mono");
		reference = r.build_transform(":srgb", ":mono", INTENT_PERCEPTUAL);
		from_dst_image = r.build_transform(":srgb", mono_image, INTENT_PERCEPTUAL);
		r.convert(reference, src_pixel, expected_mono);
		r.convert(from_dst_image, src_pixel, actual_mono);
		expect_pixels_equal(actual_mono, expected_mono, 1, "Built-in monochrome profile attachment diverged from :mono transform");
		r.destroy_transform(reference);
		r.destroy_transform(from_dst_image);

		jett_image lab_image;
		lab_image.createImage(4, 4, image_lab);
		lab_image.set_profile_data(":lab");
		reference = r.build_transform(":srgb", ":lab", INTENT_RELATIVE_COLORIMETRIC);
		from_dst_image = r.build_transform(":srgb", lab_image, INTENT_RELATIVE_COLORIMETRIC);
		r.convert(reference, src_pixel, expected_lab);
		r.convert(from_dst_image, src_pixel, actual_lab);
		expect_pixels_equal(actual_lab, expected_lab, 3, "Built-in Lab profile attachment diverged from :lab transform");
		r.destroy_transform(reference);
		r.destroy_transform(from_dst_image);
	}

	void validate_devicelink_transforms(jett &r)
	{
		jett_transform mono_to_cmyk = r.build_transform(":mono_cmyk", INTENT_PERCEPTUAL);
		jett_transform mono_to_rgb = r.build_transform(":mono_rgb", INTENT_PERCEPTUAL);
		const unsigned char black_in[1] = {0};
		const unsigned char white_in[1] = {255};
		unsigned char cmyk_pixel[4] = {0, 0, 0, 0};
		unsigned char rgb_pixel[3] = {0, 0, 0};
		const unsigned char expected_cmyk_black[4] = {0, 0, 0, 255};
		const unsigned char expected_cmyk_white[4] = {0, 0, 0, 0};
		const unsigned char expected_rgb_black[3] = {255, 255, 255};
		const unsigned char expected_rgb_white[3] = {0, 0, 0};

		r.convert(mono_to_cmyk, black_in, cmyk_pixel);
		expect_pixels_near(cmyk_pixel, expected_cmyk_black, 4, 1, "Built-in :mono_cmyk transform produced the wrong black mapping");
		r.convert(mono_to_cmyk, white_in, cmyk_pixel);
		expect_pixels_near(cmyk_pixel, expected_cmyk_white, 4, 1, "Built-in :mono_cmyk transform produced the wrong white mapping");

		r.convert(mono_to_rgb, black_in, rgb_pixel);
		expect_pixels_near(rgb_pixel, expected_rgb_black, 3, 1, "Built-in :mono_rgb transform produced the wrong black mapping");
		r.convert(mono_to_rgb, white_in, rgb_pixel);
		expect_pixels_near(rgb_pixel, expected_rgb_white, 3, 1, "Built-in :mono_rgb transform produced the wrong white mapping");

		r.destroy_transform(mono_to_cmyk);
		r.destroy_transform(mono_to_rgb);
	}

	void validate_profile_failure_paths(jett &r)
	{
		jett_image no_profile;
		no_profile.createImage(4, 4, image_rgb);

		bool failed = false;
		try
		{
			jett_transform t = r.build_transform(no_profile, cmyk_test_profile_path(), INTENT_PERCEPTUAL);
			r.destroy_transform(t);
		}
		catch (const jett_exception &e)
		{
			failed = (e.m_code == JETT_INVALID_ARGUMENT);
		}

		expect_condition(failed, "Building a transform from an image without an embedded profile should fail");
	}

	void validate_colour_management_features(jett &r)
	{
		std::vector<unsigned char> rgb_profile = read_binary_file(rgb_test_profile_path());
		std::vector<unsigned char> cmyk_profile = read_binary_file(cmyk_test_profile_path());

		jett_image built_in_srgb_image;
		fill_rgb_scene(built_in_srgb_image, 96, 72);
		built_in_srgb_image.set_profile_data(":srgb");
		expect_condition(built_in_srgb_image.get_profile_size() > 0, "Built-in sRGB profile was not materialized into the image");

		jett_image png_image;
		fill_rgb_scene(png_image, 192, 128);
		png_image.set_profile_data(rgb_test_profile_path());
		validate_profile_roundtrip(_T("Colour/profile_roundtrip.png"), png_image, rgb_profile);

		jett_image jpeg_image;
		fill_rgb_scene(jpeg_image, 176, 132);
		jpeg_image.set_profile_data(&rgb_profile[0], rgb_profile.size());
		rgb_profile[0] ^= 0xFF;
		expect_condition(static_cast<unsigned char *>(jpeg_image.get_profile_data())[0] != rgb_profile[0], "set_profile_data(void*, size) did not copy the ICC data");
		rgb_profile[0] ^= 0xFF;
		validate_profile_roundtrip(_T("Colour/profile_roundtrip.jpg"), jpeg_image, rgb_profile);

		jett_image tiff_image;
		fill_cmyk_scene(tiff_image, 144, 120);
		tiff_image.set_profile_data(cmyk_test_profile_path());
		expect_profile_bytes(tiff_image, cmyk_profile, "CMYK ICC profile bytes were not attached correctly");
		validate_profile_roundtrip(_T("Colour/profile_roundtrip.tiff"), tiff_image, cmyk_profile);

		validate_transform_overloads(r);
		validate_devicelink_transforms(r);
		validate_profile_failure_paths(r);
	}
}

int run_colour_management_tests()
{
	printf("RUN colour_management_tests\n");
	fflush(stdout);

	try
	{
		jett r;
		r.init(false);
		validate_colour_management_features(r);
		printf("COLOUR MANAGEMENT TESTS COMPLETE failures=0\n");
		return 0;
	}
	catch (const jett_exception &e)
	{
		printf("FAIL colour_management_tests: %s (code=%d subsys=%d)\n", e.m_message, e.m_code, e.m_subsys_code);
		return 1;
	}
	catch (...)
	{
		printf("FAIL colour_management_tests: unknown exception\n");
		return 1;
	}
}