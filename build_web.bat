
@echo off
setlocal

rem -- Adjust these paths --- 
set EMSDK=C:\emsdk 
set RAYLIB=..\external\Raylib
set SRC=..\garden.cpp 
set OUTDIR=dist 
set OUTNAME=garden

rem --- Load emscripten env for this shell --- 
call "%EMSDK%\emsdk_env.bat"

if NOT EXIST %OUTDIR% mkdir %OUTDIR% 
pushd %OUTDIR%

rem Complile + link to HTML/WASM
rem Use em++ (handles .cpp). It can compile the C raylib sources too.
emcc ^
  %SRC% ^
  %RAYLIB%\rcore.c ^
  %RAYLIB%\rmodels.c ^
  %RAYLIB%\raudio.c ^
  %RAYLIB%\rshapes.c ^
  %RAYLIB%\rtext.c ^
  %RAYLIB%\rtextures.c ^
  %RAYLIB%\utils.c ^
  -I"%RAYLIB%" ^
  -I"..\include" ^
  -DPLATFORM_WEB ^
  -DGRAPHICS_API_OPENGL_ES2 ^
  -sUSE_GLFW=3 ^
  -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 ^
  -sFULL_ES3=1 ^
  -sALLOW_MEMORY_GROWTH=1 ^
  -sASYNCIFY ^
  --preload-file "..\assets@assets" ^
  -O3 ^
  -o "%OUTNAME%.html"

popd

  echo.
  echo Built %OUTDIR%\%OUTNAME%.html 
  echo Run a local server in the dist/ folder: 
  echo python -m http.server 8080
  echo then open: http://localhost:8080/%OUTNAME%.html 
  echo.

  endlocal
