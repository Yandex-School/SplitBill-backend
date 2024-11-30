#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_list.hpp>

namespace split_bill {

void AppendRegisterUser(userver::components::ComponentList& component_list);

}  // namespace split_bill
