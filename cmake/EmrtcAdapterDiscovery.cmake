# Optional discovery for future fieldbus stacks.
#
# These checks are intentionally non-fatal. The project must continue to build in
# skeleton mode when vendor/OS-specific stacks are not installed.

set(EMRTC_HAS_SOEM OFF)
set(EMRTC_SOEM_INCLUDE_DIRS "")
set(EMRTC_SOEM_LIBRARIES "")

if(EMRTC_ENABLE_SOEM)
    find_path(EMRTC_SOEM_INCLUDE_DIR
        NAMES ethercat.h
        PATH_SUFFIXES soem SOEM)
    find_library(EMRTC_SOEM_LIBRARY
        NAMES soem libsoem)

    if(EMRTC_SOEM_INCLUDE_DIR AND EMRTC_SOEM_LIBRARY)
        set(EMRTC_HAS_SOEM ON)
        set(EMRTC_SOEM_INCLUDE_DIRS "${EMRTC_SOEM_INCLUDE_DIR}")
        set(EMRTC_SOEM_LIBRARIES "${EMRTC_SOEM_LIBRARY}")
        message(STATUS "SOEM dependency discovered: ${EMRTC_SOEM_LIBRARY}")
    else()
        message(STATUS "SOEM flag enabled, but headers/library were not found; using skeleton adapter mode")
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
