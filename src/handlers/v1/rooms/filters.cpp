// filters.cpp

#include "filters.hpp"
#include <unordered_map>

namespace split_bill {

TFilters TFilters::Parse(const userver::server::http::HttpRequest& request) {
  TFilters result;

  // Validate and set 'limit'
  if (request.HasArg("limit")) {
    try {
      auto limit_value = std::stoul(request.GetArg("limit"));
      if (limit_value > 0 && limit_value <= 1000) {
        result.limit = limit_value;
      } else {
        // Set to default if out of range
        result.limit = 100;
      }
    } catch (const std::exception&) {
      // Set to default if parsing fails
      result.limit = 100;
    }
  }

  if (request.HasArg("page")) {
    try {
      auto page_value = std::stoul(request.GetArg("page"));
      if (page_value > 0) {
        result.page = page_value;
      } else {
        result.page = 1; // Default to first page
      }
    } catch (const std::exception&) {
      result.page = 1; // Default to first page if parsing fails
    }
  }

  // Validate and set 'order_by'
  if (request.HasArg("order_by")) {
    static const std::unordered_map<std::string, TFilters::ESortOrder> mappings{
              {"id", TFilters::ESortOrder::ID},
              {"name", TFilters::ESortOrder::NAME},
              {"user_id", TFilters::ESortOrder::USER_ID},
          };
    auto it = mappings.find(request.GetArg("order_by"));
    if (it != mappings.end()) {
      result.order_by = it->second;
    } else {
      // Default order if invalid value provided
      result.order_by = TFilters::ESortOrder::ID;
    }
  }

  return result;
}

}  // namespace split_bill
