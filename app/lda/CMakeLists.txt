 include_directories(${PROJECT_SOURCE_DIR} ${HUSKY_EXTERNAL_INCLUDE})

 file(GLOB lda-src-files
     lda_doc.hpp
     lda_stat.cpp
     doc_sampler.cpp
     test1.cpp
     )
 add_library(lda-objs STATIC ${lda-src-files})

 # LDAExample
 if(LIBHDFS3_FOUND)
   add_executable(LDAExample lda_main.cpp)
   target_link_libraries(LDAExample csci5570)
   target_link_libraries(LDAExample ${HUSKY_EXTERNAL_LIB})
   target_link_libraries(LDAExample lda-objs)
   set_property(TARGET LDAExample PROPERTY CXX_STANDARD 11)
   add_dependencies(LDAExample ${external_project_dependencies})
 endif(LIBHDFS3_FOUND)