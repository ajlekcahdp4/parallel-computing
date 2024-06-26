cmake_minimum_required(VERSION 3.23)
project(transport-equation)

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

find_package(Boost REQUIRED COMPONENTS mpi program_options)
find_package(MPI REQUIRED)
find_package(fmt REQUIRED)

add_library(solver transport_solver.cpp)
target_enable_linter(solver)
target_compile_features(solver PUBLIC cxx_std_23)
enable_warnings(solver)
target_link_libraries(solver fmt::fmt Boost::mpi MPI::MPI_CXX)

function(add_exec TARGET_NAME SOURCE)
  add_executable(${TARGET_NAME} ${SOURCE})
  target_enable_linter(${TARGET_NAME})
  target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)
  enable_warnings(${TARGET_NAME})
	target_link_libraries(${TARGET_NAME} solver MPI::MPI_CXX Boost::mpi Boost::program_options fmt::fmt)
endfunction()

add_exec(solution main.cpp)
