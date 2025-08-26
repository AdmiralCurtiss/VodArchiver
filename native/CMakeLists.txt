cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(VodArchiver LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (DEFINED VODARCHIVER_BUILDFLAGS OR DEFINED VODARCHIVER_BUILDFLAGS_RELEASE OR DEFINED VODARCHIVER_BUILDFLAGS_DEBUG)
	# user has explicitly set buildflags
	separate_arguments(VODARCHIVER_BUILDFLAGS_INTERNAL NATIVE_COMMAND "${VODARCHIVER_BUILDFLAGS}")
	separate_arguments(VODARCHIVER_BUILDFLAGS_INTERNAL_RELEASE NATIVE_COMMAND "${VODARCHIVER_BUILDFLAGS_RELEASE}")
	separate_arguments(VODARCHIVER_BUILDFLAGS_INTERNAL_DEBUG NATIVE_COMMAND "${VODARCHIVER_BUILDFLAGS_DEBUG}")
else()
	if (MSVC)
		# we want utf8 as our execution charset
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/utf-8")

		# enable all warnings, then disable the ones that don't really help
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/Wall")

		# spectre mitigation warning
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd5045")

		# unreferenced inline function has been removed
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4514")

		# reports padding of structs
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4820")

		# enum not explicitly handled in switch
		# this triggers even when a 'default' case exists, which makes it kind of useless
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4061")

		# relative include path includes '..'
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4464")

		# function marked for inlining was not inlined
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4710")

		# aligned struct is padded
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4324")

		# passing exception-throwing function to extern C. this unfortunately occurs in winbase.h by itself...
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd5039")
		
		# ifdef DEFINE is defined as '0'
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4574")

		# if DEFINE not defined, assuming '0'. this also occurs in windows headers
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4668")

		# all of these are variations on 'copy constructor was implicitly deleted'
		# these are extremely noisy if you're making method-less structs for readability
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4623")
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4624")
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4625")
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd4626")
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd5024")
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd5025")
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd5026")
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/wd5027")

		# be standards-conforming
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/permissive-")

		# catch C++ exceptions, assume C functions do not throw, do not catch structured exceptions
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/EHsc")

		# generate .pdb files
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL "/Zi")


		# for debug build:

		# disable optimizations
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL_DEBUG "/Od")


		# for release build:

		# optimize as fast code
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL_RELEASE "/O2")

		# remove unreferenced functions or data
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL_RELEASE "/Zc:inline")

		# handle the intel JCC performance erratum
		list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL_RELEASE "/QIntel-jcc-erratum")

		# enable whole-program optimization
		# list(APPEND VODARCHIVER_BUILDFLAGS_INTERNAL_RELEASE "/GL")
	endif()
endif()
message(STATUS "Using build flags (any): ${VODARCHIVER_BUILDFLAGS_INTERNAL}")
message(STATUS "Using build flags (debug): ${VODARCHIVER_BUILDFLAGS_INTERNAL_DEBUG}")
message(STATUS "Using build flags (release): ${VODARCHIVER_BUILDFLAGS_INTERNAL_RELEASE}")

function(vodarchiver_generate_embeddable_file SOURCE_PATH TARGET_NAME)
	get_filename_component(TMP_VODARCHIVER_FILEPATH "${SOURCE_PATH}" ABSOLUTE)
	file(SHA256 "${TMP_VODARCHIVER_FILEPATH}" TMP_VODARCHIVER_FILEHASH)
	file(WRITE "${CMAKE_BINARY_DIR}/vodarchiver_embeds/${TARGET_NAME}.sha256.tmp" "${TMP_VODARCHIVER_FILEHASH}")
	execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${CMAKE_BINARY_DIR}/vodarchiver_embeds/${TARGET_NAME}.sha256" "${CMAKE_BINARY_DIR}/vodarchiver_embeds/${TARGET_NAME}.sha256.tmp" RESULT_VARIABLE TMP_VODARCHIVER_FILEHASH_EQUAL)
	if (NOT TMP_VODARCHIVER_FILEHASH_EQUAL EQUAL 0)
		file(READ "${TMP_VODARCHIVER_FILEPATH}" TMP_VODARCHIVER_FILE HEX)
		string(REGEX REPLACE "([0-9A-Fa-f][0-9A-Fa-f])" ",'\\\\x\\1'" TMP_VODARCHIVER_FILE "${TMP_VODARCHIVER_FILE}")
		string(SUBSTRING "${TMP_VODARCHIVER_FILE}" 1 -1 TMP_VODARCHIVER_FILE)
		file(WRITE "${CMAKE_BINARY_DIR}/vodarchiver_embeds/${TARGET_NAME}" "${TMP_VODARCHIVER_FILE}")
		file(WRITE "${CMAKE_BINARY_DIR}/vodarchiver_embeds/${TARGET_NAME}.sha256" "${TMP_VODARCHIVER_FILEHASH}")
	endif()
