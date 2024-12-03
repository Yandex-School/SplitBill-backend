#include "view.hpp"

#include <fmt/format.h>
#include <unordered_map>

#include <userver/components/component_context.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include "../../../../models/user-product.hpp"

#include "../../../lib/auth.hpp"
#include "../filters.hpp"

namespace split_bill {

namespace {

class GetUserProducts final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-get-all-user-products";

  GetUserProducts(
      const userver::components::ComponentConfig& config,
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
      auto& response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kUnauthorized);
      return {};
    }

    if (!request.HasArg("room_id")) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Missing room_id query parameter."})";
    }
    int room_id = std::stoi(request.GetArg("room_id"));

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT u.id AS user_id, "
        "ARRAY_AGG(up.product_id) AS product_ids "
        "FROM users u "
        "JOIN user_products up ON u.id = up.user_id "
        "JOIN products p ON up.product_id = p.id "
        "WHERE p.room_id = $1 "
        "GROUP BY u.id;",
        room_id);
    if (result.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return R"({"error":"Room Id does not exist or does not have any products"})";
    }
    userver::formats::json::ValueBuilder response;
    response["users"].Resize(0);
    for (const auto& row : result) {
      userver::formats::json::ValueBuilder user_entry;
      user_entry["user_id"] = row["user_id"].As<int>();
      user_entry["product_ids"] = row["product_ids"].As<std::vector<int>>();
      response["users"].PushBack(std::move(user_entry));
    }
    return userver::formats::json::ToString(response.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};
}  // namespace

void AppendGetUserProducts(userver::components::ComponentList& component_list) {
  component_list.Append<GetUserProducts>();
}

}  // namespace split_bill
