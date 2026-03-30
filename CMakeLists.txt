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

function(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    if(NOT SHADER_SOURCES)
        message(FATAL_ERROR "add_slang_shader_target(${TARGET}) requires at least one source file.")
    endif()

    set(SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(SLANG_OUTPUT "${SHADERS_DIR}/slang.spv")

    add_custom_command(
        OUTPUT "${SLANG_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADERS_DIR}"
        COMMAND ${SLANGC_EXECUTABLE}
            ${SHADER_SOURCES}
            -target spirv
            -profile spirv_1_4
            -emit-spirv-directly
            -fvk-use-entrypoint-name
            -entry vertMain
            -entry fragMain
            -o "${SLANG_OUTPUT}"
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Compiling Slang shaders"
        VERBATIM
    )

    add_custom_target(${TARGET} DEPENDS "${SLANG_OUTPUT}")
endfunction()

add_executable(myapp src/main.cc)

set(SHADER_SLANG_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/shaders/shader.slang")
add_slang_shader_target(myapp_slang_shaders SOURCES "${SHADER_SLANG_SOURCES}")
add_dependencies(myapp myapp_slang_shaders)

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
