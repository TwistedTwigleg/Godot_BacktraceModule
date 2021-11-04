/* breakpad.cpp */

#include "breakpad.h"
#include "core/os/os.h"
#include "core/project_settings.h"

#ifdef X11_ENABLED
#include <breakpad/client/linux/handler/exception_handler.h>
#endif

// Does not work currently due to linker issues
/*
#ifdef WINDOWS_ENABLED
#include <breakpad/client/windows/handler/exception_handler.h>
#endif
*/

// TODO - add MaCOS Support
// Use "#ifdef OSX_ENABLED"

// Static variables
String Breakpad::api_URL = "";
String Breakpad::api_token = "";
bool Breakpad::skip_error_upload = false;
Dictionary Breakpad::crash_attributes = Dictionary();
bool Breakpad::upload_godot_log = false;


void Breakpad::start_breakpad() {
#ifdef X11_ENABLED
    static google_breakpad::MinidumpDescriptor descriptor("/tmp");
    google_breakpad::ExceptionHandler* excHandler = NULL;
    excHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, dump_callback, NULL, true, -1);
    return;
#endif

    // Does not work currently due to linker issues
    /*
#ifdef WINDOWS_ENABLED
    // Get the temporary directory from the environment variables
    String temp_dir = OS::get_singleton()->get_environment("%TEMP%");  
    google_breakpad::ExceptionHandler* excHandler = NULL;
    excHandler = new google_breakpad::ExceptionHandler(temp_dir, NULL, dump_callback, NULL, google_breakpad::ExceptionHandler::HandlerType::HANDLER_ALL);
    return;
#endif
    */

    // If we have not returned yet, then this platform does not support crash reporting yet.
    WARN_PRINT("Automatic error report generation not currently supported for this platform!");
    return;
}

void Breakpad::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_breakpad"), &Breakpad::start_breakpad);
    ClassDB::bind_method(D_METHOD("force_crash"), &Breakpad::force_crash);

    ClassDB::bind_method(D_METHOD("set_api_url", "api_url"), &Breakpad::set_api_url);
	ClassDB::bind_method(D_METHOD("get_api_url"), &Breakpad::get_api_url);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "api_URL", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_api_url", "get_api_url");

    ClassDB::bind_method(D_METHOD("set_api_token", "api_token"), &Breakpad::set_api_token);
	ClassDB::bind_method(D_METHOD("get_api_token"), &Breakpad::get_api_token);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "api_token", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_api_token", "get_api_token");

    ClassDB::bind_method(D_METHOD("set_crash_attributes", "api_token"), &Breakpad::set_crash_attributes);
	ClassDB::bind_method(D_METHOD("get_crash_attributes"), &Breakpad::get_crash_attributes);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "crash_attributes", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_crash_attributes", "get_crash_attributes");

    ClassDB::bind_method(D_METHOD("set_upload_godot_log", "upload_godot_log"), &Breakpad::set_upload_godot_log);
	ClassDB::bind_method(D_METHOD("get_upload_godot_log"), &Breakpad::get_upload_godot_log);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "upload_godot_log", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_upload_godot_log", "get_upload_godot_log");

    ClassDB::bind_method(D_METHOD("set_skip_error_upload", "skip_error_upload"), &Breakpad::set_skip_error_upload);
	ClassDB::bind_method(D_METHOD("get_skip_error_upload"), &Breakpad::get_skip_error_upload);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "skip_error_upload", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_skip_error_upload", "get_skip_error_upload");
}

void Breakpad::set_api_url(String new_url)
{
    Breakpad::api_URL = new_url;
}
String Breakpad::get_api_url()
{
    return Breakpad::api_URL;
}
void Breakpad::set_api_token(String new_token)
{
    Breakpad::api_token = new_token;
}
String Breakpad::get_api_token()
{
    return Breakpad::api_token;
}

void Breakpad::set_skip_error_upload(bool new_value)
{
    Breakpad::skip_error_upload = new_value;
}
bool Breakpad::get_skip_error_upload()
{
    return Breakpad::skip_error_upload;
}

void Breakpad::set_crash_attributes(Dictionary new_value)
{
    Breakpad::crash_attributes = new_value;
}
Dictionary Breakpad::get_crash_attributes()
{
    return Breakpad::crash_attributes;
}

void Breakpad::set_upload_godot_log(bool new_value) {
    Breakpad::upload_godot_log = new_value;
}
bool Breakpad::get_upload_godot_log() {
    return Breakpad::upload_godot_log;
}

