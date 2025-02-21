#pragma once

#include <userver/server/http/http_request.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/http/content_type.hpp>

#include "../../models/session.hpp"

namespace split_bill {

const std::string USER_TICKET_HEADER_NAME = "x-ya-user-ticket";

std::optional<TSession> GetSessionInfo(
    userver::storages::postgres::ClusterPtr pg_cluster,
    const userver::server::http::HttpRequest& request
);

}  // namespace split_bill
