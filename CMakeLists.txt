cmake_minimum_required (VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)

project ("webkitten")

file(COPY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)

# copy important internet stuff
if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64
    add_compile_definitions(BITNESS64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32
    add_compile_definitions(BITNESS32)
endif()
file(COPY ${CMAKE_SOURCE_DIR}/cacert.pem DESTINATION ${CMAKE_BINARY_DIR}/)

# Set source files
set(MAIN_SOURCE
    src/main.cpp
    src/logger.cpp

    # internal
        # gsgl
            src/internal/gsgl/graphics.cpp
            src/internal/gsgl/text.cpp
            src/internal/gsgl/utils.cpp
            # libs

    # libs
        src/libs/backward.cpp
        src/libs/tinyxml2.cpp

    # classes
        # main
            src/classes/main/handler.cpp
            src/classes/main/renderer.cpp
            src/classes/main/networker.cpp
            src/classes/main/scripter.cpp
            src/classes/main/request.cpp
        # tab
            src/classes/tab/tab.cpp
        # ui
            src/classes/ui/fonts.cpp
            src/classes/ui/button.cpp
            src/classes/ui/input.cpp
            # classes
                src/classes/ui/classes/input.cpp
)
set(MAIN_HEADERS
    src/main.h
    src/logger.h

    # internal
        # gsgl
            src/internal/gsgl/gsgl.h
            # libs
                src/internal/gsgl/libs/stb_truetype.h

    # libs
        src/libs/json.hpp
        src/libs/backward.hpp
        src/libs/tinyxml2.h

    # classes
        # main
            src/classes/main/handler.h
            src/classes/main/renderer.h
            src/classes/main/networker.h
            src/classes/main/scripter.h
            src/classes/main/request.h
        # tab
            src/classes/tab/tab.h
        # ui
            src/classes/ui/fonts.h
            src/classes/ui/ui.h
            # classes
                src/classes/ui/classes/input.h
)

set(IMGUI_SOURCE
    # Dear ImGui
        src/imgui/imgui.cpp
        src/imgui/imgui_demo.cpp
        src/imgui/imgui_draw.cpp
        src/imgui/imgui_tables.cpp
        src/imgui/imgui_widgets.cpp
)
set(IMGUI_HEADERS
    # Dear ImGui
        src/imgui/imstb_rectpack.h
        src/imgui/imstb_textedit.h
        src/imgui/imstb_truetype.h
        src/imgui/imgui_internal.h
        src/imgui/imconfig.h
        src/imgui/imgui.h
)

add_executable(${PROJECT_NAME} ${MAIN_HEADERS} ${MAIN_SOURCE})

# Packages

if (WIN32)
    list(APPEND PLATFORM_LIBRARIES Winmm.lib)

    find_package(curl QUIET)
    if (NOT curl_FOUND)
        message("libcurl not found, attempting to use external binaries")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64
            message("using 64-bit curl")

            include_directories("${CMAKE_SOURCE_DIR}/curl64/include/")
            target_include_directories(${PROJECT_NAME} PUBLIC curl64/)

            set(CURL_LIBRARY "${CMAKE_SOURCE_DIR}/curl64/lib/libcurl.dll.a")
            set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/curl64/include/")

            file(COPY ${CMAKE_SOURCE_DIR}/curl64/bin/libcurl-x64.dll DESTINATION ${CMAKE_BINARY_DIR}/)
            list(APPEND PLATFORM_LIBRARIES CURL::libcurl)

            find_package(curl REQUIRED)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32
            message("using 32-bit curl")

            include_directories("${CMAKE_SOURCE_DIR}/curl32/include/")
            target_include_directories(${PROJECT_NAME} PUBLIC curl32/)

            set(CURL_LIBRARY "${CMAKE_SOURCE_DIR}/curl32/lib/libcurl.dll.a")
            set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/curl32/include/")

            file(COPY ${CMAKE_SOURCE_DIR}/curl32/bin/libcurl.dll DESTINATION ${CMAKE_BINARY_DIR}/)
            list(APPEND PLATFORM_LIBRARIES CURL::libcurl)

            find_package(curl REQUIRED)
        else()
            message("couldn't determine bitness." FATAL)
        endif()
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LINUXPKGS REQUIRED libcurl x11)
    include_directories(${LINUXPKGS_INCLUDE_DIRS})
    add_compile_options(${LINUXPKGS_CFLAGS_OTHER})
    list(APPEND PLATFORM_LIBRARIES ${LINUXPKGS_LDFLAGS})
endif()

# build V8 here
# not now though, get the general web stuff done before doing that
# https://v8.dev/docs/build

target_link_libraries(${PROJECT_NAME} ${PLATFORM_LIBRARIES})