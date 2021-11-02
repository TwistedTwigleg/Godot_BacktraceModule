#include "register_types.h"

#include "core/class_db.h"
#include "breakpad.h"

void register_breakpad_types() {
    ClassDB::register_class<Breakpad>();
}

void unregister_breakpad_types() {
   // Nothing to do here in this example.
}