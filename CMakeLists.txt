cmake_minimum_required(VERSION 3.1)

project(SDL_audiolib
    VERSION 0.0.0
    DESCRIPTION "An audio decoding, resampling and mixing library."
)
set(AULIB_VERSION 0x000000)

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
    USE_DEC_OPENMPT
    "Enable OpenMPT MOD decoder."
    ON
)

option(
    USE_DEC_XMP
    "Enable XMP MOD decoder."
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
    USE_DEC_MUSEPACK
    "Enable libmpcdec Musepack decoder."
    ON
)

option(
    USE_DEC_FLUIDSYNTH
    "Enable FluidSynth MIDI decoder."
    ON
)

option(
    USE_DEC_BASSMIDI
    "Enable BASSMIDI MIDI decoder."
    ON
)

option(
    USE_DEC_WILDMIDI
    "Enable WildMIDI MIDI decoder."
    ON
)

option(
    USE_DEC_ADLMIDI
    "Enable libADLMIDI decoder."
    ON
)

option(
    ENABLE_SDLMIXER_EMU
    "Build the SDL_mixer emulation library (doesn't really work yet.)"
    OFF
)

option(
    BUILD_EXAMPLE
    "Build the example sound player."
    OFF
)

option(
    BUILD_SHARED_LIBS
    "Build shared library instead of static."
    ON
)

include(FindPkgConfig)
include(GenerateExportHeader)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0063 NEW)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

configure_file(
    ${PROJECT_SOURCE_DIR}/aulib_version.h.in
    ${PROJECT_BINARY_DIR}/aulib_version.h
)

if("${CMAKE_C_COMPILER_ID}" MATCHES "^(GNU|((Apple)?Clang))$")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Wextra -Wpedantic")
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(GNU|((Apple)?Clang))$")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -Wextra -Wpedantic")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DAULIB_DEBUG)
endif()

configure_file (
    ${PROJECT_SOURCE_DIR}/aulib_config.h.in
    ${PROJECT_BINARY_DIR}/aulib_config.h
)

# Headers in include/SDL_Audiolib/
set(
    PUBLIC_HEADERS

    ${CMAKE_BINARY_DIR}/aulib_export.h
    ${CMAKE_BINARY_DIR}/aulib_version.h
    include/aulib.h
    include/aulib_global.h
)

# Headers in include/SDL_Audiolib/Aulib/
set(
    PUBLIC_HEADERS_AULIB_DIR
    include/Aulib/Decoder.h
    include/Aulib/Processor.h
    include/Aulib/Resampler.h
    include/Aulib/ResamplerSpeex.h
    include/Aulib/Stream.h
)

pkg_check_modules(SDL REQUIRED sdl2)
set(PKGCONF_REQ_PUB "${PKGCONF_REQ_PUB} sdl2")
set(LIBRARY_DIRS ${LIBRARY_DIRS} ${SDL_LIBRARY_DIRS})

if (USE_RESAMP_SRC)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/ResamplerSrc.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/ResamplerSrc.h)
    pkg_check_modules(SRC REQUIRED samplerate>=0.1.9)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${SRC_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} samplerate")
endif(USE_RESAMP_SRC)

if (USE_RESAMP_SOXR)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/ResamplerSox.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/ResamplerSox.h)
    pkg_check_modules(SOXR REQUIRED soxr)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${SOXR_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} soxr")
endif(USE_RESAMP_SOXR)

if (USE_DEC_OPENMPT)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderOpenmpt.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderOpenmpt.h)
    pkg_check_modules(OPENMPT REQUIRED libopenmpt)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${OPENMPT_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} libopenmpt")
endif(USE_DEC_OPENMPT)

if (USE_DEC_XMP)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderXmp.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderXmp.h)
    pkg_check_modules(XMP REQUIRED libxmp)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${XMP_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} libxmp")
endif(USE_DEC_XMP)

if (USE_DEC_MODPLUG)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderModplug.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderModplug.h)
    pkg_check_modules(MODPLUG REQUIRED libmodplug)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${MODPLUG_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} libmodplug")
endif(USE_DEC_MODPLUG)

if (USE_DEC_MPG123)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderMpg123.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderMpg123.h)
    pkg_check_modules(MPG123 REQUIRED libmpg123)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${MPG123_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} libmpg123")
endif(USE_DEC_MPG123)

if (USE_DEC_SNDFILE)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderSndfile.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderSndfile.h)
    pkg_check_modules(SNDFILE REQUIRED sndfile)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${SNDFILE_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} sndfile")
endif(USE_DEC_SNDFILE)

if (USE_DEC_LIBVORBIS)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderVorbis.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderVorbis.h)
    pkg_check_modules(VORBISFILE REQUIRED vorbisfile)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${VORBISFILE_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} vorbisfile")
endif(USE_DEC_LIBVORBIS)

if (USE_DEC_LIBOPUSFILE)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderOpus.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderOpus.h)
    pkg_check_modules(OPUSFILE REQUIRED opusfile)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${OPUSFILE_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} opusfile")
endif(USE_DEC_LIBOPUSFILE)

if (USE_DEC_MUSEPACK)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderMusepack.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderMusepack.h)
    set(PKGCONF_LIBS_PRIV "${PKGCONF_LIBS_PRIV} -lmpcdec")
    set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} mpcdec)
endif(USE_DEC_MUSEPACK)

