#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handlers/v1/products/add-product/view.hpp"
#include "handlers/v1/products/get-product/view.hpp"
#include "handlers/v1/products/delete-product/view.hpp"
#include "handlers/v1/products/get-products/view.hpp"
#include "handlers/v1/rooms/create-room/view.hpp"
#include "handlers/v1/rooms/get-all-rooms/view.hpp"
#include "handlers/v1/rooms/get-created-rooms/view.hpp"
#include "handlers/v1/rooms/get-room/view.hpp"
#include "handlers/v1/rooms/update-room/view.hpp"
#include "handlers/v1/rooms/join-room/view.hpp"
#include "handlers/v1/register/view.hpp"
#include "handlers/v1/login/view.hpp"

int main(int argc, char* argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<userver::server::handlers::Ping>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::components::HttpClient>()
          .Append<userver::server::handlers::TestsControl>()
          .Append<userver::components::Postgres>("postgres-db-1")
          .Append<userver::clients::dns::Component>();

  split_bill::AppendAddProduct(component_list);
  split_bill::AppendGetProduct(component_list);
  split_bill::AppendDeleteProduct(component_list);
  split_bill::AppendGetProducts(component_list);
  split_bill::AppendRegisterUser(component_list);
  split_bill::AppendLoginUser(component_list);

  //rooms components
  split_bill::AppendAddRoom(component_list);
  split_bill::AppendGetAllRooms(component_list);
  split_bill::AppendGetCreatedRooms(component_list);
  split_bill::AppendGetRoom(component_list);
  split_bill::AppendUpdateRoom(component_list);
  split_bill::AppendJoinRoom(component_list);

  return userver::utils::DaemonMain(argc, argv, component_list);
}
