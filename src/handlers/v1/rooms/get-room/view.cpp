#include "view.hpp"

#include <fmt/format.h>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

#include "../../../../handlers/lib/jwt.h"
#include "../../../../models/room.hpp"
#include "../../../lib/auth.hpp"

namespace split_bill {

namespace {

class GetRoom final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-get-rooms-by-id";

  GetRoom(const userver::components::ComponentConfig& config,
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

    // Extract and verify token
    auto session_opt = JwtUtils::GetToken(request);
    if (!session_opt) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Unauthorized";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    const auto& session = session_opt.value();

    // Parse room ID from path
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

    // Query database for the room
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT * FROM rooms WHERE id = $1 AND user_id = $2", room_id,
        session.user_id);

    if (result.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Room not found";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto room = result.AsSingleRow<TRoom>(userver::storages::postgres::kRowTag);

    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{room}.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendGetRoom(userver::components::ComponentList& component_list) {
  component_list.Append<GetRoom>();
}

}  // namespace split_bill