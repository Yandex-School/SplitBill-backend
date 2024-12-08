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

    auto request_body =
        userver::formats::json::FromString(request.RequestBody());

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

    // Start a transaction
    auto transaction = pg_cluster_->Begin(
        userver::storages::postgres::TransactionOptions(),
        userver::storages::postgres::OptionalCommandControl{});

    if (request_body.HasMember("room") &&
        request_body["room"].HasMember("name")) {
      const auto& name = request_body["room"]["name"].As<std::string>();
      transaction.Execute(
          "UPDATE rooms SET name = $1 WHERE id = $2 AND user_id = $3", name,
          room_id, session->user_id);
    }

    if (request_body.HasMember("product")) {
      const auto& product_data = request_body["product"];

      // Add products
      if (product_data.HasMember("add")) {
        for (const auto& product : product_data["add"]) {
          auto name = product["name"].As<std::string>();
          auto price = product["price"].As<int>();
          auto status = product["status"].As<std::string>();
          auto user_ids = product["add_users"].As<std::vector<int>>();

          auto result = transaction.Execute(
              "INSERT INTO products (name, price, room_id) VALUES ($1, $2, $3) "
              "RETURNING id",
              name, price, room_id);

          int product_id = result.AsSingleRow<int>();

          // Add users to user_products
          for (int user_id : user_ids) {
            transaction.Execute(
                "INSERT INTO user_products (product_id, user_id, status) "
                "VALUES ($1, $2, $3)",
                product_id, user_id, status);
          }
        }
      }

      // Edit products
      if (product_data.HasMember("edit")) {
        for (const auto& product : product_data["edit"]) {
          int product_id = product["id"].As<int>();
          auto name = product["name"].As<std::optional<std::string>>();
          auto price = product["price"].As<std::optional<int>>();
          auto status = product["status"].As<std::optional<std::string>>();
          auto delete_users = product["delete_users"].As<std::vector<int>>();

          if (name) {
            transaction.Execute("UPDATE products SET name = $1 WHERE id = $2",
                                *name, product_id);
          }
          if (price) {
            transaction.Execute("UPDATE products SET price = $1 WHERE id = $2",
                                *price, product_id);
          }
          if (status) {
            transaction.Execute(
                "UPDATE user_products SET status = $1 WHERE product_id = $2",
                *status, product_id);
          }

          // Remove users from user_products
          for (int user_id : delete_users) {
            transaction.Execute(
                "DELETE FROM user_products WHERE product_id = $1 AND user_id = "
                "$2",
                product_id, user_id);
          }
        }
      }

      // Remove products
      if (product_data.HasMember("remove")) {
        for (const auto& product : product_data["remove"]) {
          int product_id = product["id"].As<int>();
          transaction.Execute("DELETE FROM products WHERE id = $1", product_id);
        }
      }
    }

    transaction.Commit();
    userver::formats::json::ValueBuilder response;
    response["status"] = "success";
    return userver::formats::json::ToString(response.ExtractValue());
  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendUpdateRoom(userver::components::ComponentList& component_list) {
  component_list.Append<UpdateRoom>();
}

}  // namespace split_bill