endfunction()

function(vodarchiver_generate_embeddable_file_if_exists SOURCE_PATH TARGET_NAME OUT_VARIABLE)
	get_filename_component(TMP_VODARCHIVER_FILEPATH "${SOURCE_PATH}" ABSOLUTE)
	if (EXISTS "${TMP_VODARCHIVER_FILEPATH}")
		vodarchiver_generate_embeddable_file("${SOURCE_PATH}" "${TARGET_NAME}")
		set(${OUT_VARIABLE} TRUE PARENT_SCOPE)
	else()
		set(${OUT_VARIABLE} FALSE PARENT_SCOPE)
	endif()
endfunction()


file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vodarchiver_embeds")
if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/vodarchiver_embeds/vodarchiver_version.h")
	file(TOUCH "${CMAKE_CURRENT_BINARY_DIR}/vodarchiver_embeds/vodarchiver_version.h")
endif()

add_custom_target(
	GenerateGitRevisionHeader
	${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/GitRevisionHeader.cmake"
	BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/vodarchiver_embeds/vodarchiver_version.h"
	VERBATIM
)

set(SKIP_INSTALL_ALL ON)
set(ZLIB_BUILD_EXAMPLES OFF)
add_subdirectory(zlib)

set(BUILD_CURL_EXE OFF)
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)
set(CURL_DISABLE_INSTALL ON)
set(ENABLE_UNICODE ON)
set(CURL_ENABLE_SSL ON)
if (WIN32)
	set(CURL_USE_SCHANNEL ON)
endif()
set(CURL_USE_LIBPSL OFF) # TODO: check if we can add this easily
add_subdirectory(curl)

