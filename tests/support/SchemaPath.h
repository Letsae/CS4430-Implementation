#pragma once

#include <string>

#ifndef LMS_SCHEMA_PATH
#error "LMS_SCHEMA_PATH must be defined by the test target"
#endif

namespace Lms::TestSupport {

inline std::string schemaPath() {
    return LMS_SCHEMA_PATH;
}

} // namespace Lms::TestSupport
