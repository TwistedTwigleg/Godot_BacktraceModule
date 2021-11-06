# Godot Backtrace Module

This module adds automatic crash generation support to Godot 3.x with the intent of making it possible to send the crash data to [Backtrace](https://backtrace.io/) for automatic error reporting.

**Why use this module?** With automatic crash reporting, you can track, examine, and resolve errors quickly and efficiently without having to rely solely on user feedback or QA sessions. By using a service like Backtrace, you can have a single hub that makes it easy to track all the issues users may be having with detailed error logs. This allows you to spend more time developing fixes and making a great product, not chasing down information.

Check out the GitHub Wiki for more information on how to install and use the module in your Godot projects!

## Features

Please note that this module is still an **early work in progress!** Big changes are still being made, with new features and improvements still to be made to the code.

The feature list below will continue to grow and improve over time as the module is developed. Please check out the roadmap below for an idea of what is coming!

* Adds two new nodes for generating and sending crash report data to online servers
  * Adds two new nodes: `Crashpad` and `Breakpad`.
    * For Windows and MacOS, [Google Crashpad](https://chromium.googlesource.com/crashpad/crashpad/) is used to generate and send crash reports
      * *Documentation for how to build and use Crashpad **coming soon** to the wiki!*
    * For Linux,[Google Breakpad](https://chromium.googlesource.com/breakpad/breakpad/) is used to generate and send crash reports
    * Both nodes will compile and all platforms but will not function properly unless on the correct OS.
      * For example, if using `Breakpad` on Windows, the node will print a warning but otherwise not do anything. Likewise, using `Crashpad` on Linux will just print a warning.
      * This allows you to use both nodes in your projects without having to worry about it crashing or breaking.
  * These nodes generate crash reports and automatically uploads it for processing, making it easier and faster to resolve bugs. The new nodes are the primary interface for setting up and using this module
  * The node provides a flexible system for setting the server URL, project token, and attributes
    * `Breakpad` can also upload the Godot log files automatically if enabled in the node and project settings.
* Written in C++ for fast and efficient error generation
  * This allows the code to capture crashes caused by Godot's C++ code and accurately generate symbol files
* Supports Windows, MacOS, and Linux
  * MacOS support has not yet been tested, but it should work

## Roadmap

Below is the roadmap for features and additions to be made to this module:

* Add documentation on how to build and use Crashpad on Windows and MacOS to the wiki
* `Breakpad` module roadmap:
  * Adjust property names and layout to better fit Crashpad. Make APIs as similar as possible
  * Replace use of `curl` with a built-in HTTPS solution for better portability
* `Crashpad` module roadmap:
  * Add attachment support to upload Godot log files
  * Add Linux support
    * As of when this was written, Crashpad does not support Linux. It is on the ["in progress" section for Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/HEAD/doc/status.md), so support can be added in the future.
  * Add Android support
  * Add iOS support
    * As of when this was written, Crashpad does not support iOS. It is on the ["in progress" section for Crashpad](https://chromium.googlesource.com/crashpad/crashpad/+/HEAD/doc/status.md), so support can be added in the future.
* Add support for Godot 4.0
* (*And more! If you have any suggestions, please make a feature request issue!*)

Please note the roadmap above is not necessarily in priority order and will continue to evolve as development on the module progresses.

## Other

Because this is a Godot module, it will need to be added to Godot source code and then compiled for it to work properly! See the GitHub Wiki for more information on how to install and use the module in your Godot project. Please note that this module is written for Godot 3.x currently, though it should work with any version of Godot 3 without needing many modifications.

