# CMakeLists.txt for polyView project
################################################
### *** NOTE SPECIFIC INSTALL LOCATION ***   ###
### change to suite your desire, environment ###
################################################
# 20:49 19/06/2016 - Help with Qt4 to Qt5 transition here -
# https://www.kdab.com/using-cmake-with-qt-5/
# original CMakeLists.txt, generated from [...\gui\polyview.pro]

cmake_minimum_required( VERSION 2.6 )

if(COMMAND cmake_policy)
   if(POLICY CMP0020)
       cmake_policy(SET CMP0020 NEW)
   endif()
endif()

project( polyView )

option( ADD_SAVE_IMAGE "set ON the add a save to an image file."               ON )
option( ADD_PREF_INI   "set ON the add a read/write preferecnes to INI file."  ON )
option( QT5_BUILD      "Set ON to find and link with Qt5."                     ON )

# ### NOTE: *** Adjust version.txt when required ***
# read 'version' file into a variable (stripping any newlines or spaces)
# 20150609: Revert to supplying BOTH version and date, as we had back in Jan 2015
# NOTE: Both version and date MUST be DOT separated, in two lines.
file(READ version.txt versionFile)
if (NOT versionFile)
    message(FATAL_ERROR "Unable to determine polyView version. version.txt file is missing.")
endif()
string(STRIP "${versionFile}" VERSION_TEXT)
string(REGEX REPLACE "(.*)[\r\n|\n](.*)" "\\1" PV_VERSION ${VERSION_TEXT})
string(REGEX REPLACE "(.*)[\r\n|\n](.*)" "\\2" PV_DATE ${VERSION_TEXT})
# establish version number
if (PV_VERSION)
    string(REPLACE "." ";" VERSION_LIST ${PV_VERSION})
    list(GET VERSION_LIST 0 PV_MAJOR_VERSION)
    list(GET VERSION_LIST 1 PV_MINOR_VERSION)
    list(GET VERSION_LIST 2 PV_POINT_VERSION)
else ()
    message(FATAL_ERROR "*** FAILED to get a VERSION from version.txt!")
endif ()    
# establish version date
if (PV_DATE)
    string(REPLACE "." ";" DATE_LIST ${PV_DATE})
    list(GET DATE_LIST 0 PV_YEAR)
    list(GET DATE_LIST 1 PV_MONTH)
    list(GET DATE_LIST 2 PV_DAY)
else ()
    message(FATAL_ERROR "*** FAILED to get a DATE from version.txt!")
endif ()

if(CMAKE_COMPILER_IS_GNUCXX)
    set( WARNING_FLAGS -Wall )
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
   set( WARNING_FLAGS "-Wall -Wno-overloaded-virtual" )
endif() 

if(WIN32 AND MSVC)
    # turn off various warnings - none needed in this compile
    set(WARNING_FLAGS "${WARNING_FLAGS} /wd4996")
    # foreach(warning 4244 4251 4267 4275 4290 4786 4305)
    # C4267: 'argument' : conversion from 'size_t' to 'int', possible loss of data
    # C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)
    foreach(warning 4267 4800)
        set(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
    endforeach()
    # 20161010 - remove -DNOMINMAX
    set( MSVC_FLAGS "-D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS" )
    # if (${MSVC_VERSION} EQUAL 1600)
    #    set( MSVC_LD_FLAGS "/FORCE:MULTIPLE" )
    # endif (${MSVC_VERSION} EQUAL 1600)
    #set( NOMINMAX 1 )
    #if(MINGW)
        # need to specifically handle rc files, like
        # resource compilation for mingw
        #ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_rc.o
        #                   COMMAND windres.exe -I${CMAKE_CURRENT_SOURCE_DIR}
        #                                       -i${CMAKE_CURRENT_SOURCE_DIR}/test.rc
        #                                       -o ${CMAKE_CURRENT_BINARY_DIR}/test_rc.o)
        #SET(test_EXE_SRCS ${test_EXE_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/test_rc.o)
    #else(MINGW)
        #SET(test_EXE_SRCS ${test_EXE_SRCS} test.rc)
    #endif(MINGW)
    # to distinguish between debug and release lib
    set( CMAKE_DEBUG_POSTFIX d )
else()
    # any item for NOT Windows/MSVC, like suppress some gcc warnings perhaps...
endif()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}" )

#############################################################################################
add_definitions( -DPV2D_VERSION="${PV_MAJOR_VERSION}.${PV_MINOR_VERSION}.${PV_POINT_VERSION}" )
add_definitions( -DPV2D_DATE="${PV_YEAR}/${PV_MONTH}/${PV_DAY}" )
#############################################################################################

if (QT5_BUILD)
################################################################
### find_package ( Qt5 5.1 COMPONENTS Widgets REQUIRED )
find_package(Qt5Widgets)

