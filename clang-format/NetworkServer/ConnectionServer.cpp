#include "ConnectionServer.h"

ConnectionServer::ConnectionServer(asio::io_context &context,
                                   const asio::ip::tcp::endpoint &endpoint)
    : _acceptor(context, endpoint) {
  accept();
}

void ConnectionServer::accept() {
  _acceptor.async_accept(
      [this](const asio::error_code &ec, asio::ip::tcp::socket socket) {
        if (!ec) {
          std::make_shared<ConnectionSession>(std::move(socket), _waiting_room,
                                              _active_rooms)
              ->start();
        }

        accept();
      });
}