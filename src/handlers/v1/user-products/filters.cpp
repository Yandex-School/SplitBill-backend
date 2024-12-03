#include "filters.hpp"

#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
namespace split_bill {

TFilters Parse(const userver::server::http::HttpRequest& request) {
  TFilters result;

  if (request.HasPathArg("id")) {
    try {
      result.room_id = std::stoul(request.GetPathArg("id"));
    } catch (const std::exception&) {
      throw std::invalid_argument("Invalid roomId provided in the path.");
    }
  }

  return result;
}
}  // namespace split_bill
