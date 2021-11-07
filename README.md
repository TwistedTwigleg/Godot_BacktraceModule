# Godot Backtrace Module

This module adds automatic crash generation support to Godot 3.x with the intent of making it possible to send the crash data to [Backtrace](https://backtrace.io/) for automatic error reporting.

**Why use this module?** With automatic crash reporting, you can track, examine, and resolve errors quickly and efficiently without having to rely solely on user feedback or QA sessions. By using a service like Backtrace, you can have a single hub that makes it easy to track all the issues users may be having with detailed error logs. This allows you to spend more time developing fixes and making a great product, not chasing down information.

Check out the GitHub Wiki for more information on how to install and use the module in your Godot projects!

## Features

Please note that this module is still a **work in progress!** Changes are still being made, with new features and improvements still to be made to the code.

The feature list below will continue to grow and improve over time as the module is developed. Please check out the roadmap below for an idea of what is coming!

* Adds a new node for generating and sending crash report data automatically to online servers
  * Adds a `Crashpad` node, which uses [Google Crashpad](https://chromium.googlesource.com/crashpad/crashpad/) to generate and send crash reports
  * The module will compile and all platforms but will not function properly unless on the correct OS.
    * For example, if using `Crashpad` on iOS or Android, the node will print a warning but otherwise not do anything.
    * This allows you to use both nodes in your projects without having to worry about it crashing or breaking.
  * The `Crashpad` node exposes all the properties needed for setup in the Godot editor
    * Custom attributes can be set for easy sorting and filtering of uploaded error reports
  * (*Coming soon to Windows and MacOS*) Supports sending the Godot log files alongside the crash report
    * If writing the log to a file is enabled in the project settings, `Crashpad` will upload the log alongside the C++ generated crash
* Written in C++ for fast and efficient error generation
  * This allows the code to capture crashes caused by Godot's C++ code and accurately generate symbol files
* Supports Windows, MacOS, and Linux
  * MacOS support has not yet been tested, but it should work

## Roadmap

Below is the roadmap for features and additions to be made to this module:

* Add documentation on how to build and use Crashpad on Windows, MacOS, and Linux to the wiki
* `Crashpad` module roadmap:
  * Add attachment support to upload Godot log files for Windows and MacOS
  * Investigate adding Android support
  * Investigate add iOS support
  * Investigate adding support for adding a screenshot of the Godot application at the moment of the crash
* Add support for Godot 4.0
* Investigate adding support for `Error-free users` and `Error-free sessions` for Bracktrace
* (*And more! If you have any suggestions, please make a feature request issue!*)

Please note the roadmap above is not necessarily in priority order and will continue to evolve as development on the module progresses.

## Other

Because this is a Godot module, it will need to be added to Godot source code and then compiled for it to work properly! See the GitHub Wiki for more information on how to install and use the module in your Godot project. Please note that this module is written for Godot 3.x currently, though it should work with any version of Godot 3 without needing many modifications.

