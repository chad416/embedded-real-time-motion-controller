# Discovery for optional fieldbus stacks.
#
# Most checks are non-fatal so the default simulation build remains portable.
# SOEM can be made mandatory for dependency-backed validation.

set(EMRTC_HAS_SOEM OFF)
set(EMRTC_SOEM_INCLUDE_DIRS "")
set(EMRTC_SOEM_LIBRARIES "")

if(EMRTC_ENABLE_SOEM)
    find_package(soem CONFIG QUIET PATH_SUFFIXES cmake)
    if(TARGET soem)
        set(EMRTC_HAS_SOEM ON)
        set(EMRTC_SOEM_LIBRARIES soem)
        message(STATUS "SOEM v2 CMake target discovered")
    else()
        find_path(EMRTC_SOEM_INCLUDE_DIR
            NAMES soem/soem.h)
        find_library(EMRTC_SOEM_LIBRARY
            NAMES soem libsoem)

        if(EMRTC_SOEM_INCLUDE_DIR AND EMRTC_SOEM_LIBRARY)
            set(EMRTC_HAS_SOEM ON)
            set(EMRTC_SOEM_INCLUDE_DIRS "${EMRTC_SOEM_INCLUDE_DIR}")
            set(EMRTC_SOEM_LIBRARIES "${EMRTC_SOEM_LIBRARY}")
            message(STATUS "SOEM v2 dependency discovered: ${EMRTC_SOEM_LIBRARY}")
        else()
            if(EMRTC_REQUIRE_SOEM)
                message(FATAL_ERROR "SOEM v2 was required but soem/soem.h and the soem library were not found")
            else()
                message(STATUS "SOEM flag enabled, but SOEM v2 headers/library were not found; using skeleton adapter mode")
            endif()
        endif()
    endif()

    if(EMRTC_HAS_SOEM AND WIN32)
        set(EMRTC_SOEM_PLATFORM_INCLUDE_DIR "" CACHE PATH
            "Windows pcap include directory required by SOEM v2")
        if(EMRTC_SOEM_PLATFORM_INCLUDE_DIR)
            list(APPEND EMRTC_SOEM_INCLUDE_DIRS "${EMRTC_SOEM_PLATFORM_INCLUDE_DIR}")
        elseif(EMRTC_REQUIRE_SOEM)
            message(FATAL_ERROR "SOEM v2 on Windows requires EMRTC_SOEM_PLATFORM_INCLUDE_DIR containing pcap.h")
        endif()
    endif()
endif()

set(EMRTC_HAS_ETHERLAB OFF)
set(EMRTC_ETHERLAB_INCLUDE_DIRS "")
set(EMRTC_ETHERLAB_LIBRARIES "")

if(EMRTC_ENABLE_ETHERLAB)
    find_path(EMRTC_ETHERLAB_INCLUDE_DIR
        NAMES ecrt.h
        PATH_SUFFIXES ethercat)
    find_library(EMRTC_ETHERLAB_LIBRARY
        NAMES ethercat)

    if(EMRTC_ETHERLAB_INCLUDE_DIR AND EMRTC_ETHERLAB_LIBRARY)
        set(EMRTC_HAS_ETHERLAB ON)
        set(EMRTC_ETHERLAB_INCLUDE_DIRS "${EMRTC_ETHERLAB_INCLUDE_DIR}")
        set(EMRTC_ETHERLAB_LIBRARIES "${EMRTC_ETHERLAB_LIBRARY}")
        message(STATUS "EtherLab dependency discovered: ${EMRTC_ETHERLAB_LIBRARY}")
    else()
        message(STATUS "EtherLab flag enabled, but headers/library were not found; using skeleton adapter mode")
    endif()
endif()

set(EMRTC_HAS_SOCKETCAN OFF)
set(EMRTC_SOCKETCAN_INCLUDE_DIRS "")
set(EMRTC_SOCKETCAN_LIBRARIES "")

if(EMRTC_ENABLE_SOCKETCAN)
    find_path(EMRTC_SOCKETCAN_INCLUDE_DIR
        NAMES linux/can.h linux/can/raw.h)

    if(EMRTC_SOCKETCAN_INCLUDE_DIR)
        set(EMRTC_HAS_SOCKETCAN ON)
        set(EMRTC_SOCKETCAN_INCLUDE_DIRS "${EMRTC_SOCKETCAN_INCLUDE_DIR}")
        message(STATUS "SocketCAN headers discovered: ${EMRTC_SOCKETCAN_INCLUDE_DIR}")
    else()
        message(STATUS "SocketCAN flag enabled, but Linux CAN headers were not found; using skeleton adapter mode")
    endif()
endif()

set(EMRTC_HAS_CANOPENNODE OFF)
set(EMRTC_CANOPENNODE_INCLUDE_DIRS "")
set(EMRTC_CANOPENNODE_LIBRARIES "")

if(EMRTC_ENABLE_CANOPENNODE)
    find_path(EMRTC_CANOPENNODE_INCLUDE_DIR
        NAMES CANopen.h
        PATH_SUFFIXES CANopenNode canopennode)
    find_library(EMRTC_CANOPENNODE_LIBRARY
        NAMES canopennode CANopenNode)

    if(EMRTC_CANOPENNODE_INCLUDE_DIR AND EMRTC_CANOPENNODE_LIBRARY)
        set(EMRTC_HAS_CANOPENNODE ON)
        set(EMRTC_CANOPENNODE_INCLUDE_DIRS "${EMRTC_CANOPENNODE_INCLUDE_DIR}")
        set(EMRTC_CANOPENNODE_LIBRARIES "${EMRTC_CANOPENNODE_LIBRARY}")
        message(STATUS "CANopenNode dependency discovered: ${EMRTC_CANOPENNODE_LIBRARY}")
    else()
        message(STATUS "CANopenNode flag enabled, but headers/library were not found; using skeleton adapter mode")
    endif()
endif()
