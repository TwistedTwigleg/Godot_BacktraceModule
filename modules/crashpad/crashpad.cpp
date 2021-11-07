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

// Needed for adding a delay to a thread so we can send the dump via curl
#ifdef X11_ENABLED
#include <chrono>
#include <thread>
#endif

#endif

// Static variables
bool Crashpad::crashpad_skip_error_upload = false;
Dictionary Crashpad::crashpad_user_crash_attributes = Dictionary();
bool Crashpad::crashpad_upload_godot_log = false;
// Crashpad variables
String Crashpad::crashpad_api_URL = "";
String Crashpad::crashpad_api_token = "";
String Crashpad::crashpad_database_path = "";
String Crashpad::crashpad_application_path = "";
bool Crashpad::crashpad_linux_delete_crashpad_database_data_on_start = true;
bool Crashpad::crashpad_use_manual_application_extension = false;
String Crashpad::crashpad_manual_application_extension = "";


void Crashpad::start_crashpad() {

#if defined WINDOWS_ENABLED || defined OSX_ENABLED || defined X11_ENABLED
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
    base::FilePath::StringType handler_path(get_std_string_from_godot_string(get_global_crashpad_application_path()));

    base::FilePath db(database_path);
    base::FilePath handler(handler_path);

// Database management is only on Windows and MacOS. For other platforms, we have to upload manually using CURL
// and handle deleting the database files ourselves
#if defined WINDOWS_ENABLED || defined OSX_ENABLED
    std::unique_ptr<crashpad::CrashReportDatabase> database = crashpad::CrashReportDatabase::Initialize(db);

    if (database == nullptr || database->GetSettings() == NULL) {
        ERR_PRINT("Could not initialize crashpad database!");
        print_line("Crashpad Error: Could not initialize crashpad database!");
        return;
    }
    // Enable automatic uploads
    database->GetSettings()->SetUploadsEnabled(true);
#endif
    
    // The Backtrace URL
    String upload_url = Crashpad::crashpad_api_URL + Crashpad::crashpad_api_token + "/minidump";
    std::string upload_url_s = get_std_string_from_godot_string(upload_url);

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
        // Currently not implemented for Windows or MacOS
#if defined WINDOWS_ENABLED || defined OSX_ENABLED
        WARN_PRINT("Log file attachment support not yet implemented!");
        print_line("Crashpad Warning: Log file attachment support not yet implemented!");
#endif
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

    // If running on an unsupported platform, just log an error
    WARN_PRINT("Crashpad not supported on this platform!");
    print_line("Crashpad Warning: Crashpad not supported on this platform!");
    return;

}

void Crashpad::_notification(int p_notification)
{
    // Only needed on Linux. This is because we have to upload the dump ourselves via CURL
    // as there is not any database manager for Linux with Crashpad currently.
#if defined X11_ENABLED
    if (p_notification == NOTIFICATION_READY)
    {
        if (Crashpad::crashpad_linux_delete_crashpad_database_data_on_start == true)
        {
            // Get all the dump files
            Array file_dump_search_results = get_directory_contents(get_global_path_from_local_path(Crashpad::crashpad_database_path), ".dmp");
            if (file_dump_search_results.empty() == false)
            {
                // Delete the dumps
                Array file_search_files = (Array)file_dump_search_results.get(0);
                for (int i = 0; i < file_search_files.size(); i++)
                {
                    DirAccess::remove_file_or_error(file_search_files[i]);
                }
            }
            // Get all the meta files
            Array file_meta_search_results = get_directory_contents(get_global_path_from_local_path(Crashpad::crashpad_database_path), ".meta");
            if (file_meta_search_results.empty() == false)
            {
                // Delete the meta
                Array file_search_files = (Array)file_meta_search_results.get(0);
                for (int i = 0; i < file_search_files.size(); i++)
                {
                    DirAccess::remove_file_or_error(file_search_files[i]);
                }
            }
        }
    }
    else if (p_notification == MainLoop::NOTIFICATION_CRASH) {
		ERR_PRINT("Notification of crash found!");

        // Sleep - so Crashpad can generate the dump
        // Not ideal, but Crashpad on Linux doesn't automatically send the crash, so we have to do it manually
        // using CURL.
        // Credit for snippet: https://stackoverflow.com/questions/4184468/sleep-for-milliseconds
        std::this_thread::sleep_for(std::chrono::microseconds(2000));

        // Get all the dump files
        Array file_search_results = get_directory_contents(get_global_path_from_local_path(Crashpad::crashpad_database_path), ".dmp");
        // Was there an error?
        if (file_search_results.empty() == true)
        {
            return;
        }

        Array file_search_files = (Array)file_search_results.get(0);

        for (int i = 0; i < file_search_files.size(); i++)
        {
            upload_dump_through_curl((String)file_search_files[i]);
        }
	}
#endif
}

#if defined WINDOWS_ENABLED || defined OSX_ENABLED || defined X11_ENABLED
std::string Crashpad::get_std_string_from_godot_string(String input)
{
    // Credit: https://godotengine.org/qa/18552/gdnative-convert-godot-string-to-const-char
    std::wstring input_ws = input.c_str();
    return std::string(input_ws.begin(), input_ws.end());
}
#endif

