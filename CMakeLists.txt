#==============================================================================#
#                                                                              #
#  Copyright (c) 2011 MaidSafe.net limited                                     #
#  All rights reserved.                                                        #
#                                                                              #
#  Redistribution and use in source and binary forms, with or without          #
#  modification, are permitted provided that the following conditions are met: #
#                                                                              #
#      * Redistributions of source code must retain the above copyright        #
#        notice, this list of conditions and the following disclaimer.         #
#      * Redistributions in binary form must reproduce the above copyright     #
#        notice, this list of conditions and the following disclaimer in the   #
#        documentation and/or other materials provided with the distribution.  #
#      * Neither the name of the maidsafe.net limited nor the names of its     #
#        contributors may be used to endorse or promote products derived from  #
#        this software without specific prior written permission.              #
#                                                                              #
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" #
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   #
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  #
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  #
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR         #
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF        #
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS    #
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN     #
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)     #
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE  #
#  POSSIBILITY OF SUCH DAMAGE.                                                 #
#                                                                              #
#==============================================================================#
#                                                                              #
#  Written by maidsafe.net team                                                #
#                                                                              #
#==============================================================================#


project(transport)

cmake_minimum_required(VERSION 2.8.4 FATAL_ERROR)
message("================================================================================\n")
include(../../cmake_modules/maidsafe_standard_setup.cmake)
include_directories(${common_SOURCE_DIR}/include)
get_filename_component(CMAKE_BUILD_TYPE_DIR ${CMAKE_BINARY_DIR} NAME)

###################################################################################################
# Standard setup                                                                                  #
###################################################################################################
set(PROTO_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(PROTO_FILES "maidsafe/transport/transport.proto")
include("${CMAKE_SOURCE_DIR}/cmake_modules/maidsafe_standard_setup.cmake")

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
ms_add_static_library("transport" ${ALL_FILES})
ms_add_executable(TESTtransport Tests ${ALL_FILES_TEST_TRANSPORT_DIR})
# ms_add_executable(TESTnatpmp Tests ${ALL_FILES_TEST_MAIN} ${ALL_FILES_TEST_NATPMP_DIR})
# ms_add_executable(TESTupnp Tests ${ALL_FILES_TEST_MAIN} ${ALL_FILES_TEST_UPNP_DIR})

target_link_libraries(transport  maidsafe_common)
target_link_libraries(TESTtransport transport)
# target_link_libraries(TESTnatpmp maidsafe_transport_static gtest_static)
# target_link_libraries(TESTupnp maidsafe_transport_static gtest_static)


###################################################################################################
# Set compiler and linker flags                                                                   #
###################################################################################################
include(maidsafe_standard_flags)

if(MSVC)
  # Warnings switched off for protocol buffer generated files and libupnp files
  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/maidsafe/transport/transport.pb.cc
                              ${ALL_FILES_LIBUPNP_DIR}
                                PROPERTIES COMPILE_FLAGS "/W0")
elseif(UNIX)
  file(GLOB PROTO_SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/maidsafe/transport/*.pb.cc")
#  set_source_files_properties(${PROJECT_SOURCE_DIR}/src/maidsafe/transport/tcp_connection.cc
#                                PROPERTIES COMPILE_FLAGS "-Wno-effc++")
  set_source_files_properties(${PROTO_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-w")
endif()




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


# ###################################################################################################
# # Install details                                                                                 #
# ###################################################################################################
# final_message()
# rename_outdated_built_exes()
# 
# set(MAIDSAFE_TRANSPORT_INSTALL_FILES
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/transport.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/version.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/message_handler.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/contact.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/tcp_transport.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/udp_transport.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/rudp_transport.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/rudp_parameters.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/rudp_message_handler.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/nat_detection.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/nat_traversal.h
#       ${PROJECT_SOURCE_DIR}/src/maidsafe/transport/utils.h)  # TODO - remove utils.h once NAT detection is implemented and this is not #included by node_container.h
# 
# ms_install_libs(maidsafe_transport_static)
# ms_install_headers(transport ${MAIDSAFE_TRANSPORT_INSTALL_FILES})
# ms_install_export()
