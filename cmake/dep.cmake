FIND_PATH(GSASL_INCLUDE_DIR gsasl.h
    PATHS
    ${PC_GSASL_INCLUDEDIR}
    ${PC_GSASL_INCLUDE_DIRS}
  )

FIND_LIBRARY(GSASL_LIBRARIES NAMES gsasl
    PATHS
    ${PC_GSASL_LIBDIR}
    ${PC_GSASL_LIBRARY_DIRS}
  )
if(NOT GSASL_FIND_QUIETLY)
  message (STATUS "Found GSASL:")
  message (STATUS "  (Headers)       ${GSASL_INCLUDE_DIR}")
  message (STATUS "  (Library)       ${GSASL_LIBRARIES}")
endif( NOT GSASL_FIND_QUIETLY)

FIND_PATH(KRB5_INCLUDE_DIR krb5.h)

FIND_LIBRARY(KRB5_LIBRARIES NAMES krb5)
if(NOT KRB5_FIND_QUIETLY)
  message (STATUS "Found KRB5:")
  message (STATUS "  (Headers)       ${KRB5_INCLUDE_DIR}")
  message (STATUS "  (Library)       ${KRB5_LIBRARIES}")
endif( NOT KRB5_FIND_QUIETLY)
if(APPLE)
  list(APPEND HUSKY_EXTERNAL_LIB ${KRB5_LIBRARIES})
endif(APPLE)

FIND_LIBRARY(LIBXML2_LIBRARIES NAMES libxml2)
if(NOT LIBXML2_FIND_QUIETLY)
  message (STATUS "Found LIBXML2:")
  message (STATUS "  (Headers)       ${LIBXML2_INCLUDE_DIR}")
  message (STATUS "  (Library)       ${LIBXML2_LIBRARIES}")
endif( NOT LIBXML2_FIND_QUIETLY)

INCLUDE(FindProtobuf)
FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(LibXml2 REQUIRED)

list(APPEND HUSKY_EXTERNAL_LIB ${LIBHDFS3_LIBRARY} ${PROTOBUF_LIBRARY} ${GSASL_LIBRARIES} ${LIBXML2_LIBRARIES} ${Boost_LIBRARIES})

# Seems in MacOS, it need a KRB library included.
if(APPLE)
  list(APPEND HUSKY_EXTERNAL_LIB ${KRB5_LIBRARIES})
endif(APPLE)

# libhdfs3
if(LIBHDFS3_FOUND)
    list(APPEND HUSKY_EXTERNAL_INCLUDE ${LIBHDFS3_INCLUDE_DIR})
    list(APPEND HUSKY_EXTERNAL_LIB ${LIBHDFS3_LIBRARY})
    list(APPEND HUSKY_EXTERNAL_DEFINITION ${LIBHDFS3_DEFINITION})
endif(LIBHDFS3_FOUND)

### Eigen ###

# TODO: Check the version of Eigen
# set(EIGEN_FIND_REQUIRED true)
# find_path(EIGEN_INCLUDE_DIR NAMES eigen3)
#
# if(EIGEN_INCLUDE_DIR)
#     set(EIGEN_FOUND true)
# endif(EIGEN_INCLUDE_DIR)
# if(EIGEN_FOUND)
#     set(DEP_FOUND true)
#     set(EIGEN_INCLUDE_DIR ${EIGEN_INCLUDE_DIR}/eigen3)
#     if(NOT EIGEN_FIND_QUIETLY)
#         message (STATUS "Found Eigen:")
#         message (STATUS "  (Headers)       ${EIGEN_INCLUDE_DIR}")
#     endif(NOT EIGEN_FIND_QUIETLY)
# else(EIGEN_FOUND)
#     if(EIGEN_FIND_REQUIRED)
#         message(STATUS "Eigen will be included as a third party")
#         set(THIRDPARTY_DIR ${PROJECT_BINARY_DIR}/third_party)
#
#         ExternalProject_Add(eigen
#             PREFIX eigen
#             URL https://bitbucket.org/eigen/eigen/get/e99c823108f4.tar.gz
#             INSTALL_DIR ${THIRDPARTY_DIR}
#             SOURCE_DIR ${THIRDPARTY_DIR}
#             DOWNLOAD_DIR ${THIRDPARTY_DIR}
#             CMAKE_CACHE_ARGS
#                 -DCMAKE_BUILD_TYPE:STRING=Release
#                 -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
#                 -DBUILD_TESTING:BOOL=OFF
#                 -DCMAKE_INSTALL_PREFIX:STRING=${PROJECT_BINARY_DIR}
#         )
#         set(EIGEN_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")
#
#         message (STATUS "  (Headers should be)       ${EIGEN_INCLUDE_DIR}")
#
#         set(EIGEN_FOUND true)
#     else(EIGEN_FIND_REQUIRED)
#         message(FATAL_ERROR "Could NOT find Eigen")
#     endif(EIGEN_FIND_REQUIRED)
# endif(EIGEN_FOUND)