message( STATUS "*** Qt5 found inc ${Qt5Widgets_INCLUDE_DIRS}, libs ${Qt5Widgets_LIBRARIES} ")
### include_directories( ${Qt5Widgets_INCLUDE_DIRS} )
set( QT_INCLUDE_DIR ${Qt5Widgets_INCLUDE_DIRS} )
### list(APPEND add_LIBS ${Qt5Widgets_LIBRARIES} )
set( QT_LIBRARIES ${Qt5Widgets_LIBRARIES} )

# We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
if (Qt5Widgets_DEFINITIONS)
  add_definitions(${Qt5Widgets_DEFINITIONS})
  message( STATUS "*** Added Qt5 DEFS ${Qt5Widgets_DEFINITIONS}" )
endif ()

###############################################################
else ()
###############################################################
set( QT_USE_QTNETWORK TRUE )
set( QT_USE_QTXML TRUE )
set( QT_USE_QTXMLPATTERNS TRUE )
set( QT_USE_QTWEBKIT TRUE )
set( QT_USE_QT3SUPPORT TRUE )
find_package ( Qt4 COMPONENTS QtCore QtGui QtNetwork QtWebkit QtXml QtXmlPatterns REQUIRED )
include ( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT3_SUPPORT=1 )
# debug messages
message(STATUS "*** include ( ${QT_USE_FILE} )")
message(STATUS "*** add_definitions( ${QT_DEFINITIONS} )")
##############################################################
endif ()

if (ADD_SAVE_IMAGE)
    add_definitions( -DADD_IMAGE_SAVE )
    message(STATUS "*** Compiling in 'Save as image'" )
else ()
    message(STATUS "*** 'Save as image' DISABLED" )
endif ()
if (ADD_PREF_INI)
    add_definitions( -DADD_PREF_INI )
    message(STATUS "*** Compiling in 'preference INI'" )
else ()
    message(STATUS "*** 'Preference INI' DISABLED" )
endif ()

# QT = qt3support

set( polyView_SRCS
    gui/mainProg.cpp 
    gui/polyView.cpp 
    gui/appWindow.cpp 
    gui/chooseFilesDlg.cpp 
    gui/utils.cpp
    gui/documentation.cpp 
    geom/dPoly.cpp 
    geom/cutPoly.cpp 
    geom/geomUtils.cpp
    geom/polyUtils.cpp 
    geom/edgeUtils.cpp 
    geom/dTree.cpp 
    geom/kdTree.cpp
    )

set( polyView_HDRS 
    gui/polyView.h 
    gui/appWindow.h 
    gui/chooseFilesDlg.h
    )

set( polyView_OTHERS 
    gui/utils.h 
    geom/dPoly.h 
    geom/cutPoly.h 
    geom/geomUtils.h 
    geom/polyUtils.h
    geom/edgeUtils.h 
    geom/dTree.h 
    geom/kdTree.h 
    geom/baseUtils.h
    )

set( polyView_RCS 
    gui/polyview.qrc
    )

include_directories( gui geom
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QT_INCLUDE_DIR}
    )
# Added for DEBUG only
# get list # -- *** inc_dirs = 
get_property(inc_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
message(STATUS "*** inc_dirs = ${inc_dirs}")
if (MSVC)
    if (QT5_BUILD)
        message(STATUS "*** Doing a MSVC ${MSVC_VERSION} Qt5 build")
    else ()
        # WHY DO I HAVE TO ADD THIS??????
        include_directories( C:/Qt/4.8.6/include/Qt )
        include_directories( C:/Qt/4.8.6/include/Qt3Support )
    endif ()
endif ()

set(name polyView2D)
if (QT5_BUILD)
QT5_WRAP_CPP( polyView_HDRS_MOC ${polyView_HDRS} )
QT5_ADD_RESOURCES( polyView_RESOURCES_RCC ${polyView_RCS} )
else ()
QT4_WRAP_CPP( polyView_HDRS_MOC ${polyView_HDRS} )
QT4_ADD_RESOURCES( polyView_RESOURCES_RCC ${polyView_RCS} )
endif ()

add_executable( ${name} ${polyView_SRCS} ${polyView_HDRS} ${polyView_HDRS_MOC} ${polyView_RESOURCES_RCC} ${polyView_OTHERS} )
target_link_libraries( ${name} ${QT_LIBRARIES} )
# deal with install 
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
    ################################################
    ### *** NOTE SPECIFIC INSTALL LOCATION ***   ###
    ### change to suite your desire, environment ###
    ################################################
    install(TARGETS ${name} DESTINATION D:/UTILS/TEMP )
else ()
    # 'standard' install - /usr/local/bin
    install(TARGETS ${name} DESTINATION bin )
endif ()
# this does NOT seem to work
add_custom_command(
    TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Qt5::Core>
        $<TARGET_FILE:Qt5::Gui>
        $<TARGET_FILE:Qt5::Widgets>
        $<TARGET_FILE_DIR:${name}>
)
# eof - original generated by Qt2cmake.pl, on 2014/06/20 15:14:28
#
