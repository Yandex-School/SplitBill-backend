#include "view.hpp"

#include <fmt/format.h>

#include <userver/components/component_context.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/formats/json.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

#include "../../../handlers/lib/jwt.h"
#include "../../../models/user.hpp"
#include "../../../models/session.hpp"

namespace split_bill {

namespace {

class LoginUser final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-login-user";

  LoginUser(const userver::components::ComponentConfig& config,
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
    auto request_body =
        userver::formats::json::FromString(request.RequestBody());

    auto username = request_body["username"].As<std::optional<std::string>>();
    auto password = request_body["password"].As<std::optional<std::string>>();

    if (!username || !password) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      return R"({"error":"Username and password are required."})";
    }

    auto userResult = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT * FROM users "
        "WHERE username = $1 ",
        username.value());

    if (userResult.IsEmpty()) {
      auto& response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kNotFound);
      return {};
    }

    auto hashed_password = userver::crypto::hash::Sha256(password.value());

    auto user =
        userResult.AsSingleRow<TUser>(userver::storages::postgres::kRowTag);

    if (hashed_password != user.password) {
      auto& response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kNotFound);
      return {};
    }

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO auth_sessions(user_id) VALUES($1) "
        "ON CONFLICT DO NOTHING "
        "RETURNING auth_sessions.id, auth_sessions.user_id",
        user.id);

    auto tSession =
        result.AsSingleRow<TSession>(userver::storages::postgres::kRowTag);

    std::string token = JwtUtils::createToken(std::to_string(tSession.id), std::to_string(tSession.user_id));

    pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "UPDATE auth_sessions SET token = $1 WHERE id = $2",
        token, tSession.id);

    userver::formats::json::ValueBuilder response;
    response["token"] = token;

    return userver::formats::json::ToString(response.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendLoginUser(userver::components::ComponentList& component_list) {
  component_list.Append<LoginUser>();
}

}  // namespace split_bill
