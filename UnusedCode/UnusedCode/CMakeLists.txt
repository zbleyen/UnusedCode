set(LLVM_LINK_COMPONENTS support)

add_clang_executable(call-graph
  UnusedCodePlugin.cpp 
  UnusedCodePlugin.hpp 
  UnusedCodeUtil.cpp
  UnusedCodeUtil.hpp
  json.hpp
  )
target_link_libraries(call-graph
  PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )
