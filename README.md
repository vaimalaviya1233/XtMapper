# XtMapper

[![Build debug APK](https://github.com/Xtr126/XtMapper/actions/workflows/android.yml/badge.svg)](https://github.com/Xtr126/XtMapper/actions/workflows/android.yml)

XtMapper is an free and open source keymapper application in development for Bliss OS.  
It can be used to play certain Android games that require a touchscreen, with keyboard and mouse.

# Currently working:

- Multi-touch emulation
- Emulate a touch pointer with mouse - Useful for games that only accept touch events and not mouse clicks.
- Emulate a D-pad with W,A,S,D or arrow keys
- Keyboard events to touch - And editing config with GUI
- Aim with mouse in FPS games 
 
APK can be obtained from GitHub actions (latest) or [`releases`](https://github.com/Xtr126/XtMapper/releases).

# Contributing

Pull requests for a bug fix or even a new feature are welcome.  
If you want to contribute code, please base your commits on the latest dev branch.  

Overview:
- Touch emulation is handled by TouchPointer.java and Input.java.  
- Input.java runs separately from the app as an elevated java process using app_process and adb shell/root, check Server.java for more details. 
