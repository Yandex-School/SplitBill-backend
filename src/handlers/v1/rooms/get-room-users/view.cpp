#include "view.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include "../../../lib/auth.hpp"

namespace split_bill {

namespace {

class GetRoomUsers final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-get-room-users";

  GetRoomUsers(const userver::components::ComponentConfig& config,
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
      return userver::formats::json::ToString(
          userver::formats::json::ValueBuilder{{"error", "Unauthorized"}}
              .ExtractValue());
    }

    // Validate and parse room ID
    const auto& id_str = request.GetPathArg("id");
    int room_id;
    try {
      room_id = std::stoi(id_str);
    } catch (const std::exception&) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Invalid room ID";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    // Query the database for users in the specified room
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        "SELECT u.id, u.username, u.full_name, u.photo_url "
        "FROM users u "
        "INNER JOIN user_rooms ur ON u.id = ur.user_id "
        "WHERE ur.room_id = $1", room_id);

    userver::formats::json::ValueBuilder response;
    response["room_id"] = room_id;

    response["users"] = userver::formats::json::ValueBuilder(userver::formats::json::Type::kArray);

    for (const auto& row : result) {
      userver::formats::json::ValueBuilder user;
      user["id"] = row["id"].As<int>();
      user["username"] = row["username"].As<std::string>();

      if (row["full_name"].As<std::optional<std::string>>()) {
        user["full_name"] = *row["full_name"].As<std::optional<std::string>>();
      } else {
        user["full_name"] = nullptr;
      }

      if (row["photo_url"].As<std::optional<std::string>>()) {
        user["photo_url"] = *row["photo_url"].As<std::optional<std::string>>();
      } else {
        user["photo_url"] = nullptr;
      }
      response["users"].PushBack(std::move(user));
    }



    return userver::formats::json::ToString(response.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendGetRoomUsers(userver::components::ComponentList& component_list) {
  component_list.Append<GetRoomUsers>();
}

}  // namespace split_bill
