/* breakpad.h */

#ifndef BREAKPAD_H
#define BREAKPAD_H

#include "scene/main/node.h"
#include "core/reference.h"

#include <breakpad/client/linux/handler/exception_handler.h>

class Breakpad : public Node {
    GDCLASS(Breakpad, Node);

protected:
    static void _bind_methods();

public:
    static String api_URL;
    static String api_token;

    static bool skip_error_upload;

    static Dictionary crash_attributes;

    void start_breakpad();

    static bool dump_callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded);
    void force_crash();

    void set_api_url(String new_url);
    String get_api_url();
    void set_api_token(String new_token);
    String get_api_token();

    void set_skip_error_upload(bool new_value);
    bool get_skip_error_upload();

    void set_crash_attributes(Dictionary new_value);
    Dictionary get_crash_attributes();

    Breakpad();
    ~Breakpad();
};

#endif // BREAKPAD_H