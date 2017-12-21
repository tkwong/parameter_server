include_directories(${PROJECT_SOURCE_DIR} ${HUSKY_EXTERNAL_INCLUDE})

add_library(app_library STATIC 
  $<TARGET_OBJECTS:server-objs> 
  $<TARGET_OBJECTS:comm-objs> 
  $<TARGET_OBJECTS:base-objs>
  $<TARGET_OBJECTS:io-objs>
  $<TARGET_OBJECTS:driver-objs>
  )

add_executable(DataLoaderExample load_data_example.cpp)
target_link_libraries(DataLoaderExample app_library)
target_link_libraries(DataLoaderExample ${HUSKY_EXTERNAL_LIB})
set_property(TARGET DataLoaderExample PROPERTY CXX_STANDARD 11)
add_dependencies(DataLoaderExample ${external_project_dependencies})

add_executable(LogisticRegression logistic_regression.cpp)
target_link_libraries(LogisticRegression csci5570)
target_link_libraries(LogisticRegression ${HUSKY_EXTERNAL_LIB})
set_property(TARGET LogisticRegression PROPERTY CXX_STANDARD 11)
target_compile_definitions(LogisticRegression PRIVATE BENCHMARK=1)
add_dependencies(LogisticRegression ${external_project_dependencies})

add_executable(SVM svm.cpp)
target_link_libraries(SVM csci5570)
target_link_libraries(SVM ${HUSKY_EXTERNAL_LIB})
set_property(TARGET SVM PROPERTY CXX_STANDARD 11)
target_compile_definitions(SVM PRIVATE BENCHMARK=1)
add_dependencies(SVM ${external_project_dependencies})

add_executable(MatrixFac mf.cpp)
target_link_libraries(MatrixFac csci5570)
target_link_libraries(MatrixFac ${HUSKY_EXTERNAL_LIB})
set_property(TARGET MatrixFac PROPERTY CXX_STANDARD 11)
target_compile_definitions(MatrixFac PRIVATE BENCHMARK=1)
add_dependencies(MatrixFac ${external_project_dependencies})