if (USE_DEC_FLUIDSYNTH)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderFluidsynth.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderFluidsynth.h)
    pkg_check_modules(FLUIDSYNTH REQUIRED fluidsynth>=2)
    set(LIBRARY_DIRS ${LIBRARY_DIRS} ${FLUIDSYNTH_LIBRARY_DIRS})
    set(PKGCONF_REQ_PRIV "${PKGCONF_REQ_PRIV} fluidsynth")
endif(USE_DEC_FLUIDSYNTH)

if (USE_DEC_BASSMIDI)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderBassmidi.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderBassmidi.h)
    set(PKGCONF_LIBS_PRIV "${PKGCONF_LIBS_PRIV} -lbass -lbassmidi")
    set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} bass bassmidi)
endif(USE_DEC_BASSMIDI)

if (USE_DEC_WILDMIDI)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderWildmidi.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderWildmidi.h)
    set(PKGCONF_LIBS_PRIV "${PKGCONF_LIBS_PRIV} -lWildMidi")
    set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} WildMidi)
endif(USE_DEC_WILDMIDI)

if (USE_DEC_ADLMIDI)
    set(AULIB_SOURCES ${AULIB_SOURCES} src/DecoderAdlmidi.cpp)
    set(PUBLIC_HEADERS_AULIB_DIR ${PUBLIC_HEADERS_AULIB_DIR} include/Aulib/DecoderAdlmidi.h)
    set(PKGCONF_LIBS_PRIV "${PKGCONF_LIBS_PRIV} -lADLMIDI")
    set(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ADLMIDI)
endif(USE_DEC_ADLMIDI)

link_directories(
    ${LIBRARY_DIRS}
)

add_library(
    SDL_audiolib

    ${PUBLIC_HEADERS}
    ${PUBLIC_HEADERS_AULIB_DIR}
    ${AULIB_SOURCES}

    src/Buffer.h
    src/Decoder.cpp
    src/Processor.cpp
    src/Resampler.cpp
    src/ResamplerSpeex.cpp
    src/SdlAudioLocker.h
    src/Stream.cpp
    src/aulib.cpp
    src/aulib_debug.h
    src/sampleconv.cpp
    src/sampleconv.h
    src/stream_p.cpp
    src/stream_p.h

    src/missing/missing.h
    src/missing/sdl_load_file_rw.c
    src/missing/sdl_load_file_rw.h

    resampler/arch.h
    resampler/resample.c
    resampler/resample_sse.h
    resampler/speex_resampler.h
)

target_compile_definitions(
    SDL_audiolib
    PRIVATE
        -DSPX_RESAMPLE_EXPORT=
        -DOUTSIDE_SPEEX
        -DFLOATING_POINT
        -DRANDOM_PREFIX=SDL_audiolib
)

set_target_properties(
    SDL_audiolib PROPERTIES VERSION ${SDL_audiolib_VERSION} SOVERSION 0
)

generate_export_header(
    SDL_audiolib
    BASE_NAME aulib
)

target_include_directories(
    SDL_audiolib

    PUBLIC
        ${SDL_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_BINARY_DIR}

    PRIVATE
        ${PROJECT_SOURCE_DIR}/resampler
        ${PROJECT_SOURCE_DIR}/src/missing
        ${SRC_INCLUDE_DIRS}
        ${SOXR_INCLUDE_DIRS}
        ${FLUIDSYNTH_INCLUDE_DIRS}
        ${MPG123_INCLUDE_DIRS}
        ${SNDFILE_INCLUDE_DIRS}
        ${VORBISFILE_INCLUDE_DIRS}
        ${OPUSFILE_INCLUDE_DIRS}
        ${OPENMPT_INCLUDE_DIRS}
        ${XMP_INCLUDE_DIRS}

        # MogPlug ships a "sndfile.h" with its headers. This breaks the build,
        # since sndfile.h is expected to belong to libsndfile.
        #${MODPLUG_INCLUDE_DIRS}
)

target_link_libraries(
    SDL_audiolib

    PUBLIC
        ${SDL_LIBRARIES}

    PRIVATE
        ${SRC_LIBRARIES}
        ${SOXR_LIBRARIES}
        ${FLUIDSYNTH_LIBRARIES}
        ${OPENMPT_LIBRARIES}
        ${XMP_LIBRARIES}
        ${MODPLUG_LIBRARIES}
        ${MPG123_LIBRARIES}
        ${SNDFILE_LIBRARIES}
        ${VORBISFILE_LIBRARIES}
        ${OPUSFILE_LIBRARIES}
        ${EXTRA_LIBRARIES}
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
        SDL_mixer-2.0
        src/sdl_mixer_emu/sdl_mixer_emu.h
        src/sdl_mixer_emu/sdl_mixer_emu.cpp
        src/sdl_mixer_emu/music.cpp
    )

    set_target_properties(
        SDL_mixer-2.0 PROPERTIES VERSION 0.0.1 SOVERSION 0
    )

    target_link_libraries(
        SDL_mixer-2.0
        SDL_audiolib
    )

    install(TARGETS SDL_mixer-2.0 DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(ENABLE_SDLMIXER_EMU)

if (BUILD_EXAMPLE)
    add_executable(
        play
        example/main.cpp
    )

    target_link_libraries(
        play
        SDL_audiolib
    )
endif(BUILD_EXAMPLE)

configure_file(
    ${PROJECT_SOURCE_DIR}/Doxyfile.in
    ${PROJECT_BINARY_DIR}/Doxyfile
)
