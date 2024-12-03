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



userver::formats::json::Value Serialize(
    const TUserProduct& data,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace split_bill