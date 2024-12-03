#include "view.hpp"

#include <fmt/format.h>
#include <unordered_map>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include "../../../../models/user-product.hpp"

#include "../../../lib/auth.hpp"
#include "../filters.hpp"

namespace split_bill {

namespace {

class GetUserProduct final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-get-user-products";

  GetUserProduct(const userver::components::ComponentConfig& config,
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
    int user_id = std::stoi(request.GetPathArg("id"));
    const std::string query = fmt::format(
        R"(
          SELECT
            up.product_id AS product_id
          FROM user_products up
          JOIN products p ON up.product_id = p.id
          WHERE up.user_id = {user_id}
          )",
        fmt::arg("user_id", user_id));

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster, query);
    if (result.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return R"({"error":"User Id does not exist or not linked any products!"})";
    }

    userver::formats::json::ValueBuilder response;
    response["products"].Resize(0);

    for (const auto& row : result) {
      int product_id = row["product_id"].As<int>();
      response["products"].PushBack(product_id);
    }

    return userver::formats::json::ToString(response.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};
}  // namespace

void AppendGetUserProduct(userver::components::ComponentList& component_list) {
  component_list.Append<GetUserProduct>();
}

}  // namespace split_bill
