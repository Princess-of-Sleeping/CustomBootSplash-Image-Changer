cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()


project(CBS_Image_Changer)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_APP_NAME "CustomBootSplash Image Changer")
set(VITA_TITLEID  "CBSIC0000")

set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

add_executable(${PROJECT_NAME}
  src/main.c
)

target_link_libraries(${PROJECT_NAME}

  vita2d

  freetype
  png
  jpeg
  z
  m
  c

  SceAppMgr_stub
  SceAppUtil_stub
  SceDisplay_stub
  SceCtrl_stub
  SceCommonDialog_stub
  SceGxm_stub
  ScePgf_stub
  SceSysmodule_stub
  SceLibKernel_stub

)

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
