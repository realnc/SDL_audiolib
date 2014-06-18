cmake_minimum_required(VERSION 2.8)
project(SDL_audiolib)
include(CheckCXXCompilerFlag)
include(FindPkgConfig)
include(GenerateExportHeader)

set(AULIB_VERSION_STR "0.0.0")
set(AULIB_VERSION 0x000000)

configure_file(
    ${PROJECT_SOURCE_DIR}/aulib_version.h.in
    ${PROJECT_BINARY_DIR}/aulib_version.h
)

option(
    USE_SDL2
    "Use SDL 2 instead of SDL 1."
    ON
)

option(
    USE_RESAMP_SRC
    "Enable SRC (aka libsamplerate) resampler."
    ON
)

option(
    USE_RESAMP_SOXR
    "Enable SoxR resampler."
    ON
)

option(
    USE_DEC_MODPLUG
    "Enable ModPlug MOD decoder."
    ON
)

option(
    USE_DEC_MPG123
    "Enable Mpg123 MP3 decoder."
    ON
)

option(
    USE_DEC_SNDFILE
    "Enable Libsndfile decoder."
    ON
)

option(
    USE_DEC_LIBVORBIS
    "Enable libvorbis Ogg Vorbis decoder."
    ON
)

option(
    USE_DEC_LIBOPUSFILE
    "Enable libopusfile Opus decoder."
    ON
)

option(
    USE_DEC_FLUIDSYNTH
    "Enable FluidSynth MIDI decoder."
    ON
)

option(
    ENABLE_SDLMIXER_EMU
    "Build the SDL_mixer emulation library (doesn't really work yet.)"
    OFF
)

option(
    WITH_BUNDLED_BOOST
    "Use the bundled version of the boost headers."
    ON
)

option(
    USE_CXX11
    "Enable C++11 mode (GCC, Clang)."
    OFF
)

if (WITH_BUNDLED_BOOST)
    include_directories(${PROJECT_SOURCE_DIR}/boost)
endif(WITH_BUNDLED_BOOST)

if(USE_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    else()
        CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
        if(COMPILER_SUPPORTS_CXX0X)
            set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
        else()
            message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
        endif(COMPILER_SUPPORTS_CXX0X)
    endif(COMPILER_SUPPORTS_CXX11)
endif(USE_CXX11)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "-Wall -W -Wextra ${CMAKE_CXX_FLAGS}")
endif(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)

add_compiler_export_flags()

add_definitions(
    -DSPX_RESAMPLE_EXPORT=
    -DOUTSIDE_SPEEX
    -DFLOATING_POINT
    -DRANDOM_PREFIX=SDL_audiolib
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DAULIB_DEBUG)
endif()

configure_file (
    ${PROJECT_SOURCE_DIR}/aulib_config.h.in
    ${PROJECT_BINARY_DIR}/aulib_config.h
)

include_directories(${PROJECT_BINARY_DIR})

# Headers in include/SDL_Audiolib/
set(
    PUBLIC_HEADERS

    src/aulib_global.h
    ${CMAKE_BINARY_DIR}/aulib_export.h
    ${CMAKE_BINARY_DIR}/aulib_version.h
    src/aulib.h
)

# Headers in include/SDL_Audiolib/Aulib/
set(
    PUBLIC_HEADERS_AULIB_DIR
    include/Aulib/AudioDecoder.h
    include/Aulib/AudioResamplerSpeex.h
    include/Aulib/AudioResampler.h
    include/Aulib/Stream.h
    include/Aulib/AudioStream.h
)

if (USE_SDL2)
    pkg_check_modules(SDL REQUIRED sdl2)
    set(PKGCONF_REQ_PUB "${PKGCONF_REQ_PUB} sdl2")
else()
    pkg_check_modules(SDL REQUIRED sdl)
    set(PKGCONF_REQ_PUB "${PKGCONF_REQ_PUB} sdl")
endif(USE_SDL2)
set(LIBRARY_DIRS ${LIBRARY_DIRS} ${SDL_LIBRARY_DIRS})

if (USE_RESAMP_SRC)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/AudioResamplerSrc.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/AudioResamplerSrc.h)
    pkg_check_modules(SRC REQUIRED samplerate)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${SRC_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} samplerate")
endif(USE_RESAMP_SRC)

if (USE_RESAMP_SOXR)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/AudioResamplerSox.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/AudioResamplerSox.h)
    pkg_check_modules(SOXR REQUIRED soxr)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${SOXR_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} soxr")
endif(USE_RESAMP_SOXR)

if (USE_DEC_MODPLUG)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/AudioDecoderModplug.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/AudioDecoderModplug.h)
    pkg_check_modules(MODPLUG REQUIRED libmodplug)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${MODPLUG_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} libmodplug")
endif(USE_DEC_MODPLUG)

