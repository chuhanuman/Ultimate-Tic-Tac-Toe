cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(UTTT)

find_package(Torch REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
set(CMAKE_BUILD_TYPE Release)

add_executable(trainer trainer.cpp UTTTGameState.cpp)
target_link_libraries(trainer "${TORCH_LIBRARIES}")
set_property(TARGET trainer PROPERTY CXX_STANDARD 14)

add_executable(UTTT main.cpp UTTTGameWindow.cpp UTTTGameState.cpp)
target_link_libraries(UTTT "${TORCH_LIBRARIES}")
set_property(TARGET UTTT PROPERTY CXX_STANDARD 14)

file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
add_custom_command(TARGET trainer POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TORCH_DLLS} $<TARGET_FILE_DIR:trainer>
				COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:trainer>/examples"
				COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:trainer>/models"
				)
add_custom_command(TARGET UTTT POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TORCH_DLLS} $<TARGET_FILE_DIR:UTTT>
				COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:UTTT>/models"
				)

#Instructions
#cd build (Go to build folder)
#cmake .. (Only run the first time or after changing CMakeLists.txt)
#cmake --build . --config Release (Run every time)