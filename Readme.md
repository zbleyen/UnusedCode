#build call-graph tool

download Cmake.app and add envirenment path
 
brew install ninja


git clone https://github.com/apple/llvm-project.git

git checkout stable/20211026

add files in UnusedCode to clang-tools-extra/call-graph

append add_subdirectory(call-graph) to file clang-tools-extra/CmakeLists.txt

mkdir build
cd build
cmake -G Ninja ../llvm/llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_BUILD_TESTS=ON -DMAKE_INSTALL_PREFIX=(path to build)

不设置DMAKE_INSTALL_PREFIX会在/usr/local下再安装一遍，占用磁盘40G左右

sudo ninja install


#generate compile commands

sudo gem install xcpretty

xcodebuild clean -workspace XXX.xcworkspace -scheme XXX -sdk iphoneos
xcodebuild -workspace XXX.xcworkspace -scheme XXX -sdk iphoneos  | xcpretty -r json-compilation-database --output compile_commands.json



