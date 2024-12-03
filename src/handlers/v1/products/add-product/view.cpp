#include "view.hpp"

#include <fmt/format.h>
#include <regex>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

#include "../../../lib/auth.hpp"
#include "../../../../models/product.hpp"

namespace split_bill {

namespace {

class AddProduct final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-v1-add-product";

    AddProduct(const userver::components::ComponentConfig& config,
               const userver::components::ComponentContext& component_context)
        : HttpHandlerBase(config, component_context),
          pg_cluster_(
              component_context
                  .FindComponent<userver::components::Postgres>("postgres-db-1")
                  .GetCluster()) {}

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override {
        request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);
        auto session = GetSessionInfo(pg_cluster_, request);
        if (!session) {
            auto& response = request.GetHttpResponse();
            response.SetStatus(userver::server::http::HttpStatus::kUnauthorized);
            return {};
        }

        auto request_body = userver::formats::json::FromString(request.RequestBody());
        auto name = request_body["name"].As<std::optional<std::string>>();
        auto price = request_body["price"].As<std::optional<int64_t>>();
        auto room_id = request_body["room_id"].As<std::optional<int>>();

        if (!name || !price || !room_id) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return R"({"error":"'name', 'price', and 'room_id' fields are required."})";
        }

        LOG_INFO() << "Adding product: " << *name << " " << *price << " " << *room_id;

        try {
            auto result = pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "INSERT INTO products (name, price, room_id) VALUES($1, $2, $3) "
                "ON CONFLICT (name, room_id) DO NOTHING "
                "RETURNING id, name, price, room_id",
                *name, *price, *room_id
            );

            if (!result.IsEmpty()) {
                auto product = result.AsSingleRow<TProduct>(userver::storages::postgres::kRowTag);
                return ToString(userver::formats::json::ValueBuilder{product}.ExtractValue());
            } else {
                request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
                return R"({"error":"Product already exists."})";
            }
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to add product: " << e.what();
            request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
            return R"({"error":"Internal server error."})";
        }
    }

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendAddProduct(userver::components::ComponentList& component_list) {
    component_list.Append<AddProduct>();
}

}  // namespace split_bill
