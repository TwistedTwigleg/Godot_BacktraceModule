/* breakpad.cpp */

#include "breakpad.h"
#include "core/os/os.h"

#include <breakpad/client/linux/handler/exception_handler.h>
#include <breakpad/common/linux/http_upload.h>

String Breakpad::api_URL = "";
String Breakpad::api_token = "";
bool Breakpad::skip_error_upload = false;
Dictionary Breakpad::crash_attributes = Dictionary();


void Breakpad::start_crashpad() {
    static google_breakpad::MinidumpDescriptor descriptor("/tmp");
    google_breakpad::ExceptionHandler* excHandler = NULL;
    excHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, dump_callback, NULL, true, -1);
}

void Breakpad::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_crashpad"), &Breakpad::start_crashpad);
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


bool Breakpad::dump_callback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
    //printf("Dump path: %s\n", descriptor.path());
    
    if (Breakpad::skip_error_upload == true)
    {
        return succeeded;
    }

    List<String> arguments;
    arguments.push_back("-v");
    arguments.push_back("--data-binary");
    arguments.push_back("@" + String(descriptor.path()));

    String arguments_string = "";

    for (int i = 0; i < Breakpad::crash_attributes.size(); i++)
    {
        Variant key = Breakpad::crash_attributes.get_key_at_index(i);
        Variant value = Breakpad::crash_attributes.get_value_at_index(i);
        
        String key_string = (String)key;
        String value_string = (String)value;

        // TODO - test this!
        arguments_string += ("&" + key_string + "=" + value_string);
    }

    arguments.push_back(Breakpad::api_URL + Breakpad::api_token + "/minidump" + arguments_string);

    OS::get_singleton()->execute("curl", arguments);

    // Debug - view CURL output string
    /*
    String curl_debug_string = "curl ";
    for (int i = 0; i < arguments.size(); i++) {
        curl_debug_string += " " + arguments[i];
    }
    print_line("Curl execute call: Curl " + curl_debug_string + "\n");
    */

    return succeeded;
}

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
}

Breakpad::~Breakpad()
{

}