call parameters.bat

@if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%" || exit /b 1

cmake -B "%BUILD_DIR%" -G "%GENERATOR%" -A "%PLATFORM%" ^
  -DCMAKE_TOOLCHAIN_FILE:PATH="%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake" ^
  -DCMAKE_MODULE_PATH:PATH="%CMAKE_MODULE_PATH%" ^
  -DEXAMPLES=1 ^
  || exit /b 1