#ifdef X11_ENABLED
bool Breakpad::dump_callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
    if (Breakpad::skip_error_upload == true)
    {
        return succeeded;
    }

    List<String> arguments;

    // Start uploading using CURL
    arguments.push_back("-v");
    
    // Upload arguments
    for (int i = 0; i < Breakpad::crash_attributes.size(); i++)
    {
        Variant key = Breakpad::crash_attributes.get_key_at_index(i);
        Variant value = Breakpad::crash_attributes.get_value_at_index(i);
        String key_string = (String)key;
        String value_string = (String)value;

        arguments.push_back("-F");
        arguments.push_back(key_string + "=" + value_string);
    }

    // Upload Minidump (setup)
    arguments.push_back("-F");
    arguments.push_back("upload_file_minidump=@" + String(descriptor.path()));
    arguments.push_back("-H");
    arguments.push_back("Expect: gzip");

    // Upload log file (optional)
    ProjectSettings* project_singleton = ProjectSettings::get_singleton();
    Variant project_setting_logging_enabled = project_singleton->get_setting("logging/file_logging/enable_file_logging");
    if (project_setting_logging_enabled.get_type() == project_setting_logging_enabled.BOOL && (bool)project_setting_logging_enabled == true) {
        Variant logging_filepath = project_singleton->get_setting("logging/file_logging/log_path");
        String logging_filepath_string = (String)logging_filepath;
        String log_filepath = project_singleton->globalize_path(logging_filepath_string);

        // Upload log
        arguments.push_back("-F");
        arguments.push_back("godot_log.log=@" + log_filepath + "; type=application/text");
        arguments.push_back(Breakpad::api_URL + Breakpad::api_token + "/minidump");
    }
    
    // Uploading the actual Minidump
    arguments.push_back(Breakpad::api_URL + Breakpad::api_token + "/minidump");

    // Calling it on CURL
    OS::get_singleton()->execute("curl", arguments);

    // For debugging only: See what is being passed to CURL
    /*
    String output_test = "curl ";
    for (int i = 0; i < arguments.size(); i++) {
        output_test += " " + arguments[i];
    }
    print_line(output_test);
    */

    return succeeded;
}
#endif

// Does not work currently due to linker issues
/*
#ifdef WINDOWS_ENABLED
bool Breakpad::dump_callback(const wchar_t* dump_path, const wchar_t* minidump_id, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded)
{
    if (Breakpad::skip_error_upload == true)
    {
        return succeeded;
    }

    List<String> arguments;

    // Start uploading using CURL
    arguments.push_back("-v");
    
    // Upload arguments
    for (int i = 0; i < Breakpad::crash_attributes.size(); i++)
    {
        Variant key = Breakpad::crash_attributes.get_key_at_index(i);
        Variant value = Breakpad::crash_attributes.get_value_at_index(i);
        String key_string = (String)key;
        String value_string = (String)value;

        arguments.push_back("-F");
        arguments.push_back(key_string + "=" + value_string);
    }

    // Upload Minidump (setup)
    arguments.push_back("-F");
    arguments.push_back("upload_file_minidump=@" + String(dump_path));
    arguments.push_back("-H");
    arguments.push_back("Expect: gzip");

    // Upload log file (optional)
    ProjectSettings* project_singleton = ProjectSettings::get_singleton();
    Variant project_setting_logging_enabled = project_singleton->get_setting("logging/file_logging/enable_file_logging");
    if (project_setting_logging_enabled.get_type() == project_setting_logging_enabled.BOOL && (bool)project_setting_logging_enabled == true) {
        Variant logging_filepath = project_singleton->get_setting("logging/file_logging/log_path");
        String logging_filepath_string = (String)logging_filepath;
        String log_filepath = project_singleton->globalize_path(logging_filepath_string);

        // Upload log
        arguments.push_back("-F");
        arguments.push_back("godot_log.log=@" + log_filepath + "; type=application/text");
        arguments.push_back(Breakpad::api_URL + Breakpad::api_token + "/minidump");
    }
    
    // Uploading the actual Minidump
    arguments.push_back(Breakpad::api_URL + Breakpad::api_token + "/minidump");

    // Calling it on CURL
    OS::get_singleton()->execute("curl", arguments);

    return succeeded;
}
#endif
*/


void Breakpad::force_crash()
{
    volatile int* a = (int*)(NULL); *a = 1;
}


Breakpad::Breakpad()
{
    Breakpad::api_URL = get("get_api_url");
    Breakpad::api_token = get("get_api_token");
    Breakpad::skip_error_upload = get("skip_error_upload");
    Breakpad::crash_attributes = get("crash_attributes");
    Breakpad::upload_godot_log = get("upload_godot_log");
}

Breakpad::~Breakpad()
{

}