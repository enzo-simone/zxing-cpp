/*
* Copyright 2016 Nu-book Inc.
* Copyright 2017 Axel Waggershauser
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "ByteArray.h"
#include "BlackboxTestRunner.h"
#include "ImageLoader.h"
#include "TestReader.h"
#include "GenericLuminanceSource.h"
#include "HybridBinarizer.h"
#include "TextUtfEncoding.h"
#include "ZXContainerAlgorithms.h"

#include <iostream>
#include <fstream>
#include <set>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if __has_include(<filesystem>)
#  include <filesystem>
#  ifdef __cpp_lib_filesystem
     namespace fs = std::filesystem;
#    define ZXING_HAS_FILESYSTEM
#  endif
#endif
#if !defined(ZXING_HAS_FILESYSTEM) && __has_include(<experimental/filesystem>)
#  include <experimental/filesystem>
#  ifdef __cpp_lib_experimental_filesystem
     namespace fs = std::experimental::filesystem;
#    define ZXING_HAS_FILESYSTEM
#  endif
#endif
#if defined(__APPLE__) && !defined(__clang__) &&                                                                       \
	__has_include(<dirent.h>) && defined(__SANITIZE_ADDRESS__) && defined(ZXING_HAS_FILESYSTEM)
#  warning detected gcc + macos + <filesystem> + address_sanitizer == bad -> disable <filesystem>
#  undef ZXING_HAS_FILESYSTEM
#endif
#if !defined(ZXING_HAS_FILESYSTEM)
#  if __has_include(<dirent.h>)
#    include <dirent.h>
#  else
#    error need <filesystem> or <dirent.h>
#  endif
#endif

// compiling this with clang (e.g. version 6) might require linking against libc++experimental.a or libc++fs.a.
// E.g.: CMAKE_EXE_LINKER_FLAGS = -L/usr/local/Cellar/llvm/6.0.1/lib -lc++experimental

using namespace ZXing;
using namespace ZXing::Test;

static std::string getExtension(const std::string& fn)
{
	auto p = fn.find_last_of(".");
	return p != std::string::npos ?	fn.substr(p) : "";
}

static std::shared_ptr<LuminanceSource> readImage(const std::string& filename)
{
    int width, height, channels;
    auto buffer = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if (buffer == nullptr) {
        throw std::runtime_error("Failed to read image");
    }
    auto lumSrc = std::make_shared<GenericLuminanceSource>(width, height, buffer, width * 4, 4, 0, 1, 2);
    stbi_image_free(buffer);
    return lumSrc;
}

class GenericImageLoader : public ImageLoader
{
public:
	std::shared_ptr<LuminanceSource> load(const std::wstring& filename) const final
	{
		return readImage(TextUtfEncoding::ToUtf8(filename));
	}
};

class GenericBlackboxTestRunner : public BlackboxTestRunner
{
public:
	GenericBlackboxTestRunner(const std::wstring& pathPrefix)
		: BlackboxTestRunner(pathPrefix, std::make_shared<GenericImageLoader>())
	{
	}

	std::vector<std::wstring> getImagesInDirectory(const std::wstring& dirPath) final
	{
		std::vector<std::wstring> result;
#ifndef ZXING_HAS_FILESYSTEM
		if (auto dir = opendir(TextUtfEncoding::ToUtf8(pathPrefix() + L"/" + dirPath).c_str())) {
			while (auto entry = readdir(dir)) {
				if (Contains({".png", ".jpg", ".pgm", ".gif"}, getExtension(entry->d_name))) {
					result.push_back(dirPath + L"/" + TextUtfEncoding::FromUtf8(entry->d_name));
				}
			}
			closedir(dir);
		}
		else {
			std::cerr << "Error open dir " << TextUtfEncoding::ToUtf8(dirPath) << std::endl;
		}
#else
		for (const auto& entry : fs::directory_iterator(fs::path(pathPrefix()) / dirPath))
			if (fs::is_regular_file(entry.status()) &&
			        Contains({".png", ".jpg", ".pgm", ".gif"}, entry.path().extension()))
				result.push_back(dirPath + L"/" + entry.path().filename().generic_wstring());
#endif
		return result;
	}
};

int main(int argc, char** argv)
{
	if (argc <= 1) {
		std::cout << "Usage: " << argv[0] << " <test_path_prefix>" << std::endl;
		return 0;
	}

	std::string pathPrefix = argv[1];

	GenericBlackboxTestRunner runner(TextUtfEncoding::FromUtf8(pathPrefix));

	if (Contains({".png", ".jpg", ".pgm", ".gif"}, getExtension(pathPrefix))) {
#if 0
		TestReader reader = runner.createReader(false, false, "QR_CODE");
#else
		TestReader reader = runner.createReader(true, true);
#endif
		bool isPure = getenv("IS_PURE");
		int rotation = getenv("ROTATION") ? atoi(getenv("ROTATION")) : 0;

		for (int i = 1; i < argc; ++i) {
			auto result = reader.read(TextUtfEncoding::FromUtf8(argv[i]), rotation, isPure);
			std::cout << argv[i] << ": ";
			if (result)
				std::cout << result.format << ": " << TextUtfEncoding::ToUtf8(result.text) << " " << TextUtfEncoding::ToUtf8(result.metadata) << "\n";
			else
				std::cout << "FAILED\n";
#ifdef ZXING_HAS_FILESYSTEM
			if (result && getenv("WRITE_TEXT")) {
				std::ofstream f(fs::path(argv[i]).replace_extension(".txt"));
				f << TextUtfEncoding::ToUtf8(result.text);
			}
#endif
		}
		return 0;
	}

	std::set<std::string> includedTests;
	for (int i = 2; i < argc; ++i) {
		if (std::strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == 't') {
			includedTests.insert(argv[i] + 2);
		}
	}

	runner.run(includedTests);
}
