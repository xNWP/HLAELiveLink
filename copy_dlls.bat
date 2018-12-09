SETLOCAL
SET vcr="C:\xCode\vcpkg\installed\x64-windows\bin"

COPY /Y %vcr%\libuv.dll res\libs\win64
COPY /Y %vcr%\zlib1.dll res\libs\win64
COPY /Y %vcr%\libeay32.dll res\libs\win64
COPY /Y %vcr%\ssleay32.dll res\libs\win64