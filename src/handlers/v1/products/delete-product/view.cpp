#include "view.hpp"

#include <fmt/format.h>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/assert.hpp>

#include "../../../lib/auth.hpp"

namespace split_bill {

namespace {

class DeleteProduct final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-v1-delete-product";

    DeleteProduct(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& component_context)
        : HttpHandlerBase(config, component_context),
          pg_cluster_(
              component_context
                  .FindComponent<userver::components::Postgres>("postgres-db-1")
                  .GetCluster()) {}

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext&) const override {
        try {
            auto session = GetSessionInfo(pg_cluster_, request);
            if (!session) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
                return R"({"error":"Unauthorized"})";
            }

            const auto& id_str = request.GetPathArg("id");
            int product_id;
            try {
                product_id = std::stoi(id_str);
            } catch (const std::exception&) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
                return R"({"error":"Invalid product ID"})";
            }

            auto result = pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "SELECT p.id FROM products p "
                "JOIN rooms r ON p.room_id = r.id "
                "WHERE p.id = $1 AND r.user_id = $2",
                product_id, session->user_id
            );

            if (result.IsEmpty()) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
                return R"({"error":"Product not found or access denied"})";
            }

            auto delete_result = pg_cluster_->Execute(
                userver::storages::postgres::ClusterHostType::kMaster,
                "DELETE FROM products WHERE id = $1",
                product_id
            );

            userver::formats::json::ValueBuilder response;
            response["id"] = product_id;
            response["status"] = "deleted";
            return userver::formats::json::ToString(response.ExtractValue());

        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to delete product: " << e.what();
            request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
            return R"({"error":"Internal server error"})";
        }
    }

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace

void AppendDeleteProduct(userver::components::ComponentList& component_list) {
    component_list.Append<DeleteProduct>();
}

}  // namespace split_bill
