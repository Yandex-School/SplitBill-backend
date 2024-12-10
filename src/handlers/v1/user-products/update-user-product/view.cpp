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
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kUnauthorized);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Unauthorized";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    const auto& id_str = request.GetPathArg("id");
    int user_product_id;
    try {
      user_product_id = std::stoi(id_str);
    } catch (const std::exception&) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Invalid user product ID";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto owner_id = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        "SELECT r.user_id "
        "FROM rooms r "
        "JOIN products p ON r.id = p.room_id "
        "JOIN user_products up ON p.id = up.product_id "
        "WHERE up.id = $1",
        user_product_id);

    if(session->user_id != owner_id[0]["user_id"].As<int>()){
      request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
      userver::formats::json::ValueBuilder response;
      response["error"] = "User is not an owner of the Room!";
      return userver::formats::json::ToString(response.ExtractValue());
    }
    auto request_body =
        userver::formats::json::FromString(request.RequestBody());

    auto status = request_body["status"].As<std::optional<std::string>>();
    if (!status) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Missing or invalid 'status' field";
      return userver::formats::json::ToString(response.ExtractValue());
    }else if(status != "PAID" && status != "UNPAID"){
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Status is not valid!(PAID | UNPAID)";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        "UPDATE user_products SET status = $1 "
        "WHERE id = $2 "
        "RETURNING id, status, product_id, user_id",
        *status, user_product_id);

    if (result.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      userver::formats::json::ValueBuilder response;
      response["error"] = "User product not found";
      return userver::formats::json::ToString(response.ExtractValue());
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
