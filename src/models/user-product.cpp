#include "user-product.hpp"
#include <stdexcept>

namespace split_bill {

userver::formats::json::Value Serialize(
    const TUserProduct& user_product,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder item;
  item["id"] = user_product.id;
  item["status"] = user_product.status;
  item["product_id"] = user_product.product_id;
  item["user_id"] = user_product.user_id;
  return item.ExtractValue();
}

}  // namespace split_bill