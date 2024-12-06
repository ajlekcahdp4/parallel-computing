cmake_minimum_required(VERSION 3.23)
project(openmp-intro)

option(INSOURCEBUILD OFF) # Prevent in-source style builds
if((${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR}) AND NOT
                                                              ${INSOURCEBUILD})
  message(
    FATAL_ERROR
      "In-source building disabled. Provide -DINSOURCEBUILD option if you are sure about it."
  )
endif()

option(SANITIZE OFF)
if(${SANITIZE})
  add_compile_options(-fsanitize=undefined,address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=undefined,address -fno-omit-frame-pointer)
endif()

option(THREAD_SANITIZE OFF)
if(${THREAD_SANITIZE})
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
  add_link_options(-fsanitize=thread -fno-omit-frame-pointer)
endif()

if(THREAD_SANITIZE AND SANITIZE)
  message(FATAL_ERROR "Thread and address sanitizer can't be used together")
endif()

include(../cmake/functions.cmake)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(OpenMP REQUIRED COMPONENTS CXX)
find_package(fmt REQUIRED)

function(add_exec TARGET_NAME SOURCE)
  add_executable(${TARGET_NAME} ${SOURCE})
  target_enable_linter(${TARGET_NAME})
  target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)
  enable_warnings(${TARGET_NAME})
	target_link_libraries(${TARGET_NAME} OpenMP::OpenMP_CXX Boost::program_options fmt::fmt)
endfunction()

add_exec(hello hello.cpp)
add_exec(reduce reduce.cpp)
add_exec(shared shared.cpp)
