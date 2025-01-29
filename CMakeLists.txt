cmake_minimum_required (VERSION 3.12)
set(CMAKE_CXX_STANDARD 17)

project ("tinyweb")

file(COPY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets/)

# copy important internet stuff
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

find_package(curl REQUIRED)

# build V8 here
# not now though, get the general web stuff done before doing that
# https://v8.dev/docs/build

target_link_libraries(${PROJECT_NAME} Winmm.lib CURL::libcurl)