#include "product.hpp"

namespace split_bill {

userver::formats::json::Value Serialize(const TProduct& product,
                                        userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder item;
    item["id"] = product.id;
    item["name"] = product.name;
    item["price"] = product.price;
    item["room_id"] = product.room_id;
    return item.ExtractValue();
}

}  // namespace split_bill