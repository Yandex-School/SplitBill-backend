#include "view.hpp"

#include <fmt/format.h>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include "../../../../models/room.hpp"
#include "../../../lib/auth.hpp"

namespace split_bill {

namespace {

class UpdateRoom final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-update-room";

  UpdateRoom(const userver::components::ComponentConfig& config,
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
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      return R"({"error":"Unauthorized"})";
    }

    auto request_body =
        userver::formats::json::FromString(request.RequestBody());

    int room_id = std::stoi(request.GetPathArg("id"));

    auto name = request_body["name"].As<std::optional<std::string>>();
    if (!name || name->empty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"name is required"})";
    }

    try {
      auto update_result = pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "UPDATE rooms SET name = $1 "
          "WHERE id = $2 AND user_id = $3 "
          "RETURNING id, name, user_id",
          *name, room_id, session->id);

      if (update_result.IsEmpty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
        return R"({"error":"room not found"})";
      }

      auto updated_room = update_result.AsSingleRow<TRoom>(
          userver::storages::postgres::kRowTag);
      return userver::formats::json::ToString(
          userver::formats::json::ValueBuilder{updated_room}.ExtractValue());
    } catch (const std::exception& e) {
      LOG_ERROR() << "Failed to update room: " << e.what();
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kInternalServerError);
      return R"({"error":"Failed to update room"})";
    }
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendUpdateRoom(userver::components::ComponentList& component_list) {
  component_list.Append<UpdateRoom>();
}

}  // namespace split_bill