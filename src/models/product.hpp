#pragma once

#include <chrono>
#include <string>

#include <userver/formats/json/value_builder.hpp>

namespace split_bill {

struct TProduct {
    int id;
    std::string name;
    long price;
    int room_id;
};

userver::formats::json::Value Serialize(const TProduct& data,
                                        userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace split_bill
