#pragma once

#include <userver/server/http/http_request.hpp>
#include <userver/storages/postgres/cluster.hpp>

#include "../../models/session.hpp"

namespace split_bill {

const std::string USER_TICKET_HEADER_NAME = "X-Ya-User-Ticket";

std::optional<TSession> GetSessionInfo(
    userver::storages::postgres::ClusterPtr pg_cluster,
    const userver::server::http::HttpRequest& request
);

}  // namespace split_bill