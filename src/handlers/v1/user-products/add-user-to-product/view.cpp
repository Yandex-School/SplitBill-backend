#include "view.hpp"

#include <fmt/format.h>
#include <regex>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

#include "../../../../models/user-product.hpp"
#include "../../../lib/auth.hpp"

namespace split_bill {

namespace {

class AddUserToProduct final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-add-user-to-product";

  AddUserToProduct(
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
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Unauthorized";
      return userver::formats::json::ToString(response.ExtractValue());
    }
    LOG_INFO() << "Request Body: " << request.RequestBody();

    auto request_body =
        userver::formats::json::FromString(request.RequestBody());
    auto status = request_body["status"].As<std::optional<std::string>>();
    auto product_id = request_body["product_id"].As<std::optional<int>>();
    auto user_id = request_body["user_id"].As<std::optional<int>>();
    std::string status_str;
    if (!status) {
      status_str = "UNPAID";
    } else {
      status_str = *status;
    }
    if(status_str != "PAID" && status_str != "UNPAID"){
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Status is not valid!(PAID | UNPAID)";
      return userver::formats::json::ToString(response.ExtractValue());
    }
    if (!product_id || !user_id) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      userver::formats::json::ValueBuilder response;
      response["error"] = "'product_id' and 'user_id' fields are required";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    LOG_INFO() << "Executing query with status: " << status_str
               << ", product_id: " << *product_id << ", user_id: " << *user_id;

    auto check_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT 1 FROM user_products WHERE product_id = $1 AND user_id = $2 "
        "LIMIT 1",
        *product_id, *user_id);

    if (!check_result.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      userver::formats::json::ValueBuilder response;
      response["error"] = "User already associated with this product";
      return userver::formats::json::ToString(response.ExtractValue());
    }
    auto check_product_id = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT 1 FROM products WHERE id = $1", *product_id);
    if (check_product_id.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Product Does not exist";
      return userver::formats::json::ToString(response.ExtractValue());
    }
    auto check_user_id = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT 1 FROM users WHERE id = $1", *user_id);
    if (check_user_id.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      userver::formats::json::ValueBuilder response;
      response["error"] = "User Does not exist!";
      return userver::formats::json::ToString(response.ExtractValue());
    }
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO user_products (status, product_id, user_id) VALUES($1, "
        "$2, $3) "
        "ON CONFLICT DO NOTHING "
        "RETURNING id, status, product_id, user_id",
        status_str, *product_id, *user_id);

    if (!result.IsEmpty()) {
      auto user_product = result.AsSingleRow<TUserProduct>(
          userver::storages::postgres::kRowTag);
      return ToString(
          userver::formats::json::ValueBuilder{user_product}.ExtractValue());
    } else {
      request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
      userver::formats::json::ValueBuilder response;
      response["error"] = "User already associated with this product";
      return userver::formats::json::ToString(response.ExtractValue());
    }
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendAddUserToProduct(
    userver::components::ComponentList& component_list) {
  component_list.Append<AddUserToProduct>();
}

}  // namespace split_bill
