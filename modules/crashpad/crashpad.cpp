/* crashpad.cpp */

#include "crashpad.h"
#include "core/os/os.h"
#include "core/os/file_access.h"
#include "core/os/dir_access.h"
#include "core/project_settings.h"


// We have to use ifdef because Crashpad is currently not supported on Linux
#if defined WINDOWS_ENABLED || defined OSX_ENABLED
#include "crashpad/client/crash_report_database.h"
#include "crashpad/client/crashpad_client.h"
#include "crashpad/client/settings.h"

// Needed to avoid compiling/linking issues on Windows
#ifdef WINDOWS_ENABLED
#define NOMINMAX
#endif

#endif

// TODO - add Linux support once Crashpad supports Linux
// Use "ifdef X11_ENABLED" for Linux

// Static variables
bool Crashpad::crashpad_skip_error_upload = false;
Dictionary Crashpad::crashpad_user_crash_attributes = Dictionary();
bool Crashpad::crashpad_upload_godot_log = false;
// Crashpad variables
String Crashpad::crashpad_api_URL = "";
String Crashpad::crashpad_api_token = "";
String Crashpad::crashpad_database_path = "";
String Crashpad::crashpad_application_path = "";


void Crashpad::start_crashpad() {

// Crashpad only works on Windows and MacOS currently
#if defined WINDOWS_ENABLED || defined OSX_ENABLED

    // Make sure the application exists
    if (check_for_crashpad_application() == false) {
        String application_path = get_global_path_from_local_path(Crashpad::crashpad_application_path);
        ERR_PRINT("Cannot find crashpad_handle application! Setup 'application_path' in editor to point to application! Input file path: " + application_path);
        print_line("Crashpad Error: Cannot find crashpad_handle application! Setup 'application_path' in editor to point to application! Input file path: " + application_path);
        return;
    }
    // Make sure the crashpad database directory exists or create it if it does not
    check_for_crashpad_database(true);

    base::FilePath::StringType database_path(get_std_string_from_godot_string(get_global_path_from_local_path(Crashpad::crashpad_database_path)));
    base::FilePath::StringType handler_path(get_std_string_from_godot_string(get_global_path_from_local_path(Crashpad::crashpad_application_path)));

    base::FilePath db(database_path);
    base::FilePath handler(handler_path);
    std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(db);

    if (database == nullptr || database->GetSettings() == NULL) {
        ERR_PRINT("Could not initialize crashpad database!");
        print_line("Crashpad Error: Could not initialize crashpad database!");
        return;
    }
    // Enable automatic uploads
    database->GetSettings()->SetUploadsEnabled(true);
    
    // The Backtrace URL
    String upload_url = Crashpad::crashpad_api_URL + Crashpad::crashpad_api_token + "/minidump";
    std::string upload_url_s = get_std_string_from_godot_string(upload_url);

    // Credit: https://godotengine.org/qa/18552/gdnative-convert-godot-string-to-const-char
    //std::wstring upload_url_ws = upload_url.c_str();
    //std::string upload_url_s(upload_url_ws.begin(), upload_url_ws.end());

    // Remove upload limit for now
    crashpad_arguments.push_back("--no-rate-limit");

    // Add annotations
    for (int i = 0; i < crashpad_user_crash_attributes.size(); i++)
    {
        Variant key_variant = crashpad_user_crash_attributes.get_key_at_index(i);
        Variant value_variant = crashpad_user_crash_attributes.get_value_at_index(i);
        String key_string = (String)key_variant;
        String value_string = (String)value_variant;

        crashpad_annotations.insert(std::pair<std::string, std::string>(get_std_string_from_godot_string(key_string), get_std_string_from_godot_string(value_string)));
    }

    // Add log file attachment?
    if (Crashpad::crashpad_upload_godot_log == true)
    {
        WARN_PRINT("Log file attachment support not yet implemented!");
        print_line("Crashpad Warning: Log file attachment support not yet implemented!");
    }

    // Skip starting the client?
    if (Crashpad::crashpad_skip_error_upload == true)
    {
        OS::get_singleton()->print("Skipped Crashpad error uploading!");
        print_line("Crashpad Note: Skipped Crashpad error uploading!");
        return;
    }

    crashpad_client_init = crashpad_client.StartHandler(
        handler,
        db,
        db,
        upload_url_s,
        crashpad_annotations,
        crashpad_arguments,
        true,
        true
    );
    if (crashpad_client_init == false) {
        ERR_PRINT("Could not initialize crashpad client!");
        return;
    }

    OS::get_singleton()->print("Crashpad initialized successfully!");
    print_line("Crashpad Note: Crashpad initialized successfully!");
    return;

#endif

    WARN_PRINT("Crashpad not supported on this platform!");
    print_line("Crashpad Warning: Crashpad not supported on this platform!");
    return;

}

