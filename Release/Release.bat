@rem Copyright (c) MediaArea.net SARL. All Rights Reserved.
@rem
@rem Use of this source code is governed by a BSD-style license that can
@rem be found in the License.html file in the root of the source tree.
@rem

@echo off

rem --- Search binaries ---
set BPATH=
if exist "%~dp0\..\..\..\MediaArea-Utils-Binaries" set BPATH="%~dp0\..\..\..\MediaArea-Utils-Binaries"
if exist "%~dp0\..\..\MediaArea-Utils-Binaries" set BPATH="%~dp0\..\..\MediaArea-Utils-Binaries"
if "%BPATH%"=="" (
    echo "ERROR: binaries path not found"
    exit /b 1
)

rem --- Clean up ---
del MediaInfoLib_Demos.7z
rmdir MediaInfoLib_Demos /S /Q
mkdir MediaInfoLib_Demos

@rem --- Copying : Bin ---
mkdir MediaInfoLib_Demos\Bin
mkdir MediaInfoLib_Demos\Bin\Windows
mkdir MediaInfoLib_Demos\Bin\Windows\Win32
mkdir MediaInfoLib_Demos\Bin\Windows\x64
copy ..\..\MediaInfoLib\Project\MSVC2015\Win32\Release\MediaInfo.dll MediaInfoLib_Demos\Bin\Windows\Win32
copy ..\..\MediaInfoLib\Project\MSVC2015\x64\Release\MediaInfo.dll MediaInfoLib_Demos\Bin\Windows\x64

@rem --- Copying : Doc ---
mkdir MediaInfoLib_Demos\Doc
powershell -Command "(Get-Content ..\README.md) | ForEach-Object { $_ -replace '.md', '.html' } | Set-Content MediaInfoLib_Demos\README.md"
pandoc MediaInfoLib_Demos\README.md -o MediaInfoLib_Demos\Demos.html -f markdown_github -s
pandoc ..\Doc\ReadGrowing.md -o MediaInfoLib_Demos\Doc\ReadGrowing.html -f markdown_github -s
del MediaInfoLib_Demos\README.md
powershell -Command "(Get-Content MediaInfoLib_Demos\Demos.html) | ForEach-Object { $_ -replace '</style>', '</style><link rel=\"stylesheet\" href=\"Doc/style.css\" type=\"text/css\" />' } | Set-Content MediaInfoLib_Demos\Demos.html"
powershell -Command "(Get-Content MediaInfoLib_Demos\Doc\ReadGrowing.html) | ForEach-Object { $_ -replace '</style>', '</style><link rel=\"stylesheet\" href=\"style.css\" type=\"text/css\" />' } | Set-Content MediaInfoLib_Demos\Doc\ReadGrowing.html"
copy ..\Doc\style.css MediaInfoLib_Demos\Doc

@rem --- Copying : Include ---
mkdir MediaInfoLib_Demos\Include
mkdir MediaInfoLib_Demos\Include\MediaInfo
mkdir MediaInfoLib_Demos\Include\MediaInfoDLL
copy ..\..\MediaInfoLib\Source\MediaInfo\MediaInfo.h MediaInfoLib_Demos\Include\MediaInfo
copy ..\..\MediaInfoLib\Source\MediaInfo\MediaInfo_Events.h MediaInfoLib_Demos\Include\MediaInfo
copy ..\..\MediaInfoLib\Source\MediaInfoDLL\MediaInfoDLL.h MediaInfoLib_Demos\Include\MediaInfoDLL

@rem --- Copying : Project ---
mkdir MediaInfoLib_Demos\Project
xcopy ..\Project\*.sln MediaInfoLib_Demos\Project /S
xcopy ..\Project\*.suo MediaInfoLib_Demos\Project /S /h
xcopy ..\Project\*.vcxproj MediaInfoLib_Demos\Project /S
xcopy ..\Project\*.vcxproj.filters MediaInfoLib_Demos\Project /S
xcopy ..\Project\*.vcxproj.user MediaInfoLib_Demos\Project /S

@rem --- Copying : Result ---
mkdir MediaInfoLib_Demos\Result

@rem --- Copying : Sample ---
mkdir MediaInfoLib_Demos\Sample
copy ..\..\MediaInfoLib_Demos_Data\Sample\* MediaInfoLib_Demos\Sample

@rem --- Copying : Source ---
mkdir MediaInfoLib_Demos\Source
copy ..\Source\* MediaInfoLib_Demos\Source


rem --- Compressing Archive ---
if "%2"=="SkipCompression" goto SkipCompression
%BPATH%\Windows\7-zip\7z a -r -t7z -mx9 MediaInfoLib_Demos.7z MediaInfoLib_Demos\*
:SkipCompression

rem --- Clean up ---
if "%1"=="SkipCleanUp" goto SkipCleanUp
rem rmdir MediaInfoLib_Demos /S /Q
:SkipCleanUp
