include_directories(${PROJECT_SOURCE_DIR} ${HUSKY_EXTERNAL_INCLUDE})

add_library(csci5570 STATIC 
  $<TARGET_OBJECTS:server-objs> 
  $<TARGET_OBJECTS:comm-objs> 
  $<TARGET_OBJECTS:base-objs>
  $<TARGET_OBJECTS:io-objs>
  $<TARGET_OBJECTS:driver-objs>
  )

# Unit Test
file(GLOB_RECURSE UNITTEST_FILES ${PROJECT_SOURCE_DIR}/*_test.cpp) 
include_directories(${GTEST_INCLUDE})
add_executable(HuskyUnitTest ${UNITTEST_FILES} test_main.cpp)
add_dependencies(HuskyUnitTest gtest)
target_link_libraries(HuskyUnitTest csci5570)
target_link_libraries(HuskyUnitTest ${HUSKY_EXTERNAL_LIB})
target_link_libraries(HuskyUnitTest ${GTEST_LIBRARIES})
target_link_libraries(HuskyUnitTest ${GMOCK_LIBRARIES})
set_property(TARGET HuskyUnitTest PROPERTY CXX_STANDARD 11)
add_dependencies(HuskyUnitTest ${external_project_dependencies})

if(LIBHDFS3_FOUND)
    add_executable(TestRead test_hdfs_read.cpp)
    target_link_libraries(TestRead csci5570)
    target_link_libraries(TestRead ${HUSKY_EXTERNAL_LIB})
    set_property(TARGET TestRead PROPERTY CXX_STANDARD 11)
    add_dependencies(TestRead ${external_project_dependencies})
endif(LIBHDFS3_FOUND)

add_executable(TestMnist test_mnist.cpp)
# target_link_libraries(TestMnist csci5570)
# target_link_libraries(TestMnist ${HUSKY_EXTERNAL_LIB})
# set_property(TARGET TestMnist PROPERTY CXX_STANDARD 11)
target_compile_definitions(TestMnist PRIVATE MNIST_DATA_LOCATION="${CMAKE_CURRENT_LIST_DIR}/data/")
add_dependencies(TestMnist ${external_project_dependencies})

add_executable(LogisticRegression logistic_regression.cpp)
target_link_libraries(LogisticRegression csci5570)
target_link_libraries(LogisticRegression ${HUSKY_EXTERNAL_LIB})
set_property(TARGET LogisticRegression PROPERTY CXX_STANDARD 11)
add_dependencies(LogisticRegression ${external_project_dependencies})

add_executable(ReadData read_data.cpp)
target_link_libraries(ReadData csci5570)
target_link_libraries(ReadData ${HUSKY_EXTERNAL_LIB})
set_property(TARGET ReadData PROPERTY CXX_STANDARD 11)
add_dependencies(ReadData ${external_project_dependencies})
