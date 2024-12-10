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

    auto room_user_id = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave,
        "SELECT user_id FROM rooms WHERE id = $1", room_id);

    if (room_user_id.AsSingleRow<int>() != session->user_id) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
      userver::formats::json::ValueBuilder response;
      response["error"] = "You can't update the room";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto transaction = pg_cluster_->Begin(
        userver::storages::postgres::TransactionOptions(),
        userver::storages::postgres::OptionalCommandControl{});

    if (request_body.HasMember("room") && !request_body["room"].IsNull()) {
      const auto& room_data = request_body["room"];
      if (room_data.HasMember("name") && !room_data["name"].IsNull()) {
        const auto& name = room_data["name"].As<std::string>();
        transaction.Execute(
            "UPDATE rooms SET name = $1 WHERE id = $2 AND user_id = $3", name,
            room_id, session->user_id);
      }
    }

    if (request_body.HasMember("product") && !request_body["product"].IsNull()) {
      const auto& product_data = request_body["product"];

      if (product_data.HasMember("add") && !product_data["add"].IsNull()) {
        // Bulk insert products
        std::vector<std::string> product_values;
        std::vector<int> product_prices;
        std::vector<int> product_room_ids;

        std::vector<std::pair<int, int>> user_product_mappings;

        for (const auto& product : product_data["add"]) {
          auto name = product["name"].As<std::string>();
          auto price = product["price"].As<int>();
          auto user_ids = product["add_users"].As<std::vector<int>>();

          product_values.push_back(name);
          product_prices.push_back(price);
          product_room_ids.push_back(room_id);

          // Prepare user_product mappings
          for (int user_id : user_ids) {
            user_product_mappings.emplace_back(0, user_id);  // 0 will be replaced with actual product_id
          }
        }

        // Single bulk insert for products
        auto product_result = transaction.Execute(
            "INSERT INTO products (name, price, room_id) "
            "VALUES (unnest($1::text[]), unnest($2::int[]), unnest($3::int[])) "
            "RETURNING id",
            product_values, product_prices, product_room_ids);

        // Get the inserted product IDs manually
        std::vector<int> product_ids;
        for (const auto& row : product_result) {
          product_ids.push_back(row["id"].As<int>());
        }

        // Update user_product mappings with actual product IDs
        std::vector<std::pair<int, int>> final_user_product_mappings;
        size_t product_index = 0;
        for (const auto& mapping : user_product_mappings) {
          if (mapping.first == 0) {
            final_user_product_mappings.emplace_back(
                product_ids[product_index / mapping.second],
                mapping.second
            );

            if ((product_index + 1) % mapping.second == 0) {
              product_index++;
            }
          }
        }

        // Bulk insert user_products
        if (!final_user_product_mappings.empty()) {
          std::vector<int> product_ids_to_insert;
          std::vector<int> user_ids_to_insert;

          for (const auto& mapping : final_user_product_mappings) {
            product_ids_to_insert.push_back(mapping.first);
            user_ids_to_insert.push_back(mapping.second);
          }

          transaction.Execute(
              "INSERT INTO user_products (product_id, user_id) "
              "VALUES (unnest($1::int[]), unnest($2::int[]))",
              product_ids_to_insert,
              user_ids_to_insert
          );
        }
      }

      // Replace the existing edit section with this bulk update approach
      if (product_data.HasMember("edit") && !product_data["edit"].IsNull()) {
        // Prepare vectors for bulk operations
        std::vector<int> name_update_ids;
        std::vector<std::string> name_update_values;
        std::vector<int> price_update_ids;
        std::vector<int> price_update_values;
        std::vector<int> status_update_product_ids;
        std::vector<std::string> status_update_values;

        // Prepare vectors for user-product deletions
        std::vector<int> delete_product_ids;
        std::vector<int> delete_user_ids;

        for (const auto& product : product_data["edit"]) {
          int product_id = product["id"].As<int>();
          auto name = product["name"].As<std::optional<std::string>>();
          auto price = product["price"].As<std::optional<int>>();
          auto status = product["status"].As<std::optional<std::string>>();
          auto delete_users = product["delete_users"].As<std::vector<int>>();

          // Collect name updates
          if (name) {
            name_update_ids.push_back(product_id);
            name_update_values.push_back(name.value());
          }

          // Collect price updates
          if (price) {
            price_update_ids.push_back(product_id);
            price_update_values.push_back(price.value());
          }

          // Collect status updates
          if (status) {
            status_update_product_ids.push_back(product_id);
            status_update_values.push_back(status.value());
          }

          // Collect user-product deletions
          for (int user_id : delete_users) {
            delete_product_ids.push_back(product_id);
            delete_user_ids.push_back(user_id);
          }
        }

        // Bulk update names
        if (!name_update_ids.empty()) {
          transaction.Execute(
              "UPDATE products AS p "
              "SET name = u.name "
              "FROM (SELECT unnest($1::int[]) AS id, unnest($2::text[]) AS name) AS u "
              "WHERE p.id = u.id",
              name_update_ids,
              name_update_values
          );
        }

        // Bulk update prices
        if (!price_update_ids.empty()) {
          transaction.Execute(
              "UPDATE products AS p "
              "SET price = u.price "
              "FROM (SELECT unnest($1::int[]) AS id, unnest($2::int[]) AS price) AS u "
              "WHERE p.id = u.id",
              price_update_ids,
              price_update_values
          );
        }

        // Bulk update statuses in user_products
        if (!status_update_product_ids.empty()) {
          transaction.Execute(
              "UPDATE user_products AS up "
              "SET status = u.status "
              "FROM (SELECT unnest($1::int[]) AS product_id, unnest($2::text[]) AS status) AS u "
              "WHERE up.product_id = u.product_id",
              status_update_product_ids,
              status_update_values
          );
        }

        // Bulk delete user-product associations
        if (!delete_product_ids.empty()) {
          transaction.Execute(
              "DELETE FROM user_products AS up "
              "WHERE (up.product_id, up.user_id) IN "
              "(SELECT unnest($1::int[]), unnest($2::int[]))",
              delete_product_ids,
              delete_user_ids
          );
        }
      }

      if (product_data.HasMember("remove") && !product_data["remove"].IsNull()) {
        // Batch deletion of products
        std::vector<int> product_ids_to_remove;
        for (const auto& product : product_data["remove"]) {
          product_ids_to_remove.push_back(product["id"].As<int>());
        }

        transaction.Execute(
            "DELETE FROM products WHERE id = ANY($1::int[])",
            product_ids_to_remove
        );
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
