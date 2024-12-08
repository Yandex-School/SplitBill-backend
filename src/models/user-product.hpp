#pragma once

#include <string>
#include <userver/formats/json/value_builder.hpp>
namespace split_bill {

struct TUserProduct {
  int id;
  std::string status;
  int product_id;
  int user_id;
};

struct TUserProductWithDetails {
  int id;
  std::string status;
  int product_id;
  int user_id;
  std::optional<std::string> full_name;
  std::optional<std::string> photo_url;
};

userver::formats::json::Value Serialize(
    const TUserProduct& data,
    userver::formats::serialize::To<userver::formats::json::Value>);

userver::formats::json::Value Serialize(
    const TUserProductWithDetails& data,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace split_bill