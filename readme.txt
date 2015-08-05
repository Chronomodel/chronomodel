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

——————————————————
ADDITIONNAL NOTES
——————————————————
Generate a MAC icon :
Use command line tool in the « icon » folder:
iconutil -c icns Chronomodel.iconset
https://developer.apple.com/library/mac/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Optimizing/Optimizing.html#//apple_ref/doc/uid/TP40012302-CH7-SW3


Windows compilation notes:
When compiling on Windows, if you get a message like :
'<UNC path>' is an invalid current directory path. 
Then refer to this documentation to solve the problem :
https://support.microsoft.com/en-us/kb/156276
It will guide you through adding a new register key.
Without it, Qt may fail to compile the resource file Chronomodel.rc


