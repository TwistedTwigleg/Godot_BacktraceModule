# Godot Backtrace Module

This module adds automatic crash generation support to Godot 3.x with the intent of making it possible to send the crash data to [Backtrace](https://backtrace.io/) for automatic error reporting.

**Why use this module?** With automatic crash reporting, you can track, examine, and resolve errors quickly and efficiently without having to rely solely on user feedback or QA sessions. By using a service like Backtrace, you can have a single hub that makes it easy to track all the issues users may be having with detailed error logs. This allows you to spend more time developing fixes and making a great product, not chasing down information.

Check out the GitHub Wiki for more information on how to install and use the module in your Godot projects!

## Features

Please note that this module is still an **early work in progress!** Big changes are still being made, with new features and improvements still to be made to the code.

The feature list below will continue to grow and improve over time as the module is developed. Please check out the roadmap below for an idea of what is coming!

* Adds a new node for generating and sending crash report data to online servers
  * Crash report generation is currently using [Google Breakpad](https://chromium.googlesource.com/breakpad/breakpad/)
  * This node generates crash reports and automatically uploads it for processing, making it easier and faster to resolve bugs. This node is currently the primary interface for setting up and using this module
  * The node provides a flexible system for setting the server URL, project token, and attributes
  * Currently the node is named `Breakpad`, however this name may change in the future!
* Written in C++ for fast and efficient error generation
  * This allows the code to capture crashes caused by Godot's C++ code and accurately generate symbol files
* More platforms coming soon! (Currently only supports Linux)

## Roadmap

Below is the roadmap for features and additions to be made to this module:

* Add Windows support
* Add MacOS support
* Replace Google Breakpad with [Google Crashpad](https://chromium.googlesource.com/crashpad/crashpad/)
  * Using Crashpad should allow for better MacOS support, as well as more cross-platform code
* Replace use of `curl` with a built-in HTTPS solution for better portability
* Add support for attaching Godot generated log files automatically to crash reports
* Add Android and iOS support
* Add support for Godot 4.0
* (*And more! If you have any suggestions, please make a feature request issue!*)

Please note the roadmap above is not necessarily in priority order and will continue to evolve as development on the module progresses.

## Other

Because this is a Godot module, it will need to be added to Godot source code and then compiled for it to work properly! See the GitHub Wiki for more information on how to install and use the module in your Godot project. Please note that this module is written for Godot 3.x currently, though it should work with any version of Godot 3 without needing many modifications.