Array Crashpad::get_directory_contents(String root_directory_path, String desired_extension)
{
    Array files;
    Array directories;
    if (root_directory_path.ends_with("//"))
    {
        root_directory_path.remove(root_directory_path.size()-1);
        root_directory_path.remove(root_directory_path.size()-1);
    }
    else if (root_directory_path.ends_with("/"))
    {
        root_directory_path.remove(root_directory_path.size()-1);
    }
    DirAccess *dir_link = DirAccess::open(root_directory_path);
    
    Error error = dir_link->list_dir_begin();
    if (error == Error::ERR_CANT_OPEN)
    {
        ERR_PRINT("Cannot open Crashpad database folder! Cannot upload crash log automatically");
        return files;
    }

    _add_directory_contents(dir_link, files, directories, desired_extension);
    
    Array return_value;
    return_value.append(files);
    return_value.append(directories);
    return return_value;
}
void Crashpad::_add_directory_contents(DirAccess *dir, Array files, Array directories, String desired_extension)
{
    String filename = dir->get_next();

    while (filename != "")
    {
        if (filename == "." || filename == "..")
        {
            filename = dir->get_next();
        }
        else
        {
            String path = dir->get_current_dir() + "/" + filename;
            if (dir->current_is_dir())
            {
                DirAccess *sub_dir_link = DirAccess::open(path);
                sub_dir_link->list_dir_begin();
                directories.append(path);
                _add_directory_contents(sub_dir_link, files, directories, desired_extension);
            }
            else
            {
                if (path.ends_with(desired_extension))
                {
                    files.append(path);
                }
            }
            filename = dir->get_next();
        }
    }
    dir->list_dir_end();
}

void Crashpad::upload_dump_through_curl(String dump_path)
{
    List<String> arguments;

    // Start uploading using CURL
    arguments.push_back("-v");
    
    // Upload arguments
    for (int i = 0; i < Crashpad::crashpad_user_crash_attributes.size(); i++)
    {
        Variant key = Crashpad::crashpad_user_crash_attributes.get_key_at_index(i);
        Variant value = Crashpad::crashpad_user_crash_attributes.get_value_at_index(i);
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
        arguments.push_back(Crashpad::crashpad_api_URL + Crashpad::crashpad_api_token + "/minidump");
    }
    
    // Uploading the actual Minidump
    arguments.push_back(Crashpad::crashpad_api_URL + Crashpad::crashpad_api_token + "/minidump");

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
}

bool Crashpad::check_for_crashpad_application()
{
    // If the application path is set to an empty string, then set it so it's relative to the application
    if (Crashpad::crashpad_application_path.empty() == true)
    {
        Crashpad::crashpad_application_path = "res://crashpad_handler.exe";
    }

    String actual_path = get_global_crashpad_application_path();
    return FileAccess::exists(actual_path);
}
String Crashpad::get_global_crashpad_application_path()
{
    String actual_path = "";
    if (Crashpad::crashpad_use_manual_application_extension == true)
    {
        actual_path = get_global_path_from_local_path(Crashpad::crashpad_application_path + Crashpad::crashpad_manual_application_extension);
    }
    else
    {
        // Default to just the file name
        actual_path = get_global_path_from_local_path(Crashpad::crashpad_application_path);

#ifdef WINDOWS_ENABLED
        // The default extension is .exe
        actual_path = get_global_path_from_local_path(Crashpad::crashpad_application_path + ".exe");
#endif

#ifdef OSX_ENABLED
        // I believe it is just the name of the file, no extension needed
        actual_path = get_global_path_from_local_path(Crashpad::crashpad_application_path);
#endif

#ifdef X11_ENABLED
        // The default extension is nothing, just the file name
        actual_path = get_global_path_from_local_path(Crashpad::crashpad_application_path);
#endif
    }
    return actual_path;
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
    
    ClassDB::bind_method(D_METHOD("set_use_manual_application_extension", "use_manual_extension"), &Crashpad::set_crashpad_use_manual_application_extension);
	ClassDB::bind_method(D_METHOD("get_use_manual_application_extension"), &Crashpad::get_crashpad_use_manual_application_extension);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "crashpad_settings/use_manual_application_extension", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_use_manual_application_extension", "get_use_manual_application_extension");
    ClassDB::bind_method(D_METHOD("set_manual_application_extension", "manual_extension"), &Crashpad::set_crashpad_manual_application_extension);
	ClassDB::bind_method(D_METHOD("get_manual_application_extension"), &Crashpad::get_crashpad_manual_application_extension);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "crashpad_settings/manual_application_extension", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT_INTL), "set_manual_application_extension", "get_manual_application_extension");
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

void Crashpad::set_crashpad_use_manual_application_extension(bool new_value) {
    Crashpad::crashpad_use_manual_application_extension = true;
}
bool Crashpad::get_crashpad_use_manual_application_extension() {
    return Crashpad::crashpad_use_manual_application_extension;
}
void Crashpad::set_crashpad_manual_application_extension(String new_value) {
    Crashpad::crashpad_manual_application_extension = new_value;
}
String Crashpad::get_crashpad_manual_application_extension() {
    return Crashpad::crashpad_manual_application_extension;
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

    Crashpad::crashpad_use_manual_application_extension = get("crashpad_settings/use_manual_application_extension");
    Crashpad::crashpad_manual_application_extension = get("crashpad_settings/manual_application_extension");
}

Crashpad::~Crashpad()
{

}