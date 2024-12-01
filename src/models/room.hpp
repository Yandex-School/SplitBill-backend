#pragma once

#include <chrono>
#include <string>

#include <userver/formats/json/value_builder.hpp>

namespace split_bill {

struct TRoom {
    int id;
    std::string name;
    int user_id;
};

userver::formats::json::Value Serialize(const TRoom& data,
                                        userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace split_bill
