#include "view.hpp"

#include <fmt/format.h>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "../../../../models/detailed_room.hpp"
#include "../../../../models/room.hpp"
#include "../../../../models/product.hpp"
#include "../../../../models/room.hpp"
#include "../../../../models/user-product.hpp"
#include "../../../../models/user.hpp"
#include "../../../lib/auth.hpp"

namespace split_bill {

namespace {

struct MemberCount {
  int count;
};

struct TotalPrice {
  long total;
};

class GetRoom final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-v1-get-rooms-by-id";

  GetRoom(const userver::components::ComponentConfig& config,
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

    // Fetch room details
    auto room_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, name, user_id FROM rooms WHERE id = $1 AND user_id = $2", room_id, session->user_id);


    if (room_result.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      userver::formats::json::ValueBuilder response;
      response["error"] = "Room not found";
      return userver::formats::json::ToString(response.ExtractValue());
    }

    auto room = room_result.AsSingleRow<split_bill::TRoom>(userver::storages::postgres::kRowTag);

    // Fetch room products
    auto products_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT id, name, price, room_id FROM products WHERE room_id = $1", room.id);

    std::vector<split_bill::TRoomProduct> room_products;
    long total_price = 0;

    for (const auto& row : products_result) {
      auto product = row.As<split_bill::TProduct>(userver::storages::postgres::kRowTag);
      total_price += product.price;

      // Fetch user products for this product
      auto user_products_result = pg_cluster_->Execute(
          userver::storages::postgres::ClusterHostType::kMaster,
          "SELECT id, status, product_id, user_id FROM user_products WHERE product_id = $1", product.id);

      std::vector<split_bill::TUserProduct> user_products;
      for (const auto& user_row : user_products_result) {
        user_products.push_back(user_row.As<split_bill::TUserProduct>(userver::storages::postgres::kRowTag));
      }

      room_products.push_back({product.id, product.name, product.price, product.room_id, std::move(user_products)});
    }

    // Fetch total members
    auto members_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT COUNT(*) as count FROM user_rooms WHERE room_id = $1", room.id);

    int total_members = members_result.AsSingleRow<MemberCount>(userver::storages::postgres::kRowTag).count;

//    auto price_result = pg_cluster_->Execute(
//        userver::storages::postgres::ClusterHostType::kMaster,
//        "SELECT COALESCE(SUM(price), 0) as total FROM products WHERE room_id = $1", room.id);
//
//    long total_price2 = price_result.AsSingleRow<TotalPrice>(userver::storages::postgres::kRowTag).total;

    split_bill::TRoomDetails room_details{room.id, room.name, room.user_id, std::move(room_products), total_price, total_members};

    return userver::formats::json::ToString(
        userver::formats::json::ValueBuilder{room_details}.ExtractValue());
  }


 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendGetRoom(userver::components::ComponentList& component_list) {
  component_list.Append<GetRoom>();
}

}  // namespace split_bill