set(SOURCES_UTIL
	util/hash/crc32.h
	util/hash/crc32.c
	util/hash/md5.h
	util/hash/sha_x86_extension.cpp
	util/hash/sha_x86_extension.h
	util/hash/sha1.h
	util/hash/sha1.cpp
	util/hash/sha256.h
	util/hash/sha256.cpp
	util/hash/util.h
	util/align.h
	util/align.cpp
	util/args.h
	util/args.cpp
	util/bps.h
	util/bps.cpp
	util/cpuid.cpp
	util/cpuid.h
	util/endian.h
	util/file.h
	util/file.cpp
	util/ini.h
	util/ini.cpp
	util/ini_writer.cpp
	util/ini_writer.h
	util/memread.h
	util/memwrite.h
	util/number.cpp
	util/number.h
	util/result.h
	util/scope.h
	util/stream.cpp
	util/stream.h
	util/system.cpp
	util/system.h
	util/text.cpp
	util/text.h
	util/thread.cpp
	util/thread.h
	util/vector.cpp
	util/vector.h
)
set(SOURCES_LZMA_BASE
	lzma/7zTypes.h
	lzma/Compiler.h
	lzma/LzmaDec.c
	lzma/LzmaDec.h
	lzma/Precomp.h
)
set(SOURCES_LZMA_COMPRESSION
	lzma/7zWindows.h
	lzma/CpuArch.c
	lzma/CpuArch.h
	lzma/LzFind.c
	lzma/LzFind.h
	lzma/LzFindMt.c
	lzma/LzFindMt.h
	lzma/LzFindOpt.c
	lzma/LzHash.h
	lzma/LzmaEnc.c
	lzma/LzmaEnc.h
	lzma/Threads.c
	lzma/Threads.h
)
set(SOURCES_RAPIDXML
	rapidxml/rapidxml.hpp
	rapidxml/rapidxml_iterators.hpp
	rapidxml/rapidxml_print.hpp
	rapidxml/rapidxml_utils.hpp
)
set(SOURCES_RAPIDJSON
	rapidjson/allocators.h
	rapidjson/cursorstreamwrapper.h
	rapidjson/document.h
	rapidjson/encodedstream.h
	rapidjson/encodings.h
	rapidjson/filereadstream.h
	rapidjson/filewritestream.h
	rapidjson/fwd.h
	rapidjson/istreamwrapper.h
	rapidjson/memorybuffer.h
	rapidjson/memorystream.h
	rapidjson/ostreamwrapper.h
	rapidjson/pointer.h
	rapidjson/prettywriter.h
	rapidjson/rapidjson.h
	rapidjson/reader.h
	rapidjson/schema.h
	rapidjson/stream.h
	rapidjson/stringbuffer.h
	rapidjson/uri.h
	rapidjson/writer.h
	rapidjson/error/en.h
	rapidjson/error/error.h
	rapidjson/internal/biginteger.h
	rapidjson/internal/clzll.h
	rapidjson/internal/diyfp.h
	rapidjson/internal/dtoa.h
	rapidjson/internal/ieee754.h
	rapidjson/internal/itoa.h
	rapidjson/internal/meta.h
	rapidjson/internal/pow10.h
	rapidjson/internal/regex.h
	rapidjson/internal/stack.h
	rapidjson/internal/strfunc.h
	rapidjson/internal/strtod.h
	rapidjson/internal/swap.h
	rapidjson/msinttypes/inttypes.h
	rapidjson/msinttypes/stdint.h
)

