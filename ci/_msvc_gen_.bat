@echo off

set TYPE=vs2022
set ROOT_DIR=.\
set SOLUTION_NAME=Akvarium

set script_dir=%~dp0
set script_dir=%script_dir:\=/%
cd %script_dir%

set BUILD_DIR=%ROOT_DIR%build_%SOLUTION_NAME%_%TYPE%

echo configure of TYPE "%TYPE%" for "%SOLUTION_NAME%" started...
echo root dir: %ROOT_DIR%
echo build dir: %BUILD_DIR%

if not exist %BUILD_DIR% (
	mkdir %BUILD_DIR%
)

MKLINK /H /J "%script_dir%/include/mshadow" "3rdparty/mshadow/mshadow" 

cd %BUILD_DIR%

cmake ^
-DCMAKE_CXX_COMPILER=MSVC ^
-DBOOST_ROOT=c:/LIBRARIES/Boost ^
-DBOOST_LIBRARYDIR=c:/LIBRARIES/Boost/stage/lib ^
-DRAPIDJSON_ROOT=c:/LIBRARIES/RapidJson ^
-DTEST_ROOT=c:/LIBRARIES ^
-G "Visual Studio 17 2022" ^
%script_dir%

rem -G "Visual Studio 16 2019" -A Win32 ^
