#include "room.hpp"

namespace split_bill {

userver::formats::json::Value Serialize(const TRoom& product,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder item;
  item["id"] = product.id;
  item["name"] = product.name;
  item["user_id"] = product.user_id;
  return item.ExtractValue();
}

}  // namespace split_bill