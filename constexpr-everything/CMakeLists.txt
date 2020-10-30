set(LLVM_LINK_COMPONENTS support)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp")

# Executable
add_clang_executable(constexpr-everything ${SOURCES})

target_link_libraries(constexpr-everything PRIVATE
  clangTooling clangBasic clangASTMatchers)

# clang-format

add_custom_target(format-constexpr-everything COMMAND "clang-format"
  "-style=Mozilla" "-i" "${SOURCES}" "${HEADERS}")