add_executable(vodarchiver)
target_sources(vodarchiver PRIVATE
	vodarchiver/cli_tool.h
	vodarchiver/common_paths.cpp
	vodarchiver/common_paths.h
	vodarchiver/curl_util.cpp
	vodarchiver/curl_util.h
	vodarchiver/decompress_helper.cpp
	vodarchiver/decompress_helper.h
	vodarchiver/disk_lock.cpp
	vodarchiver/disk_lock.h
	vodarchiver/exec.cpp
	vodarchiver/exec.h
	vodarchiver/ffmpeg_util.cpp
	vodarchiver/ffmpeg_util.h
	vodarchiver/filename_util.cpp
	vodarchiver/filename_util.h
	vodarchiver/job_config.h
	vodarchiver/job_handling.cpp
	vodarchiver/job_handling.h
	vodarchiver/main.cpp
	vodarchiver/system_util.cpp
	vodarchiver/system_util.h
	vodarchiver/task_cancellation.cpp
	vodarchiver/task_cancellation.h
	vodarchiver/task_reporting.h
	vodarchiver/task_reporting_dummy.h
	vodarchiver/task_reporting_from_thread.h
	vodarchiver/time_types.cpp
	vodarchiver/time_types.h
	vodarchiver/twitch_util.cpp
	vodarchiver/twitch_util.h
	vodarchiver/youtube_util.cpp
	vodarchiver/youtube_util.h

	vodarchiver/tasks/fetch-task-group.cpp
	vodarchiver/tasks/fetch-task-group.h
	vodarchiver/tasks/video-task-group.cpp
	vodarchiver/tasks/video-task-group.h

	vodarchiver/userinfo/archive-org-user-info.cpp
	vodarchiver/userinfo/archive-org-user-info.h
	vodarchiver/userinfo/ffmpeg-job-user-info.cpp
	vodarchiver/userinfo/ffmpeg-job-user-info.h
	vodarchiver/userinfo/generic-user-info.cpp
	vodarchiver/userinfo/generic-user-info.h
	vodarchiver/userinfo/hitbox-user-info.cpp
	vodarchiver/userinfo/hitbox-user-info.h
	vodarchiver/userinfo/i-user-info.cpp
	vodarchiver/userinfo/i-user-info.h
	vodarchiver/userinfo/rss-feed-user-info.cpp
	vodarchiver/userinfo/rss-feed-user-info.h
	vodarchiver/userinfo/serialization.cpp
	vodarchiver/userinfo/serialization.h
	vodarchiver/userinfo/twitch-user-info.cpp
	vodarchiver/userinfo/twitch-user-info.h
	vodarchiver/userinfo/youtube-channel-user-info.cpp
	vodarchiver/userinfo/youtube-channel-user-info.h
	vodarchiver/userinfo/youtube-playlist-user-info.cpp
	vodarchiver/userinfo/youtube-playlist-user-info.h
	vodarchiver/userinfo/youtube-url-user-info.cpp
	vodarchiver/userinfo/youtube-url-user-info.h
	vodarchiver/userinfo/youtube-user-user-info.cpp
	vodarchiver/userinfo/youtube-user-user-info.h

	vodarchiver/videoinfo/ffmpeg-reencode-job-video-info.cpp
	vodarchiver/videoinfo/ffmpeg-reencode-job-video-info.h
	vodarchiver/videoinfo/generic-video-info.cpp
	vodarchiver/videoinfo/generic-video-info.h
	vodarchiver/videoinfo/hitbox-video-info.cpp
	vodarchiver/videoinfo/hitbox-video-info.h
	vodarchiver/videoinfo/i-video-info.cpp
	vodarchiver/videoinfo/i-video-info.h
	vodarchiver/videoinfo/twitch-video-info.cpp
	vodarchiver/videoinfo/twitch-video-info.h
	vodarchiver/videoinfo/youtube-video-info.cpp
	vodarchiver/videoinfo/youtube-video-info.h

	vodarchiver/videojobs/ffmpeg-reencode-job.cpp
	vodarchiver/videojobs/ffmpeg-reencode-job.h
	vodarchiver/videojobs/ffmpeg-split-job.cpp
	vodarchiver/videojobs/ffmpeg-split-job.h
	vodarchiver/videojobs/generic-file-job.cpp
	vodarchiver/videojobs/generic-file-job.h
	vodarchiver/videojobs/hitbox-video-job.cpp
	vodarchiver/videojobs/hitbox-video-job.h
	vodarchiver/videojobs/i-video-job.cpp
	vodarchiver/videojobs/i-video-job.h
	vodarchiver/videojobs/serialization.cpp
	vodarchiver/videojobs/serialization.h
	vodarchiver/videojobs/twitch-chat-replay-job.cpp
	vodarchiver/videojobs/twitch-chat-replay-job.h
	vodarchiver/videojobs/twitch-video-job.cpp
	vodarchiver/videojobs/twitch-video-job.h
	vodarchiver/videojobs/youtube-video-job.cpp
	vodarchiver/videojobs/youtube-video-job.h

	${SOURCES_UTIL}
	${SOURCES_LZMA_BASE}
	${SOURCES_LZMA_COMPRESSION}
	${SOURCES_RAPIDXML}
	${SOURCES_RAPIDJSON}
)

target_compile_definitions(vodarchiver
	PUBLIC FILE_WRAPPER_WITH_STD_FILESYSTEM RAPIDXML_NO_STREAMS
)
if (WIN32)
	target_compile_definitions(vodarchiver PUBLIC BUILD_FOR_WINDOWS UNICODE _UNICODE)
endif()

target_compile_options(vodarchiver PRIVATE ${VODARCHIVER_BUILDFLAGS_INTERNAL})
target_compile_options(vodarchiver PRIVATE $<$<CONFIG:Debug>:${VODARCHIVER_BUILDFLAGS_INTERNAL_DEBUG}>)
target_compile_options(vodarchiver PRIVATE $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:${VODARCHIVER_BUILDFLAGS_INTERNAL_RELEASE}>)
target_include_directories(vodarchiver
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/vodarchiver_embeds"
)
target_link_libraries(vodarchiver PUBLIC zlibstatic libcurl)

