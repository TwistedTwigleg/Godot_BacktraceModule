/* crashpad.h */

#ifndef CRASHPAD_H
#define CRASHPAD_H

#include "scene/main/node.h"
#include "core/reference.h"

// Crashpad only works on Windows and MacOS currently
#if defined WINDOWS_ENABLED || defined OSX_ENABLED
#include "crashpad/client/crash_report_database.h"
#include "crashpad/client/crashpad_client.h"
#include "crashpad/client/settings.h"
#endif

class Crashpad : public Node {
    GDCLASS(Crashpad, Node);

protected:
    static void _bind_methods();

public:
    static bool crashpad_skip_error_upload;
    static Dictionary crashpad_user_crash_attributes;
    static bool crashpad_upload_godot_log;

    static String crashpad_api_URL;
    static String crashpad_api_token;
    static String crashpad_database_path;
    static String crashpad_application_path;

    // Crashpad only works on Windows and MacOS currently
    #if defined WINDOWS_ENABLED || defined OSX_ENABLED
    crashpad::CrashpadClient crashpad_client;
    bool crashpad_client_init = false;
    std::map<std::string, std::string> crashpad_annotations;
    std::vector<std::string> crashpad_arguments;
    #endif

    void start_crashpad();
    void force_crash();

    void set_crashpad_api_url(String new_url);
    String get_crashpad_api_url();
    void set_crashpad_api_token(String new_token);
    String get_crashpad_api_token();
    void set_crashpad_database_path(String new_path);
    String get_crashpad_database_path();
    void set_crashpad_application_path(String new_path);
    String get_crashpad_application_path();

    void set_crashpad_skip_error_upload(bool new_value);
    bool get_crashpad_skip_error_upload();

    void set_crashpad_user_crash_attributes(Dictionary new_value);
    Dictionary get_crashpad_user_crash_attributes();

    void set_crashpad_upload_godot_log(bool new_value);
    bool get_crashpad_upload_godot_log();

    Crashpad();
    ~Crashpad();

private:
    bool check_for_crashpad_application();
    bool check_for_crashpad_database(bool make_if_not_exist);
    String get_global_path_from_local_path(String input_local_path);

    // Crashpad only works on Windows and MacOS currently
    #if defined WINDOWS_ENABLED || defined OSX_ENABLED
    std::string get_std_string_from_godot_string(String input);
    #endif

};

#endif // CRASHPAD_H