if (USE_DEC_MPG123)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/AudioDecoderMpg123.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/AudioDecoderMpg123.h)
    pkg_check_modules(MPG123 REQUIRED libmpg123)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${MPG123_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} libmpg123")
endif(USE_DEC_MPG123)

if (USE_DEC_SNDFILE)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/AudioDecoderSndfile.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/AudioDecoderSndfile.h)
    pkg_check_modules(SNDFILE REQUIRED sndfile)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${SNDFILE_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} sndfile")
endif(USE_DEC_SNDFILE)

if (USE_DEC_LIBVORBIS)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/AudioDecoderVorbis.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/AudioDecoderVorbis.h)
    pkg_check_modules(VORBISFILE REQUIRED vorbisfile)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${VORBISFILE_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} vorbisfile")
endif(USE_DEC_LIBVORBIS)

if (USE_DEC_LIBOPUSFILE)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/AudioDecoderOpus.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/AudioDecoderOpus.h)
    pkg_check_modules(OPUSFILE REQUIRED opusfile)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${OPUSFILE_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} opusfile")
endif(USE_DEC_LIBOPUSFILE)

if (USE_DEC_FLUIDSYNTH)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/AudioDecoderFluidsynth.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/AudioDecoderFluidsynth.h)
    pkg_check_modules(FLUIDSYNTH REQUIRED fluidsynth)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${FLUIDSYNTH_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} fluidsynth")
endif(USE_DEC_FLUIDSYNTH)

link_directories(
    ${LIBRARY_DIRS}
)

add_library(
    SDL_audiolib

    ${PUBLIC_HEADERS}
    ${PUBLIC_HEADERS_AULIB_DIR}
    ${AULIB_SOURCES}

    src/aulib_debug.h
    src/aulib.cpp
    src/AudioDecoder.cpp
    src/AudioResampler.cpp
    src/AudioStream.cpp
    src/audiostream_p.h
    src/audiostream_p.cpp
    src/AudioResamplerSpeex.cpp
    src/sampleconv.h
    src/sampleconv.cpp
    src/Stream.cpp
    resampler/arch.h
    resampler/resample_sse.h
    resampler/speex_resampler.h
    resampler/stack_alloc.h
    resampler/resample.c
)

set_target_properties(
    SDL_audiolib PROPERTIES VERSION 0.0.0 SOVERSION 0
)

generate_export_header(
    SDL_audiolib
    BASE_NAME aulib
)

include_directories(
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/resampler
    ${PROJECT_SOURCE_DIR}/include
    ${SDL_INCLUDE_DIRS}
    ${SRC_INCLUDE_DIRS}
    ${SOXR_INCLUDE_DIRS}
    ${FLUIDSYNTH_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}
    ${SNDFILE_INCLUDE_DIRS}
    ${VORBISFILE_INCLUDE_DIRS}
    ${OPUSFILE_INCLUDE_DIRS}

    # MogPlug ships a "sndfile.h" with its headers. This breaks the build,
    # since sndfile.h is expected to belong to libsndfile.
    #${MODPLUG_INCLUDE_DIRS}
)

target_link_libraries(
    SDL_audiolib
    ${SDL_LIBRARIES}
    ${SRC_LIBRARIES}
    ${SOXR_LIBRARIES}
    ${FLUIDSYNTH_LIBRARIES}
    ${MODPLUG_LIBRARIES}
    ${MPG123_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${VORBISFILE_LIBRARIES}
    ${OPUSFILE_LIBRARIES}
)

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries.")
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)

configure_file(
    ${PROJECT_SOURCE_DIR}/SDL_audiolib.pc.in
    ${PROJECT_BINARY_DIR}/SDL_audiolib.pc
    @ONLY
)

install(TARGETS SDL_audiolib DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES ${PUBLIC_HEADERS} DESTINATION include/SDL_audiolib)
install(FILES ${PUBLIC_HEADERS_AULIB_DIR} DESTINATION include/SDL_audiolib/Aulib)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SDL_audiolib.pc
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if (ENABLE_SDLMIXER_EMU)
    add_library(
        SDL_mixer-1.2
        src/sdl_mixer_emu/sdl_mixer_emu.h
        src/sdl_mixer_emu/sdl_mixer_emu.cpp
        src/sdl_mixer_emu/music.cpp
    )

    set_target_properties(
        SDL_mixer-1.2 PROPERTIES VERSION 0.12.0 SOVERSION 0
    )

    target_link_libraries(
        SDL_mixer-1.2
        SDL_audiolib
    )

    install(TARGETS SDL_mixer-1.2 DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(ENABLE_SDLMIXER_EMU)

configure_file(
    ${PROJECT_SOURCE_DIR}/Doxyfile.in
    ${PROJECT_BINARY_DIR}/Doxyfile
)