target_sources(vodarchiver PRIVATE
	vodarchiver/gui/background_save_thread.cpp
	vodarchiver/gui/background_save_thread.h
	vodarchiver/gui/gui_background_task.h
	vodarchiver/gui/gui_fetch_window.cpp
	vodarchiver/gui/gui_fetch_window.h
	vodarchiver/gui/gui_file_browser.cpp
	vodarchiver/gui/gui_file_browser.h
	vodarchiver/gui/gui_fonts.cpp
	vodarchiver/gui/gui_fonts.h
	vodarchiver/gui/gui_log_window.cpp
	vodarchiver/gui/gui_log_window.h
	vodarchiver/gui/gui_settings_window.cpp
	vodarchiver/gui/gui_settings_window.h
	vodarchiver/gui/gui_state.cpp
	vodarchiver/gui/gui_state.h
	vodarchiver/gui/gui_user_settings.cpp
	vodarchiver/gui/gui_user_settings.h
	vodarchiver/gui/gui_video_split_window.cpp
	vodarchiver/gui/gui_video_split_window.h
	vodarchiver/gui/gui_vodarchiver_main_window.cpp
	vodarchiver/gui/gui_vodarchiver_main_window.h
	vodarchiver/gui/gui_window.cpp
	vodarchiver/gui/gui_window.h
	vodarchiver/gui/main_gui.cpp
	vodarchiver/gui/main_gui.h
	vodarchiver/gui/vodarchiver_imgui_utils.h
	vodarchiver/gui/window_id_management.cpp
	vodarchiver/gui/window_id_management.h

	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(vodarchiver
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/imgui" "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)
target_compile_definitions(vodarchiver PUBLIC IMGUI_USE_WCHAR32)

if (WIN32)
	# on Windows we just use DX11 for this
	target_sources(vodarchiver PRIVATE
		vodarchiver/gui/gui_setup_dx11.cpp
		vodarchiver/gui/gui_setup_dx11.h
		imgui/backends/imgui_impl_dx11.cpp
		imgui/backends/imgui_impl_win32.cpp
	)
	target_link_libraries(vodarchiver PUBLIC d3d11.lib d3dcompiler.lib)
	target_compile_definitions(vodarchiver PUBLIC IMGUI_IMPL_WIN32_DISABLE_GAMEPAD) # we handle this ourselves
else()
	# on Linux there's no obvious choice, so I just went with GLFW + Vulkan
	target_sources(vodarchiver PRIVATE
		vodarchiver/gui/gui_setup_glfw_vulkan.cpp
		vodarchiver/gui/gui_setup_glfw_vulkan.h
		imgui/backends/imgui_impl_glfw.cpp
		imgui/backends/imgui_impl_vulkan.cpp
	)
	find_package(Vulkan REQUIRED)
	target_link_libraries(vodarchiver PUBLIC Vulkan::Vulkan Vulkan::Headers)

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GLFW3 REQUIRED glfw3>=3.3)
	target_link_libraries(vodarchiver PUBLIC ${GLFW3_LIBRARIES})
	target_include_directories(vodarchiver PUBLIC ${GLFW3_INCLUDE_DIRS})
endif()

add_dependencies(vodarchiver GenerateGitRevisionHeader)

vodarchiver_generate_embeddable_file("../fonts/Cuprum/Cuprum-Regular.ttf.bin" "cuprum.h")
vodarchiver_generate_embeddable_file("../fonts/Noto_Sans_JP/NotoSansJP-Medium.ttf.bin" "noto_sans_jp.h")

if (VODARCHIVER_BUILD_GTESTS)
	enable_testing()
	set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
	set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
	set(GTEST_HAS_ABSL OFF CACHE BOOL "" FORCE)
	add_subdirectory(gtest)

	add_executable(tests)
	target_sources(tests PRIVATE
		test/text_case_test.cpp
		test/timespan_test.cpp

		vodarchiver/time_types.cpp
		vodarchiver/time_types.h

		${SOURCES_UTIL}
	)
	if (WIN32)
		target_compile_definitions(tests PUBLIC BUILD_FOR_WINDOWS)
	endif()
	target_include_directories(tests
		PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/vodarchiver_embeds" "gtest/googletest/include"
	)
	add_dependencies(tests GenerateGitRevisionHeader)
	target_link_libraries(tests PUBLIC GTest::gtest_main)

	include(GoogleTest)
	gtest_discover_tests(tests DISCOVERY_MODE PRE_TEST)
endif()
