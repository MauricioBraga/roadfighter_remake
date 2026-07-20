cmake_minimum_required(VERSION 3.16)
project(roadfighter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# All these are provided by vcpkg (Windows/VS Code) or by your system's
# SDL3 packages (Linux/macOS)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)

file(GLOB ROADFIGHTER_SOURCES CONFIGURE_DEPENDS src/*.cpp)

# On Windows, an executable is built as a "Console" subsystem app by
# default, which pops up a console/terminal window alongside the actual
# game window. WIN32 here tells the linker to build it as a GUI-subsystem
# app instead, so only the game's own window shows up. This has no effect
# on Linux/macOS (WIN32 is simply ignored there).
if (WIN32)
	add_executable(roadfighter WIN32 ${ROADFIGHTER_SOURCES})
else()
	add_executable(roadfighter ${ROADFIGHTER_SOURCES})
endif()
target_include_directories(roadfighter PRIVATE src)

target_link_libraries(roadfighter PRIVATE
	SDL3::SDL3
	SDL3_image::SDL3_image
	SDL3_ttf::SDL3_ttf
	SDL3_mixer::SDL3_mixer
)

if (WIN32)
	target_link_libraries(roadfighter PRIVATE winmm)
endif()

# RoadFighter loads its assets (fonts/graphics/maps/sound) using paths that
# are relative to the current working directory at run time (e.g.
# "graphics/disclaimer.jpg"), so the asset folders need to sit next to the
# executable. This copies them into the build output directory after every
# build.
foreach(asset_dir fonts graphics maps sound)
	add_custom_command(TARGET roadfighter POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			${CMAKE_SOURCE_DIR}/${asset_dir}
			$<TARGET_FILE_DIR:roadfighter>/${asset_dir}
	)
endforeach()

# On Windows, vcpkg builds shared (DLL) SDL3 libraries by default; copy them
# next to the .exe so it can actually run without installing them system-wide.
if (WIN32)
	add_custom_command(TARGET roadfighter POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_RUNTIME_DLLS:roadfighter>
			$<TARGET_FILE_DIR:roadfighter>
		COMMAND_EXPAND_LISTS
	)

	# SDL3_image (and, depending on how it was built, SDL3_mixer) loads its
	# per-format codec libraries dynamically at runtime via LoadLibrary,
	# instead of linking against them at load time - e.g. libjpeg is only
	# opened the moment the game actually tries to decode a .jpg. That
	# makes these DLLs invisible to $<TARGET_RUNTIME_DLLS:...> above (and
	# to dependency-walking tools like ldd/Dependencies), since they never
	# show up in roadfighter.exe's/SDL3_image.dll's import table. They have
	# to be located and copied explicitly by name instead.
	set(RUNTIME_PLUGIN_DLLS
		libjpeg-8.dll
		libpng16-16.dll
		libtiff-6.dll
		libwebp-7.dll
		libwebpdemux-2.dll
	)
	foreach(dll_name ${RUNTIME_PLUGIN_DLLS})
		find_file(FOUND_${dll_name} NAMES ${dll_name})
		if (FOUND_${dll_name})
			add_custom_command(TARGET roadfighter POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different
					${FOUND_${dll_name}}
					$<TARGET_FILE_DIR:roadfighter>
			)
		else()
			message(WARNING "Optional runtime-loaded DLL not found, skipping: ${dll_name} (only a problem if the game actually needs that format at runtime)")
		endif()
	endforeach()
endif()
