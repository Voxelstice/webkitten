> [!CAUTION]
> This browser is not guranteed to be safe. While it does try to follow basic security practices, some things may be unsafe. Use at your own risk.
> This is also a work in progress. Expect instability from time to time, and report bugs as needed.

# tinyweb
A lightweight browser that can load and render basic websites.

## Why?
The reason I began this project is because there didn't exist any other modern, performant and lightweight browser that could run on my Futijsu Lifebook P1510 a little bit smoother. Another reason of this project is to test my own knowledge, and mostly just for fun. This may not make it to the final goal.

## Current state
There is some basic UI, with tabs that can be clicked on and closed. Some buttons that can be clicked on. And a input bar that can be clicked on and typed.

# Libraries used
- [nlohmann/json](https://github.com/nlohmann/json) - MIT
- [backward-cpp](https://github.com/bombela/backward-cpp) - MIT
- [Dear ImGui](https://github.com/ocornut/imgui) - MIT
- [tinyxml2](https://github.com/leethomason/tinyxml2) - Zlib
- [libcurl](https://curl.se/libcurl/) - [curl license](https://curl.se/docs/copyright.html)

# Roadmap
- [ ] Get networking and graphics working
    - [X] win32
        - [X] Get window creation working
        - [X] Software renderer
            - [X] Basic shapes
            - [X] Text
        - [ ] Hardware renderer
            - [ ] Basic shapes
            - [ ] Text
    - [ ] X11
        - [ ] Get window creation working
        - [ ] Software renderer
        - [ ] Hardware renderer
    - [X] Draw the received HTML data from networking
- [ ] Get a basic subset of HTML working
    - TODO
- [ ] Linux port of some stuff
- [ ] Get the full subset of HTML working
    - TODO
- [ ] Get the V8 JavaScript engine working
    - TODO

# Building
Currently, only 2 platforms are supported.

## Windows
You shouldn't need to do any other major steps to get it building other than getting the libcurl binary in the right place.

### libcurl
This would have had an automatic building process but I couldn't quite get OpenSSL to build in Windows

- Go to https://curl.se/download.html
- Scroll down until you find "Windows 64-bit binary the curl project" highlighted in yellow. Click on the version number.
    - As of writing, this project currently uses libcurl 8.11.1
    - You can get that information in the app logs
- Click on "curl for 64-bit".
- Extract the recently-downloaded ZIP file and rename the curl folder to ``curl64``. Then put it in the project folder.
- If you're using a 32-bit machine, follow the same steps but select ``32-bit`` instead. And rename the folder to ``curl32``.

### Certificate
libcurl requires a certificate file in order to attempt HTTPS connections. [Get it here.](https://curl.se/docs/caextract.html) After you download the certificate file, rename it to ``cacert.pem`` and move it to the project folder.

### Starting the build
This project uses CMake and Ninja. I develop this on Windows using Visual Studio Code with the ``C/C++``, ``CMake``, and ``CMake Tools`` extensions. Then you need the MSVC compiler (which can be obtained by installing it from Microsoft's official site, or by downloading Visual Studio 2022 and installing "Game development for C++" without anything but MSVC).

If you're using Visual Studio Code, it's as simple as setting the preset to ``x64 Debug`` and just clicking the little play button (``Launch``).
If you're using 32-bit, select the ``x86 Debug`` preset.

Otherwise, here are the commands that Visual Studio Code runs. Note that the file paths are specific to my setup. I will sort it out to something more user-friendly at one point.
```
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe -DCMAKE_INSTALL_PREFIX=C:/Stuff/programming/c++/tinyweb/out/install/x64-debug -SC:/Stuff/programming/c++/tinyweb -BC:/Stuff/programming/c++/tinyweb/out/build/x64-debug -G Ninja
cmake --build C:/Stuff/programming/c++/tinyweb/out/build/x64-debug --parallel 6 --target tinyweb
```

## Linux
This is a LOT easier since you have a package manager out of the box. But first, make sure you install the following packages:
```
sudo apt install pkg-config g++ cmake git
```

### libcurl
You can simply install ``libcurl4-openssl-dev`` via ``apt`` or whatever package manager you have.
```
sudo apt install libcurl4-openssl-dev
```

### Starting the build
TODO