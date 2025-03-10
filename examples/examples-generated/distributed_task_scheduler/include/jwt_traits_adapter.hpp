// MIT License
// Copyright (c) 2025 dbjwhs

#pragma once

#include "../external/jwt-cpp/include/jwt-cpp/traits/kazuho-picojson/traits.h"

namespace jwt {
    namespace traits {
        // Adapted traits class that adds the missing functions needed by jwt-cpp
        struct kazuho_picojson_adapter : public jwt::traits::kazuho_picojson {
            using value_type = picojson::value;
            
            // Add the missing as_integer and as_boolean functions
            static int64_t as_integer(const picojson::value& val) {
                if (!val.is<int64_t>()) throw std::bad_cast();
                return val.get<int64_t>();
            }
            
            static bool as_boolean(const picojson::value& val) {
                if (!val.is<bool>()) throw std::bad_cast();
                return val.get<bool>();
            }
        };
    }
}