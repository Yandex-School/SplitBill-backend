#include "detailed-room.hpp"
#include <userver/formats/json/value_builder.hpp>

namespace split_bill {

userver::formats::json::Value Serialize(
    const TRoomProduct& room_product,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder json;
  json["id"] = room_product.id;
  json["name"] = room_product.name;
  json["price"] = room_product.price;
  json["room_id"] = room_product.room_id;
  json["user_products"] = room_product.user_products;
  return json.ExtractValue();
}

userver::formats::json::Value Serialize(
    const TRoomDetails& room_details,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder json;
  json["id"] = room_details.id;
  json["name"] = room_details.name;
  json["owner_id"] = room_details.owner_id;
  json["room_products"] = room_details.room_products;
  json["room_status"] = room_details.status;
  json["total_price"] = room_details.total_price;
  json["total_members"] = room_details.total_members;
  return json.ExtractValue();
}

userver::formats::json::Value Serialize(
    const TUserProductTransaction& data,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace split_bill
