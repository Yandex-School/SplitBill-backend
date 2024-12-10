#include "view.hpp"

#include <fmt/format.h>
#include <unordered_map>

#include <userver/components/component_context.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include "../../../../models/room.hpp"
#include "../../../lib/auth.hpp"
#include "../filters.hpp"

namespace split_bill {

namespace {

class GetRooms final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-get-all-rooms";

  GetRooms(const userver::components::ComponentConfig& config,
           const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("postgres-db-1")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kApplicationJson);
    auto session = GetSessionInfo(pg_cluster_, request);
    if (!session) {
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Unauthorized";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto filters = TFilters::Parse(request);

    static const std::unordered_map<TFilters::ESortOrder, std::string>
        order_by_columns{
            {TFilters::ESortOrder::ID, "r.id"},
            {TFilters::ESortOrder::NAME, "r.name"},
            {TFilters::ESortOrder::USER_ID, "r.user_id"},
        };
    auto order_by_column = order_by_columns.at(filters.order_by);

    size_t offset = (filters.page - 1) * filters.limit;

    auto count_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        "SELECT COUNT(DISTINCT r.id) FROM rooms r "
        "JOIN user_rooms ur ON r.id = ur.room_id "
        "WHERE ur.user_id = $1",
        session->user_id);
    auto total_count = count_result.AsSingleRow<int>();

    auto rooms_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        fmt::format("SELECT DISTINCT r.* FROM rooms r "
                    "JOIN user_rooms ur ON r.id = ur.room_id "
                    "WHERE ur.user_id = $1 "
                    "ORDER BY {} "
                    "LIMIT $2 OFFSET $3",
                    order_by_column),
        session->user_id, static_cast<int>(filters.limit),
        static_cast<int>(offset));

    auto rooms = rooms_result.AsContainer<std::vector<TRoom>>(userver::storages::postgres::kRowTag);
    userver::formats::json::ValueBuilder response;

    response["items"] = rooms;
    response["page"] = filters.page;
    response["limit"] = filters.limit;
    response["total_count"] = total_count;
    response["total_pages"] = (total_count + filters.limit - 1) / filters.limit;

    return userver::formats::json::ToString(response.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendGetAllRooms(userver::components::ComponentList& component_list) {
  component_list.Append<GetRooms>(GetRooms::kName);
}

}  // namespace split_bill