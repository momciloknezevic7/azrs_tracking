
#include "../NetworkCommon/common.h"
#include "ConnectionServer.h"

int main() {
  // localhost
  const int port = 5000;

  // Context will be shared for all participants.
  asio::io_context context;

  // Such initialization is used for accepting new connections.
  asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);

  ConnectionServer server(context, endpoint);

  // Since everything happens asynchronously we need to asign idle work
  // to prevent context from closing prior to first I/O operation.
  asio::io_context::work idle_work(context);

  context.run();

  return 0;
}