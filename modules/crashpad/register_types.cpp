#include "register_types.h"

#include "core/class_db.h"
#include "crashpad.h"

void register_crashpad_types() {
    ClassDB::register_class<Crashpad>();
}

void unregister_crashpad_types() {
   // Nothing to do here in this example.
}