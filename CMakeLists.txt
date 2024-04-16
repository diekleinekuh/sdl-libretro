cmake_minimum_required(VERSION 3.13)
project(sdl-libretro)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set (SDL-src
    src/SDL.c
    src/SDL_error.c
    src/SDL_fatal.c

    src/audio/SDL_audio.c
    src/audio/SDL_audiocvt.c
    src/audio/SDL_audiodev.c
    src/audio/SDL_mixer.c
    src/audio/SDL_mixer_MMX.c
    src/audio/SDL_mixer_MMX_VC.c
    src/audio/SDL_mixer_m68k.c
    src/audio/SDL_wave.c
    src/audio/libretro/SDL_LIBRETROaudio.c

    src/cdrom/SDL_cdrom.c

    src/cpuinfo/SDL_cpuinfo.c

    src/events/SDL_active.c
    src/events/SDL_events.c
    src/events/SDL_expose.c
    src/events/SDL_keyboard.c
    src/events/SDL_mouse.c
    src/events/SDL_quit.c
    src/events/SDL_resize.c

    src/file/SDL_rwops.c

    src/stdlib/SDL_getenv.c
    src/stdlib/SDL_iconv.c
    src/stdlib/SDL_malloc.c
    src/stdlib/SDL_qsort.c
    src/stdlib/SDL_stdlib.c
    src/stdlib/SDL_string.c

    src/thread/SDL_thread.c

    src/timer/SDL_timer.c
    src/timer/libretro/SDL_systimer.c

    src/video/SDL_RLEaccel.c
    src/video/SDL_blit.c
    src/video/SDL_blit_0.c
    src/video/SDL_blit_1.c 
    src/video/SDL_blit_A.c 
    src/video/SDL_blit_N.c 
    src/video/SDL_bmp.c 
    src/video/SDL_cursor.c 
    src/video/SDL_gamma.c 
    src/video/SDL_pixels.c 
    src/video/SDL_stretch.c 
    src/video/SDL_surface.c 
    src/video/SDL_video.c 
    src/video/SDL_yuv.c 
    src/video/SDL_yuv_mmx.c 
    src/video/SDL_yuv_sw.c
    src/video/libretro/SDL_LIBRETROevents.c 
    src/video/libretro/SDL_LIBRETROkeyboard.c 
    src/video/libretro/SDL_LIBRETROmouse.c 
    src/video/libretro/SDL_LIBRETROvideo.c     

    src/joystick/SDL_joystick.c
    src/joystick/libretro/SDL_sysjoystick.c 

    src/loadso/dummy/SDL_sysloadso.c 
)

add_compile_definitions(__LIBRETRO__ LSB_FIRST )

if (WIN32)
    set( SDL-src ${SDL-src}
        src/cdrom/win32/SDL_syscdrom.c
        src/thread/win32/SDL_sysmutex.c
        src/thread/win32/SDL_syssem.c
        src/thread/win32/SDL_systhread.c
        src/thread/generic/SDL_syscond.c
    )    
else()
    set( SDL-src ${SDL-src}
        src/cdrom/linux/SDL_syscdrom.c
        src/thread/pthread/SDL_syscond.c
        src/thread/pthread/SDL_sysmutex.c
        src/thread/pthread/SDL_syssem.c
        src/thread/pthread/SDL_systhread.c
    )
    add_compile_definitions(__LINUX__ )
endif()

add_library( sdl-libretro STATIC ${SDL-src})
target_link_libraries(sdl-libretro Threads::Threads)
include_directories(include)


add_compile_definitions(WAV_MUSIC OGG_MUSIC MOD_MUSIC)
set (SDL_mixer-src
    tests/SDL_mixer-1.2.12/dynamic_flac.c
    tests/SDL_mixer-1.2.12/dynamic_fluidsynth.c
    tests/SDL_mixer-1.2.12/dynamic_mod.c
    tests/SDL_mixer-1.2.12/dynamic_mp3.c
    tests/SDL_mixer-1.2.12/dynamic_ogg.c
    tests/SDL_mixer-1.2.12/effect_position.c
    tests/SDL_mixer-1.2.12/effect_stereoreverse.c
    tests/SDL_mixer-1.2.12/effects_internal.c
    tests/SDL_mixer-1.2.12/fluidsynth.c
    tests/SDL_mixer-1.2.12/load_aiff.c
    tests/SDL_mixer-1.2.12/load_flac.c
    tests/SDL_mixer-1.2.12/load_ogg.c
    tests/SDL_mixer-1.2.12/load_voc.c
    tests/SDL_mixer-1.2.12/mixer.c
    tests/SDL_mixer-1.2.12/music.c
    tests/SDL_mixer-1.2.12/music_cmd.c
    tests/SDL_mixer-1.2.12/music_flac.c
    tests/SDL_mixer-1.2.12/music_mad.c
    tests/SDL_mixer-1.2.12/music_mod.c
    tests/SDL_mixer-1.2.12/music_modplug.c
    tests/SDL_mixer-1.2.12/music_ogg.c
    tests/SDL_mixer-1.2.12/wavestream.c
)

add_library( sdl_mixer-libretro STATIC ${SDL_mixer-src})