// Crashpad only works on Windows and MacOS currently
#if defined WINDOWS_ENABLED || defined OSX_ENABLED
std::string Crashpad::get_std_string_from_godot_string(String input)
{
    // Credit: https://godotengine.org/qa/18552/gdnative-convert-godot-string-to-const-char
    std::wstring input_ws = input.c_str();
    return std::string(input_ws.begin(), input_ws.end());
}
#endif

bool Crashpad::check_for_crashpad_application()
{
    // If the application path is set to an empty string, then set it so it's relative to the application
    if (Crashpad::crashpad_application_path.empty() == true)
    {
#ifdef WINDOWS_ENABLED
        Crashpad::crashpad_application_path = "res://crashpad_handler.exe";
#endif
        // TODO - add Linux and MacOS support!
    }

    String actual_path = get_global_path_from_local_path(Crashpad::crashpad_application_path);
    return FileAccess::exists(actual_path);
}
String Crashpad::get_global_path_from_local_path(String input_local_path)
{
    String actual_path = input_local_path;

    // Convert res:// to the actual directory
    if (input_local_path.begins_with("res://"))
    {
        actual_path = OS::get_singleton()->get_executable_path().get_base_dir();
        actual_path += input_local_path.replace("res://", "/");
    }
    // Convert user:// to the actual directory
    else if (input_local_path.begins_with("user://"))
    {
        actual_path = ProjectSettings::get_singleton()->localize_path(input_local_path);
    }

    return actual_path;
}

bool Crashpad::check_for_crashpad_database(bool make_if_not_exist)
{
    if (Crashpad::crashpad_database_path.empty() == true)
    {
        Crashpad::crashpad_database_path = "res://Crashpad/db/";
    }

    String actual_path = get_global_path_from_local_path(Crashpad::crashpad_database_path);

    if (DirAccess::exists(actual_path) == false)
    {
        if (make_if_not_exist == true)
        {
            DirAccess *d = DirAccess::create_for_path(actual_path);
            Error err = d->make_dir_recursive(actual_path);
            memdelete(d);
            return true;
        }
        else
        {
            return false;
        }
    }
    return true;
}

