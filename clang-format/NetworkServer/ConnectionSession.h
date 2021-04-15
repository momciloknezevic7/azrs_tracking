#ifndef CONNECTIONSESSION_H
#define CONNECTIONSESSION_H

#include <map>
#include <utility>

#include "../GameLogic/Human_player.h"
#include "../NetworkCommon/Message.h"
#include "../NetworkCommon/ThreadSafeQueue.h"
#include "../NetworkCommon/common.h"
#include "ConnectionParticipant.h"
#include "ConnectionRoom.h"

using asio::ip::tcp;

// Responsible for passing requests to the ChatRoom that are received from
// participants via network.
class ConnectionSession
    : public ConnectionParticipant,
      public std::enable_shared_from_this<ConnectionSession> {
public:
  ConnectionSession(tcp::socket socket, ConnectionRoom &room,
                    std::map<game_t, room_ptr> &active_rooms);

  void start();
  void deliver(Message &msg);

private:
  void parse_client_create_game();
  void parse_client_join_game();
  void parse_client_chat();
  void parse_client_intermediate_move();
  void parse_client_finish_move();
  void parse_client_announcement();
  void parse_client_surrender();
  void parse_message();

  void leave();

  void read_header();
  void read_body();
  void write_header();
  void write_body();

  void determine_winner();

  // ASIO buffers point to data which don't have local scope. For that purpose
  // smart pointers are used for easier manipulation with data on heap. These
  // poiners serve purpose of placeholders.
  std::unique_ptr<std::vector<uint8_t>> _series_ptr_write;
  std::unique_ptr<std::vector<uint8_t>> _series_ptr_read;

  tcp::socket _socket;
  ConnectionRoom &_room;
  std::map<game_t, room_ptr> &_active_rooms;

  Header _store_header;
  Message _store_message;
  ThreadSafeQueue<Message> _write_msgs;
};

#endif