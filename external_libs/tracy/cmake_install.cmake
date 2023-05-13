# Install script for directory: W:/Priscilla/external_libs/tracy

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Priscilla")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "W:/Priscilla/external_libs/tracy/Debug/TracyClient.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "W:/Priscilla/external_libs/tracy/Release/TracyClient.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "W:/Priscilla/external_libs/tracy/MinSizeRel/TracyClient.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "W:/Priscilla/external_libs/tracy/RelWithDebInfo/TracyClient.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "W:/Priscilla/external_libs/tracy/TracyC.h"
    "W:/Priscilla/external_libs/tracy/Tracy.hpp"
    "W:/Priscilla/external_libs/tracy/TracyD3D11.hpp"
    "W:/Priscilla/external_libs/tracy/TracyD3D12.hpp"
    "W:/Priscilla/external_libs/tracy/TracyLua.hpp"
    "W:/Priscilla/external_libs/tracy/TracyOpenCL.hpp"
    "W:/Priscilla/external_libs/tracy/TracyOpenGL.hpp"
    "W:/Priscilla/external_libs/tracy/TracyVulkan.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/client" TYPE FILE FILES
    "W:/Priscilla/external_libs/tracy/client/tracy_concurrentqueue.h"
    "W:/Priscilla/external_libs/tracy/client/tracy_rpmalloc.hpp"
    "W:/Priscilla/external_libs/tracy/client/tracy_SPSCQueue.h"
    "W:/Priscilla/external_libs/tracy/client/TracyArmCpuTable.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyCallstack.h"
    "W:/Priscilla/external_libs/tracy/client/TracyCallstack.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyDebug.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyDxt1.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyFastVector.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyLock.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyProfiler.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyRingBuffer.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyScoped.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyStringHelpers.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracySysTime.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracySysTrace.hpp"
    "W:/Priscilla/external_libs/tracy/client/TracyThread.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/common" TYPE FILE FILES
    "W:/Priscilla/external_libs/tracy/common/tracy_lz4.hpp"
    "W:/Priscilla/external_libs/tracy/common/tracy_lz4hc.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyAlign.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyAlign.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyAlloc.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyApi.h"
    "W:/Priscilla/external_libs/tracy/common/TracyColor.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyForceInline.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyMutex.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyProtocol.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyQueue.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracySocket.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyStackFrames.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracySystem.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyUwp.hpp"
    "W:/Priscilla/external_libs/tracy/common/TracyYield.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/Tracy/TracyConfig.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/Tracy/TracyConfig.cmake"
         "W:/Priscilla/external_libs/tracy/CMakeFiles/Export/7430802ac276f58e70c46cf34d169c6f/TracyConfig.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/Tracy/TracyConfig-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/Tracy/TracyConfig.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/Tracy" TYPE FILE FILES "W:/Priscilla/external_libs/tracy/CMakeFiles/Export/7430802ac276f58e70c46cf34d169c6f/TracyConfig.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/Tracy" TYPE FILE FILES "W:/Priscilla/external_libs/tracy/CMakeFiles/Export/7430802ac276f58e70c46cf34d169c6f/TracyConfig-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/Tracy" TYPE FILE FILES "W:/Priscilla/external_libs/tracy/CMakeFiles/Export/7430802ac276f58e70c46cf34d169c6f/TracyConfig-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/Tracy" TYPE FILE FILES "W:/Priscilla/external_libs/tracy/CMakeFiles/Export/7430802ac276f58e70c46cf34d169c6f/TracyConfig-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/Tracy" TYPE FILE FILES "W:/Priscilla/external_libs/tracy/CMakeFiles/Export/7430802ac276f58e70c46cf34d169c6f/TracyConfig-release.cmake")
  endif()
endif()

