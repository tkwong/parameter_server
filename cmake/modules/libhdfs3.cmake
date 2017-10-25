### LibHDFS3 ###
find_path(LIBHDFS3_INCLUDE_DIR NAMES hdfs/hdfs.h)
find_library(LIBHDFS3_LIBRARY NAMES hdfs3 libhdfs3)

if(LIBHDFS3_INCLUDE_DIR AND LIBHDFS3_LIBRARY)
    set(LIBHDFS3_FOUND true)
endif(LIBHDFS3_INCLUDE_DIR AND LIBHDFS3_LIBRARY)

if(WITHOUT_HDFS)
    unset(LIBHDFS3_FOUND)
    message(STATUS "Not using libhdfs3 due to WITHOUT_HDFS option")
else(WITHOUT_HDFS)
    if(LIBHDFS3_FOUND)
        set(LIBHDFS3_DEFINITION "-DWITH_HDFS")
        if(NOT LIBHDFS3_FIND_QUIETLY)
            message (STATUS "Found libhdfs3:")
            message (STATUS "  (Headers)       ${LIBHDFS3_INCLUDE_DIR}")
            message (STATUS "  (Library)       ${LIBHDFS3_LIBRARY}")
            message (STATUS "  (Definition)    ${LIBHDFS3_DEFINITION}")
        endif(NOT LIBHDFS3_FIND_QUIETLY)
    else(LIBHDFS3_FOUND)
        
        # message(FATAL_ERROR "Could NOT find HDFS3")
        set(THIRDPARTY_DIR ${PROJECT_BINARY_DIR}/third_party)        
        message(STATUS "HDFS3 will be included as a third party")

        ExternalProject_Add(libhdfs3
            URL https://github.com/Pivotal-Data-Attic/pivotalrd-libhdfs3/archive/v2.2.31.tar.gz
            URL_MD5 7629f18ba81b638f5f47599e07eee28c
            PREFIX ${THIRDPARTY_DIR}
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR})
        # ExternalProject_Get_Property(libhdfs3 INSTALL_DIR)

        # list(APPEND THRILL_DEFINITIONS "THRILL_HAVE_LIBHDFS3=1")
        # set(THRILL_INCLUDE_DIRS "${INSTALL_DIR}/include" ${THRILL_INCLUDE_DIRS})
        # add_library(hdfs3 UNKNOWN IMPORTED)
        # set_target_properties(hdfs3 PROPERTIES
        # IMPORTED_LOCATION "${INSTALL_DIR}/lib/libhdfs3.so")
        # add_dependencies(hdfs3 libhdfs3)
        set(LIBHDFS3_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")

        # list(APPEND external_project_dependencies hdfs3)
        if(WIN32)
            set(LIBHDFS3_LIBRARY "${PROJECT_BINARY_DIR}/lib/libhdfs3.a")
        else(WIN32)
            set(LIBHDFS3_LIBRARY "${PROJECT_BINARY_DIR}/lib/libhdfs3.a")
        endif(WIN32)

        message (STATUS "  (Headers should be)       ${LIBHDFS3_INCLUDE_DIR}")
        message (STATUS "  (Library should be)       ${LIBHDFS3_LIBRARY}")

        set(LIBHDFS3_FOUND true)
    
    endif(LIBHDFS3_FOUND)
endif(WITHOUT_HDFS)
