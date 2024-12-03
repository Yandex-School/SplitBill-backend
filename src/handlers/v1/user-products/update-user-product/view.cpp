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

class UpdateUserProduct final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-update-user-product";

  UpdateUserProduct(
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
    int user_product_id = std::stoi(request.GetPathArg("id"));

    auto request_body =
        userver::formats::json::FromString(request.RequestBody());

    auto status = request_body["status"].As<std::optional<std::string>>();
    if (!status) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Missing or invalid 'status' field"})";
    }

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "UPDATE user_products SET status = $1 "
        "WHERE id = $2 "
        "RETURNING id, status, product_id, user_id",
        *status, user_product_id);

    if (result.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return R"({"error":"User product not found"})";
    }
    auto updated_user_product =
        result.AsSingleRow<TUserProduct>(userver::storages::postgres::kRowTag);
    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{updated_user_product}
            .ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};
}  // namespace

void AppendUpdateUserProduct(
    userver::components::ComponentList& component_list) {
  component_list.Append<UpdateUserProduct>();
}

}  // namespace split_bill
