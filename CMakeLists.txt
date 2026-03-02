cmake_minimum_required(VERSION 3.29)

project(Tutorial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(ENABLE_VULKAN_RAII "Enable Vulkan-Hpp RAII wrappers" OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Stb REQUIRED)

set(STB_INCLUDEDIR ${Stb_INCLUDE_DIR})

find_program(GLSLANG_VALIDATOR NAMES glslangValidator HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin" REQUIRED)
add_executable(glslang::validator IMPORTED)
set_property(TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")

find_program(SLANGC_EXECUTABLE NAMES slangc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin" REQUIRED)
add_executable(slangc::executable IMPORTED)
set_property(TARGET slangc::executable PROPERTY IMPORTED_LOCATION "${SLANGC_EXECUTABLE}")

add_executable(myapp src/main.cc)

target_compile_definitions(myapp PRIVATE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

if(ENABLE_VULKAN_RAII)
    target_compile_definitions(myapp PRIVATE USE_VULKAN_RAII=1)
endif()

target_link_libraries(myapp PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    KTX::ktx
    tinyobjloader::tinyobjloader
)

target_include_directories(myapp PRIVATE ${STB_INCLUDEDIR})
