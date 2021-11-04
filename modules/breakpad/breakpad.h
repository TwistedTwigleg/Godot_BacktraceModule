/* breakpad.h */

#ifndef BREAKPAD_H
#define BREAKPAD_H

#include "scene/main/node.h"
#include "core/reference.h"

#ifdef X11_ENABLED
#include <breakpad/client/linux/handler/exception_handler.h>
#endif

// Does not work currently due to linker issues
/*
#ifdef WINDOWS_ENABLED
#include <breakpad/client/windows/handler/exception_handler.h>
#endif
*/

class Breakpad : public Node {
    GDCLASS(Breakpad, Node);

protected:
    static void _bind_methods();

public:
    static String api_URL;
    static String api_token;
    static bool skip_error_upload;
    static Dictionary crash_attributes;
    static bool upload_godot_log;

    void start_breakpad();

#ifdef X11_ENABLED
    static bool dump_callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded);
#endif

// Does not work currently due to linker issues
/*
#ifdef WINDOWS_ENABLED
    static bool dump_callback(const wchar_t* dump_path, const wchar_t* minidump_id, void* context, EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded);
#endif
*/

    void force_crash();

    void set_api_url(String new_url);
    String get_api_url();
    void set_api_token(String new_token);
    String get_api_token();

    void set_skip_error_upload(bool new_value);
    bool get_skip_error_upload();

    void set_crash_attributes(Dictionary new_value);
    Dictionary get_crash_attributes();

    void set_upload_godot_log(bool new_value);
    bool get_upload_godot_log();

    Breakpad();
    ~Breakpad();
};

#endif // BREAKPAD_H