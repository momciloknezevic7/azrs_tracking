#ifndef CONNECTION_SERVER_H
#define CONNECTION_SERVER_H

#include <map>

#include "../NetworkCommon/common.h"
#include "ConnectionRoom.h"
#include "ConnectionSession.h"

class ConnectionServer {
public:
  ConnectionServer(asio::io_context &context,
                   const asio::ip::tcp::endpoint &endpoint);

private:
  // Accepting new participant. Current implementation allows two players.
  void accept();

  asio::ip::tcp::acceptor _acceptor;
  ConnectionRoom _waiting_room;
  std::map<game_t, room_ptr> _active_rooms;
};

#endif