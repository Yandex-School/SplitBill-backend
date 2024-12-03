#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <userver/server/http/http_request.hpp>

#include "../../../models/session.hpp"

namespace split_bill {
    struct TSession;
    struct TFilters {
        enum class ESortOrder {
            ID = 0,
            NAME = 1,
            PRICE = 2,
            ROOM_ID = 3
        } order_by = ESortOrder::ID;

        size_t limit = 10;
        size_t page = 1;

        static TFilters Parse(const userver::server::http::HttpRequest& request);
    };

}  // namespace split_bill
