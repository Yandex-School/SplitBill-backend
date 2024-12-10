#include "view.hpp"

#include <fmt/format.h>
#include <regex>

#include <userver/components/component_context.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/formats/json.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

#include "../../../models/product.hpp"

namespace split_bill {

namespace {

class RegisterUser final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-register-user";

  RegisterUser(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("postgres-db-1")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    auto request_body =
        userver::formats::json::FromString(request.RequestBody());
    request.GetHttpResponse().SetContentType(
        userver::http::content_type::kApplicationJson);

    auto username = request_body["username"].As<std::optional<std::string>>();
    auto password = request_body["password"].As<std::optional<std::string>>();
    auto full_name = request_body["full_name"].As<std::optional<std::string>>();
    auto photo_url = request_body["photo_url"].As<std::optional<std::string>>();

    if (!username || !password) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Username and password are required.";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto hashed_password = userver::crypto::hash::Sha256(password.value());

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        "INSERT INTO users(username, password, full_name, photo_url) "
        "VALUES($1, $2, $3, $4) "
        "ON CONFLICT DO NOTHING "
        "RETURNING users.id",
        username.value(), hashed_password, full_name, photo_url);

    if (result.IsEmpty()) {
      auto check_result = pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kSlave,
          "SELECT id FROM users WHERE username = $1", username.value());

      if (!check_result.IsEmpty()) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
        userver::formats::json::ValueBuilder response;
        response["error"] = "Username already exists.";
        return userver::formats::json::ToString(response.ExtractValue());
      }
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kInternalServerError);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Failed to register user due to an unknown error.";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto user_id = result.AsSingleRow<int>();

    userver::formats::json::ValueBuilder response;
    response["id"] = user_id;

    return userver::formats::json::ToString(response.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendRegisterUser(userver::components::ComponentList& component_list) {
  component_list.Append<RegisterUser>();
}

}  // namespace split_bill
