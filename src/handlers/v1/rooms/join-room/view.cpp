#include "view.hpp"

#include <fmt/format.h>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "../../../../models/room.hpp"
#include "../../../lib/auth.hpp"

namespace split_bill {

namespace {

class JoinRoom final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-join-room";

  JoinRoom(const userver::components::ComponentConfig& config,
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

    auto session_id = std::stoi(request.GetHeader(USER_TICKET_HEADER_NAME));
    auto user_session = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT user_id FROM auth_sessions WHERE id = $1 LIMIT 1", session_id);

    int user_id = user_session.Front()[0].As<int>();
    LOG_INFO() << "User ID: " << user_id;

    const auto& id_str = request.GetPathArg("id");
    int room_id;
    try {
      room_id = std::stoi(id_str);
    } catch (const std::exception&) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Invalid room ID"})";
    }

    // Check if the room exists first
    auto room_check = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT 1 FROM rooms WHERE id = $1", room_id);

    if (room_check.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return R"({"error":"Room does not exist"})";
    }

    // Insert into user_rooms, ignoring if already exists
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO user_rooms (user_id, room_id) VALUES($1, $2) "
        "ON CONFLICT (user_id, room_id) DO NOTHING "
        "RETURNING 1",
        user_id, room_id);

    // Create a JSON value for the response
    userver::formats::json::ValueBuilder response_builder(true);

    return userver::formats::json::ToString(response_builder.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendJoinRoom(userver::components::ComponentList& component_list) {
  component_list.Append<JoinRoom>();
}

}  // namespace split_bill