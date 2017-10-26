### Eigen ###

# TODO: Check the version of Eigen
set(EIGEN_FIND_REQUIRED true)
find_path(EIGEN_INCLUDE_DIR NAMES eigen3)

if(EIGEN_INCLUDE_DIR)
    set(EIGEN_FOUND true)
endif(EIGEN_INCLUDE_DIR)
if(EIGEN_FOUND)
    set(DEP_FOUND true)
    set(EIGEN_INCLUDE_DIR ${EIGEN_INCLUDE_DIR}/eigen3)
    if(NOT EIGEN_FIND_QUIETLY)
        message (STATUS "Found Eigen:")
        message (STATUS "  (Headers)       ${EIGEN_INCLUDE_DIR}")
    endif(NOT EIGEN_FIND_QUIETLY)
else(EIGEN_FOUND)
    if(EIGEN_FIND_REQUIRED)
        message(STATUS "Eigen will be included as a third party")
        set(THIRDPARTY_DIR ${PROJECT_BINARY_DIR}/third_party)
        
        ExternalProject_Add(eigen
            PREFIX eigen
            URL https://bitbucket.org/eigen/eigen/get/e99c823108f4.tar.gz
            INSTALL_DIR ${THIRDPARTY_DIR}
            SOURCE_DIR ${THIRDPARTY_DIR}
            DOWNLOAD_DIR ${THIRDPARTY_DIR}
            CMAKE_CACHE_ARGS
                -DCMAKE_BUILD_TYPE:STRING=Release
                -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
                -DBUILD_TESTING:BOOL=OFF
                -DCMAKE_INSTALL_PREFIX:STRING=${PROJECT_BINARY_DIR}
        )
        set(EIGEN_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")

        message (STATUS "  (Headers should be)       ${EIGEN_INCLUDE_DIR}")

        set(EIGEN_FOUND true)
    else(EIGEN_FIND_REQUIRED) 
        message(FATAL_ERROR "Could NOT find Eigen")
    endif(EIGEN_FIND_REQUIRED)
endif(EIGEN_FOUND)

