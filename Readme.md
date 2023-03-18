install Cmake  
download Cmake.app and add envirenment path

install Ninja 
brew install ninja


git clone https://github.com/apple/llvm-project.git

git checkout stable/20211026

add files in UnusedCode to clang-tools-extra/call-graph

append add_subdirectory(call-graph) to file clang-tools-extra/CmakeLists.txt




