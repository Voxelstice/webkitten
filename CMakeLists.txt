cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

project ("tinyweb")

file(COPY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)

# copy important internet stuff
file(COPY ${CMAKE_SOURCE_DIR}/curl/bin/libcurl-x64.dll DESTINATION ${CMAKE_BINARY_DIR}/)
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
            src/classes/main/networkerBarrier.cpp
            src/classes/main/scripter.cpp
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
            src/classes/main/networkerBarrier.h
            src/classes/main/scripter.h
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

# Curl
# required for networking
set(BUILD_CURL OFF)

if (BUILD_CURL)
    # i haven't gotten this to work too well
    # this builds curl from scratch

    # OpenSSL
    set(OPENSSL_ROOT_DIR "${CMAKE_SOURCE_DIR}/openssl/")
    set(OPENSSL_CRYPTO_LIBRARY "${CMAKE_SOURCE_DIR}/openssl/crypto/")
    set(OPENSSL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/openssl/include/")

    include_directories("${CMAKE_SOURCE_DIR}/openssl/include/")
    target_include_directories(${PROJECT_NAME} PUBLIC openssl/)
    add_subdirectory(openssl)

    # libcurl
    set(CURL_VERSION 8.11.1)
    find_package(curl QUIET)
    if (NOT curl_FOUND)
        FetchContent_Declare(curl
            URL https://curl.se/download/curl-${CURL_VERSION}.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP true
            OVERRIDE_FIND_PACKAGE
        )

        set(CURL_USE_OPENSSL ON)
        FetchContent_MakeAvailable(curl)
        find_package(curl)
    endif()
else()
    # this works better
    # this uses a pre-built curl

    include_directories("${CMAKE_SOURCE_DIR}/curl/include/")
    target_include_directories(${PROJECT_NAME} PUBLIC curl/)

    set(CURL_LIBRARY "${CMAKE_SOURCE_DIR}/curl/lib/libcurl.dll.a")
    set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/curl/include/")

    find_package(curl REQUIRED)
endif()

# build V8 here
# not now though, get the general web stuff done before doing that
# https://v8.dev/docs/build

target_link_libraries(${PROJECT_NAME} Winmm.lib CURL::libcurl)