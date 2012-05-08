#==============================================================================#
#                                                                              #
#  Copyright (c) 2012 MaidSafe.net limited                                     #
#                                                                              #
#  The following source code is property of MaidSafe.net limited and is not    #
#  meant for external use.  The use of this code is governed by the license    #
#  file licence.txt found in the root directory of this project and also on    #
#  www.maidsafe.net.                                                           #
#                                                                              #
#  You are not free to copy, amend or otherwise use this source code without   #
#  the explicit written permission of the board of directors of MaidSafe.net.  #
#                                                                              #
#==============================================================================#


set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

cmake_minimum_required(VERSION 2.8.7 FATAL_ERROR)
project(transport)

set(PROTO_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(PROTO_FILES "maidsafe/transport/transport.proto")
include(../../cmake_modules/maidsafe_standard_setup.cmake)

include_directories(${common_SOURCE_DIR}/include)


###################################################################################################
# Set up all files as GLOBs                                                                       #
###################################################################################################
####    CODE    ####
list(APPEND ALL_FILES_LIBUPNP_DIR
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/igd_desc_parse.c
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/minixml.c
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/minisoap.c
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/miniwget.c
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/upnpcommands.c
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/upnpreplyparse.c
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/upnperrors.c
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/miniupnpc.c
       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/libupnp/minissdpc.c)
source_group("libupnp Source Files" FILES ${ALL_FILES_LIBUPNP_DIR})

file(GLOB SOURCE_FILES_NATPMP_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/nat-pmp/*.cc")
file(GLOB HEADER_FILES_NATPMP_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/nat-pmp/*.h")
set(ALL_FILES_NATPMP_DIR ${SOURCE_FILES_NATPMP_DIR} ${HEADER_FILES_NATPMP_DIR})
source_group("NAT-PMP Source Files" FILES ${SOURCE_FILES_NATPMP_DIR})
source_group("NAT-PMP Header Files" FILES ${HEADER_FILES_NATPMP_DIR})

file(GLOB SOURCE_FILES_TRANSPORT_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/*.cc")
file(GLOB HEADER_FILES_TRANSPORT_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/*.h")
set(ALL_FILES_TRANSPORT_DIR ${SOURCE_FILES_TRANSPORT_DIR} ${HEADER_FILES_TRANSPORT_DIR})
set(STYLE_TRANSPORT ${ALL_FILES_TRANSPORT_DIR})
list(REMOVE_ITEM STYLE_TRANSPORT "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/transport.pb.cc")
list(REMOVE_ITEM STYLE_TRANSPORT "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/transport.pb.h")
source_group("Transport Source Files" FILES ${SOURCE_FILES_TRANSPORT_DIR})
source_group("Transport Header Files" FILES ${HEADER_FILES_TRANSPORT_DIR})

file(GLOB SOURCE_FILES_TCP "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/tcp_*.cc")
file(GLOB HEADER_FILES_TCP "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/tcp_*.h")
source_group("TCP Source Files" FILES ${SOURCE_FILES_TCP})
source_group("TCP Header Files" FILES ${HEADER_FILES_TCP})

file(GLOB SOURCE_FILES_UDP "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/udp_*.cc")
file(GLOB HEADER_FILES_UDP "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/udp_*.h")
source_group("UDP Source Files" FILES ${SOURCE_FILES_UDP})
source_group("UDP Header Files" FILES ${HEADER_FILES_UDP})

file(GLOB SOURCE_FILES_RUDP "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/rudp_*.cc")
file(GLOB HEADER_FILES_RUDP "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/rudp_*.h")
source_group("RUDP Source Files" FILES ${SOURCE_FILES_RUDP})
source_group("RUDP Header Files" FILES ${HEADER_FILES_RUDP})

file(GLOB SOURCE_FILES_NAT_DETECTION "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/nat_detection*.cc")
file(GLOB HEADER_FILES_NAT_DETECTION "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/nat_detection*.h")
source_group("NAT Detection Source Files" FILES ${SOURCE_FILES_NAT_DETECTION})
source_group("NAT Detection Header Files" FILES ${HEADER_FILES_NAT_DETECTION})

file(GLOB SOURCE_FILES_UPNP_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/upnp/*.cc")
file(GLOB HEADER_FILES_UPNP_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/upnp/*.h")
list(REMOVE_ITEM SOURCE_FILES_UPNP_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/upnp/upnp_client_impl.cc")
list(REMOVE_ITEM HEADER_FILES_UPNP_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/upnp/upnp_client_impl.h")
set(ALL_FILES_UPNP_DIR ${SOURCE_FILES_UPNP_DIR} ${HEADER_FILES_UPNP_DIR})
source_group("UPnP Source Files" FILES ${SOURCE_FILES_UPNP_DIR})
source_group("UPnP Header Files" FILES ${HEADER_FILES_UPNP_DIR})

set(ALL_FILES ${ALL_FILES_TRANSPORT_DIR} ${ALL_FILES_LIBUPNP_DIR} ${ALL_FILES_UPNP_DIR} ${ALL_FILES_NATPMP_DIR})

####    TESTS    ####
file(GLOB SOURCE_FILES_TEST_TRANSPORT_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/tests/*.cc")
file(GLOB HEADER_FILES_TEST_TRANSPORT_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/tests/*.h")
set(ALL_FILES_TEST_TRANSPORT_DIR ${SOURCE_FILES_TEST_TRANSPORT_DIR} ${HEADER_FILES_TEST_TRANSPORT_DIR})
source_group("Test Source Files" FILES ${SOURCE_FILES_TEST_TRANSPORT_DIR})
source_group("Test Header Files" FILES ${HEADER_FILES_TEST_TRANSPORT_DIR})

# file(GLOB ALL_FILES_TEST_NATPMP_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/nat-pmp/tests*.cc"
#                                     "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/nat-pmp/tests/*.h")
# file(GLOB ALL_FILES_TEST_UPNP_DIR "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/upnp/tests/*.cc"
#                                   "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/upnp/tests/*.h")


###################################################################################################
# Define MaidSafe libraries and executables                                                       #
###################################################################################################
ms_add_static_library(transport ${ALL_FILES})
ms_add_executable(TESTtransport "Tests/Transport" ${ALL_FILES_TEST_TRANSPORT_DIR})
# ms_add_executable(TESTnatpmp Tests ${ALL_FILES_TEST_MAIN} ${ALL_FILES_TEST_NATPMP_DIR})
# ms_add_executable(TESTupnp Tests ${ALL_FILES_TEST_MAIN} ${ALL_FILES_TEST_UPNP_DIR})

target_link_libraries(maidsafe_transport maidsafe_common)
target_link_libraries(TESTtransport maidsafe_transport)
# target_link_libraries(TESTnatpmp maidsafe_transport_static gtest_static)
# target_link_libraries(TESTupnp maidsafe_transport_static gtest_static)

rename_outdated_built_exes()


###################################################################################################
# Set compiler and linker flags                                                                   #
###################################################################################################
include(maidsafe_standard_flags)
ms_disable_warnings(${PROJECT_SOURCE_DIR}/src/maidsafe/transport/transport.pb.cc ${ALL_FILES_LIBUPNP_DIR})


###################################################################################################
# Tests                                                                                           #
###################################################################################################
add_style_test(${STYLE_TRANSPORT} ${ALL_FILES_TEST_TRANSPORT_DIR})
add_gtests(TESTtransport)
# add_gtests(TESTnatpmp)
# add_gtests(TESTupnp)
test_summary_output()

# Remove files from coverage stats
add_coverage_exclude(libupnp/)
add_coverage_exclude(nat-pmp/)
add_coverage_exclude(upnp/)
add_coverage_exclude(transport.pb.cc)
