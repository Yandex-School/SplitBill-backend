#pragma once

#include <string>
#include <vector>
#include <optional>
#include "user-product.hpp"
#include "product.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/parse/common_containers.hpp>

namespace split_bill {

// Forward declarations
struct TRoomProduct;
struct TRoomDetails;

userver::formats::json::Value Serialize(
    const TRoomProduct& data,
    userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value Serialize(
    const TRoomDetails& data,
    userver::formats::serialize::To<userver::formats::json::Value>);

struct TRoomProduct {
  int id;
  std::string name;
  long price;
  int room_id;
  std::vector<split_bill::TUserProductWithDetails> user_products;

  TRoomProduct(int id, const std::string& name, long price, int room_id, std::vector<split_bill::TUserProductWithDetails>&& user_products)
      : id(id), name(name), price(price), room_id(room_id), user_products(std::move(user_products)) {}
};


struct TRoomDetails {
  int id;
  std::string name;
  int owner_id;
  std::vector<TRoomProduct> room_products;
  long total_price;
  int total_members;
};

struct TUserProductTransaction {
  std::string action;
  std::optional<int> id;
  std::optional<std::string> name;
  std::optional<long> price;
  std::optional<std::string> status;
  std::vector<int> user_ids;
};

}  // namespace split_bill