void Crashpad::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_crashpad"), &Crashpad::start_crashpad);
    ClassDB::bind_method(D_METHOD("force_crash"), &Crashpad::force_crash);

    // Crashpad setup variables
    // =====
    ClassDB::bind_method(D_METHOD("set_api_url", "api_url"), &Crashpad::set_crashpad_api_url);
	ClassDB::bind_method(D_METHOD("get_api_url"), &Crashpad::get_crashpad_api_url);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "crashpad_settings/api_url", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_api_url", "get_api_url");
    ClassDB::bind_method(D_METHOD("set_api_token", "api_token"), &Crashpad::set_crashpad_api_token);
	ClassDB::bind_method(D_METHOD("get_api_token"), &Crashpad::get_crashpad_api_token);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "crashpad_settings/crashpad_api_token", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_api_token", "get_api_token");
    ClassDB::bind_method(D_METHOD("set_application_path", "new_path"), &Crashpad::set_crashpad_application_path);
	ClassDB::bind_method(D_METHOD("get_application_path"), &Crashpad::get_crashpad_application_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "crashpad_settings/application_path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_application_path", "get_application_path");
    ClassDB::bind_method(D_METHOD("set_database_path", "new_path"), &Crashpad::set_crashpad_database_path);
	ClassDB::bind_method(D_METHOD("get_database_path"), &Crashpad::get_crashpad_database_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "crashpad_settings/database_path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_database_path", "get_database_path");
    // =====

    // User custom data
    // =====
    ClassDB::bind_method(D_METHOD("set_crash_attributes", "crash_attributes"), &Crashpad::set_crashpad_user_crash_attributes);
	ClassDB::bind_method(D_METHOD("get_crash_attributes"), &Crashpad::get_crashpad_user_crash_attributes);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "custom_data/user_crash_attributes", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_crash_attributes", "get_crash_attributes");

    ClassDB::bind_method(D_METHOD("set_upload_godot_log", "upload_godot_log"), &Crashpad::set_crashpad_upload_godot_log);
	ClassDB::bind_method(D_METHOD("get_upload_godot_log"), &Crashpad::get_crashpad_upload_godot_log);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "custom_data/upload_godot_log", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_upload_godot_log", "get_upload_godot_log");
    // =====

    ClassDB::bind_method(D_METHOD("set_skip_error_upload", "skip_error_upload"), &Crashpad::set_crashpad_skip_error_upload);
	ClassDB::bind_method(D_METHOD("get_skip_error_upload"), &Crashpad::get_crashpad_skip_error_upload);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "skip_error_upload", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_skip_error_upload", "get_skip_error_upload");
}

void Crashpad::set_crashpad_api_url(String new_url)
{
    Crashpad::crashpad_api_URL = new_url;
}
String Crashpad::get_crashpad_api_url()
{
    return Crashpad::crashpad_api_URL;
}
void Crashpad::set_crashpad_api_token(String new_token)
{
    Crashpad::crashpad_api_token = new_token;
}
String Crashpad::get_crashpad_api_token()
{
    return Crashpad::crashpad_api_token;
}
void Crashpad::set_crashpad_application_path(String new_path)
{
    Crashpad::crashpad_application_path = new_path;
}
String Crashpad::get_crashpad_application_path()
{
    return Crashpad::crashpad_application_path;
}
void Crashpad::set_crashpad_database_path(String new_path)
{
    Crashpad::crashpad_database_path = new_path;
}
String Crashpad::get_crashpad_database_path()
{
    return Crashpad::crashpad_database_path;
}

void Crashpad::set_crashpad_skip_error_upload(bool new_value)
{
    Crashpad::crashpad_skip_error_upload = new_value;
}
bool Crashpad::get_crashpad_skip_error_upload()
{
    return Crashpad::crashpad_skip_error_upload;
}

void Crashpad::set_crashpad_user_crash_attributes(Dictionary new_value)
{
    Crashpad::crashpad_user_crash_attributes = new_value;
}
Dictionary Crashpad::get_crashpad_user_crash_attributes()
{
    return Crashpad::crashpad_user_crash_attributes;
}

void Crashpad::set_crashpad_upload_godot_log(bool new_value) {
    Crashpad::crashpad_upload_godot_log = new_value;
}
bool Crashpad::get_crashpad_upload_godot_log() {
    return Crashpad::crashpad_upload_godot_log;
}

void Crashpad::force_crash()
{
    volatile int* a = (int*)(NULL); *a = 1;
}


Crashpad::Crashpad()
{
    Crashpad::crashpad_api_URL = get("crashpad_settings/api_url");
    Crashpad::crashpad_api_token = get("crashpad_settings/api_token");
    Crashpad::crashpad_application_path = get("crashpad_settings/application_path");

    Crashpad::crashpad_user_crash_attributes = get("custom_data/user_crash_attributes");
    Crashpad::crashpad_upload_godot_log = get("custom_data/upload_godot_log");

    Crashpad::crashpad_skip_error_upload = get("skip_error_upload");
}

Crashpad::~Crashpad()
{

}