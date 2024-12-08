#include "view.hpp"

#include <fmt/format.h>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

#include "../../../../models/room.hpp"
#include "../../../../models/user-price.hpp"
#include "../../../lib/auth.hpp"

namespace split_bill {

namespace {

class GetRoomUserPrices final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-get-room-user-prices";

  GetRoomUserPrices(const userver::components::ComponentConfig& config,
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
    int room_id;
    try {
      room_id = std::stoi(id_str);
    } catch (const std::exception&) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Invalid room ID";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT u.id AS user_id, u.full_name, u.photo_url, "
        "CAST(SUM(p.price) OVER (PARTITION BY u.id) AS INT) AS amount, "
        "up.product_id, p.name AS product_name, p.price, up.status "
        "FROM users u "
        "JOIN user_products up ON u.id = up.user_id "
        "JOIN products p ON up.product_id = p.id "
        "WHERE p.room_id = $1;",
        room_id);
    std::unordered_map<int, userver::formats::json::ValueBuilder> user_map;
    std::unordered_map<int, int> user_count;
    std::unordered_map<int, int> user_prices;
    for (const auto& row : result){
      user_count[row["product_id"].As<int>()]++;
    }
    for(const auto& row : result){
      user_prices[row["user_id"].As<int>()] += row["price"].As<int>() / user_count[row["product_id"].As<int>()];
    }
    for (const auto& row : result) {
      int user_id = row["user_id"].As<int>();

      if (user_map.find(user_id) == user_map.end()) {
        userver::formats::json::ValueBuilder user_entry;
        user_entry["id"] = user_id;
        user_entry["full_name"] = row["full_name"].As<std::string>();
        user_entry["photo_url"] = row["photo_url"].As<std::string>();
        user_entry["amount"] = user_prices[user_id];
        user_entry["products"].Resize(0);

        user_map[user_id] = std::move(user_entry);
      }

      userver::formats::json::ValueBuilder product_entry;
      product_entry["id"] = row["product_id"].As<int>();
      product_entry["name"] = row["product_name"].As<std::string>();
      product_entry["price"] = row["price"].As<int64_t>();
      product_entry["status"] = row["status"].As<std::string>();

      user_map[user_id]["products"].PushBack(std::move(product_entry));
    }

    // Build final response
    userver::formats::json::ValueBuilder response;
    response["data"].Resize(0);
    for (auto& [_, user_entry] : user_map) {
      response["data"].PushBack(std::move(user_entry));
    }

    return userver::formats::json::ToString(response.ExtractValue());


  }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendGetRoomUserPrices(userver::components::ComponentList& component_list) {
  component_list.Append<GetRoomUserPrices>();
}

}  // namespace split_bill
