#include "view.hpp"

#include <fmt/format.h>
#include <unordered_map>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include "../../../../models/room.hpp"
#include "../filters.hpp"
#include "../../../lib/auth.hpp"

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
    auto session = GetSessionInfo(pg_cluster_, request);
    if (!session) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
      return R"({"error":"Unauthorized"})";
    }

    auto filters = TFilters::Parse(request);

    static const std::unordered_map<TFilters::ESortOrder, std::string> order_by_columns{
        {TFilters::ESortOrder::ID, "r.id"},
        {TFilters::ESortOrder::NAME, "r.name"},
        {TFilters::ESortOrder::USER_ID, "r.user_id"},
    };
    auto order_by_column = order_by_columns.at(filters.order_by);

    auto session_id = std::stoi(request.GetHeader(USER_TICKET_HEADER_NAME));
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT user_id FROM auth_sessions WHERE id = $1 LIMIT 1", session_id);

    int user_id = result.Front()[0].As<int>();
    LOG_INFO() << "User ID: " << user_id;

    size_t offset = (filters.page - 1) * filters.limit;

    try {
      auto count_result = pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "SELECT COUNT(DISTINCT r.id) FROM rooms r "
          "JOIN user_rooms ur ON r.id = ur.room_id "
          "WHERE ur.user_id = $1",
          user_id
      );
      int total_count = count_result.Front()[0].As<int>();

      auto rooms_result = pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          fmt::format(
              "SELECT DISTINCT r.* FROM rooms r "
              "JOIN user_rooms ur ON r.id = ur.room_id "
              "WHERE ur.user_id = $1 "
              "ORDER BY {} "
              "LIMIT $2 OFFSET $3",
              order_by_column
              ),
          user_id,
          static_cast<int>(filters.limit),
          static_cast<int>(offset)
      );

      userver::formats::json::ValueBuilder response;
      response["items"].Resize(0);
      for (const auto& row : rooms_result.AsSetOf<TRoom>(userver::storages::postgres::kRowTag)) {
        response["items"].PushBack(userver::formats::json::ValueBuilder(row));
      }

      response["page"] = filters.page;
      response["limit"] = filters.limit;
      response["total_count"] = total_count;
      response["total_pages"] = (total_count + filters.limit - 1) / filters.limit;

      return userver::formats::json::ToString(response.ExtractValue());
    } catch (const std::exception& e) {
      LOG_ERROR() << "Failed to retrieve rooms: " << e.what();
      request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
      return R"({"error":"Internal server error."})";
    }
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendGetAllRooms(userver::components::ComponentList& component_list) {
  component_list.Append<GetRooms>();
}

}  // namespace split_bill