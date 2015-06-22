——————————————————
BUILD ON MAC
——————————————————

Pre-requisite:
- Xcode (latest version recommended)
- Qt 5 installed on your Mac (latest version recommended)
- Xcode tools installed (for pkgbuild tool, among others…)

The build_all_mac.sh contains everything to:
- create an Xcode project from the .pro file
- build the Xcode project 
- create a package ready to deploy

In your Terminal, go to the project path and then type :
sh build_all_mac.sh 1.2 /your/absolute/path/to/Qt/5.4/clang_64/bin/
Note : 1.2 is the version number. We use it when releasing official versions.
You may choose what you want instead when building by yourself.