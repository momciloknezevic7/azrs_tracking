#include "ConnectionSession.h"

#include <algorithm>

ConnectionSession::ConnectionSession(tcp::socket socket, ConnectionRoom &room,
                                     std::map<game_t, room_ptr> &active_rooms)
    : _socket(std::move(socket)), _room(room), _active_rooms(active_rooms),
      _store_header(), _store_message(), _series_ptr_read(nullptr),
      _series_ptr_write(nullptr) {}

void ConnectionSession::start() {
  _room.join(shared_from_this());
  read_header();
}

void ConnectionSession::deliver(Message &msg) {
  bool in_progress = !_write_msgs.empty();
  _write_msgs.push_back(msg);

  if (!in_progress) {
    write_header();
  }
}

void ConnectionSession::read_header() {
  _series_ptr_read.reset(new std::vector<uint8_t>(Header::get_header_size()));

  asio::async_read(
      _socket,
      asio::buffer(_series_ptr_read->data(), Header::get_header_size()),
      [this](const asio::error_code &ec, std::size_t) {
        if (!ec) {
          _store_header = Header(*_series_ptr_read);

          // Reading message involves reading header first.
          read_body();
        } else {
          _room.leave(shared_from_this());
          leave();
        }
      });
}

void ConnectionSession::parse_client_create_game() {
  owner_t owner_id = _store_header.get_owner_id();
  game_t game_id = _store_header.get_game_id();

  // Setting owner_id and game_id for the first time.
  ConnectionParticipant::_owner_id = owner_id;
  ConnectionParticipant::_game_id = game_id;
  // Creating player.
  ConnectionParticipant::_player = new HumanPlayer(nullptr, nullptr, nullptr);

  // Creating new room and responding with OK.
  _active_rooms.insert(
      std::make_pair(game_id, std::make_shared<ConnectionRoom>()));
  // Assigning participant to current room.
  _active_rooms[game_id]->join(shared_from_this());

  Header h(Communication::msg_header_t::SERVER_OK, owner_id, game_id);
  Message msg(h);
  _room.deliver(msg, DeliverType::SAME);
  _room.leave(shared_from_this());
}

void ConnectionSession::parse_client_join_game() {
  owner_t owner_id = _store_header.get_owner_id();
  game_t game_id = _store_header.get_game_id();

  // Setting owner_id for the first time.
  ConnectionParticipant::_owner_id = owner_id;
  ConnectionParticipant::_game_id = game_id;
  // Creating player.
  ConnectionParticipant::_player = new HumanPlayer(nullptr, nullptr, nullptr);

  // Searching for proper room.
  auto it_room = _active_rooms.find(game_id);

  if (it_room == _active_rooms.end() || it_room->second->is_full()) {
    // Current player is not assigned to any room.
    ConnectionParticipant::_game_id = WAITING_ROOM_ID;

    // In case such room doesn't exist responding with ERROR.
    Header h(Communication::msg_header_t::SERVER_ERROR, owner_id, game_id);
    Message msg(h);
    _room.deliver(msg, DeliverType::SAME);
  } else {
    // In case such room exists server responds with OK.
    Header h(Communication::msg_header_t::SERVER_OK, owner_id, game_id);
    Message msg(h);
    // Assigning participant to proper room.
    it_room->second->join(shared_from_this());
    it_room->second->deliver(msg, DeliverType::SAME);

    // Removing participant from current room.
    _room.leave(shared_from_this());

    // Opponent player shall play their move.
    msg.get_header().set_msg_id(Communication::msg_header_t::SERVER_PLAY_MOVE);
    it_room->second->deliver(msg, DeliverType::OPPOSITE);
  }
}

void ConnectionSession::parse_client_chat() {
  owner_t owner_id = _store_header.get_owner_id();
  game_t game_id = _store_header.get_game_id();

  // Sending client's message to all participants in the room.
  _store_message.get_header().set_msg_id(
      Communication::msg_header_t::SERVER_CHAT);
  _active_rooms[game_id]->deliver(_store_message, DeliverType::ALL);
}

void ConnectionSession::parse_client_intermediate_move() {
  owner_t owner_id = _store_header.get_owner_id();
  game_t game_id = _store_header.get_game_id();

  // Participant has throw dice one more time in his move.
  _store_message.get_header().set_msg_id(
      Communication::msg_header_t::SERVER_INTERMEDIATE_MOVE);
  _active_rooms[game_id]->deliver(_store_message, DeliverType::OPPOSITE);
}

void ConnectionSession::parse_client_finish_move() {
  owner_t owner_id = _store_header.get_owner_id();
  game_t game_id = _store_header.get_game_id();

  // Reading which dice are selected to be played.
  std::vector<Dice> selected_dice;
  for (int i = 0; i < NUM_OF_DICE; i++) {
    int8_t x;
    _store_message >> x;

    if (x < 0) {
      selected_dice.emplace_back(-x);
    }
  }

  // Reading how many rolls till end.
  uint8_t roll_countdown;
  _store_message >> roll_countdown;

  // Note that row and col should be read in reverse order.
  uint8_t row, col;
  _store_message >> col >> row;

  // This participant is updating its ticket.
  Fields field = Field::row_to_enum(row);
  Columns column = Column::col_to_enum(col);
  bool outcome = ConnectionParticipant::_player->write_on_ticket(
      selected_dice, field, column, ROLLS_PER_MOVE - roll_countdown);

  if (outcome) {
    // Move is legal and participants are properly notified.
    size_t enum_row = static_cast<size_t>(field);
    size_t enum_col = static_cast<size_t>(column);
    score_t score =
        static_cast<uint8_t>(ConnectionParticipant::_player->get_ticket()
                                 .get_ticket_value()[enum_row][enum_col]);

    auto column_sum =
        ConnectionParticipant::_player->get_ticket().calculate_column_sum(
            column);
    score_t upper_sum = static_cast<score_t>(std::get<0>(column_sum));
    score_t middle_sum = static_cast<score_t>(std::get<1>(column_sum));
    score_t lower_sum = static_cast<score_t>(std::get<2>(column_sum));

    Header h(Communication::msg_header_t::SERVER_FINISH_MOVE, owner_id,
             game_id);
    Message msg(h);
    msg << row << col << score << upper_sum << middle_sum << lower_sum;
    _active_rooms[game_id]->deliver(msg, DeliverType::ALL);

    // If participant filled every field in his ticket with last move.
    if (ConnectionParticipant::_player->get_ticket().is_full())
      _active_rooms[game_id]->increment_filled_tickets();

    // If both participants filled theirs tickets.
    if (_active_rooms[game_id]->get_filled_tickets() == ROOM_LIMIT) {
      determine_winner();
    }

  } else {
    // Move is illegal and owner is notified about it.
    Header h(Communication::msg_header_t::SERVER_ERROR, owner_id, game_id);
    Message msg(h);
    _active_rooms[game_id]->deliver(msg, DeliverType::SAME);
  }
}

void ConnectionSession::determine_winner() {
  game_t game_id = ConnectionParticipant::_game_id;
  owner_t owner_id = ConnectionParticipant::_owner_id;

  // Calculate score for both participants.
  auto scores = _active_rooms[game_id]->get_results();
  std::pair<score_t, owner_t> winner = {0, SERVER_ID};

  // Searching for bigger score.
  for (auto s : scores) {
    winner = std::max(winner, s);
  }

  // Notify both participants who wins.
  Header h(Communication::msg_header_t::SERVER_END_GAME, owner_id, game_id);
  Message msg(h);

  // Sending only owner_id of the winner.
  msg << winner.second;
  _active_rooms[game_id]->deliver(msg, DeliverType::ALL);
}

void ConnectionSession::parse_client_announcement() {
  owner_t owner_id = _store_header.get_owner_id();
  game_t game_id = _store_header.get_game_id();

  // Coordinates of the announcement field.
  uint8_t row;
  _store_message >> row;
  Fields field = Field::row_to_enum(row);

  bool is_allowed = ConnectionParticipant::_player->announce(field);
  if (!is_allowed) {
    // Move is illegal and participant is notified about that.
    Header h(Communication::msg_header_t::SERVER_ERROR, owner_id, game_id);
    Message msg(h);
    _active_rooms[game_id]->deliver(msg, DeliverType::SAME);
  }
}

void ConnectionSession::parse_client_surrender() {
  owner_t owner_id = _store_header.get_owner_id();
  game_t game_id = _store_header.get_game_id();

  // Other participant is notified that they won.
  Header h(Communication::msg_header_t::SERVER_END_GAME, owner_id, game_id);
  Message msg(h);
  // Sending SERVER_ID as winner means that game is surrendered.
  msg << SERVER_ID;
  _active_rooms[game_id]->deliver(msg, DeliverType::OPPOSITE);
}

// Parsing received message.
void ConnectionSession::parse_message() {
  Communication::msg_header_t msg_id = _store_header.get_msg_id();

  if (msg_id == Communication::msg_header_t::CLIENT_CREATE_GAME) {
    parse_client_create_game();
  } else if (msg_id == Communication::msg_header_t::CLIENT_JOIN_GAME) {
    parse_client_join_game();
  } else if (msg_id == Communication::msg_header_t::CLIENT_CHAT) {
    parse_client_chat();
  } else if (msg_id == Communication::msg_header_t::CLIENT_INTERMEDIATE_MOVE) {
    parse_client_intermediate_move();
  } else if (msg_id == Communication::msg_header_t::CLIENT_FINISH_MOVE) {
    parse_client_finish_move();
  } else if (msg_id == Communication::msg_header_t::CLIENT_ANNOUNCEMENT) {
    parse_client_announcement();
  } else if (msg_id == Communication::msg_header_t::CLIENT_SURRENDER) {
    parse_client_surrender();
  }
}

// Ensuring no memory leakage upon leave.
void ConnectionSession::leave() {
  game_t game_id = ConnectionParticipant::get_game_id();
  if (game_id != WAITING_ROOM_ID &&
      _active_rooms.find(game_id) != _active_rooms.end()) {
    Header h(Communication::msg_header_t::SERVER_END_GAME,
             ConnectionParticipant::get_owner_id(), game_id);
    Message msg(h);
    msg << SERVER_ID;
    _active_rooms[game_id]->deliver(msg, DeliverType::OPPOSITE);

    // In case room becomes empty we can delete it.
    if (_active_rooms[game_id]->number_of_participants() == 0) {
      _active_rooms[game_id].reset();
      _active_rooms.erase(game_id);
    }
  }
}

// Reads message's body from the opened socket. It's working asynchronously.
void ConnectionSession::read_body() {
  std::vector<uint8_t> body_series(_store_header.get_size());
  _series_ptr_read.reset(new std::vector<uint8_t>(std::move(body_series)));

  asio::async_read(
      _socket, asio::buffer(_series_ptr_read->data(), _series_ptr_read->size()),
      [this](const asio::error_code &ec, std::size_t) {
        if (!ec) {
          _store_message.set_header(_store_header);
          _store_message << *_series_ptr_read;
          std::cout << _store_message << std::endl;

          // Parse message and message client properly.
          parse_message();

          // Message has been received; start reading a new one.
          read_header();
        } else {
          _room.leave(shared_from_this());
          leave();
        }
      });
}

// Writes message's header to the opened socket. It's working asynchronously.
void ConnectionSession::write_header() {
  std::vector<uint8_t> header_series =
      _write_msgs.front().get_header().serialize();
  _series_ptr_write.reset(new std::vector<uint8_t>(std::move(header_series)));

  asio::async_write(
      _socket,
      asio::buffer(_series_ptr_write->data(), _series_ptr_write->size()),
      [this](const asio::error_code &ec, size_t) {
        if (!ec) {
          // Writing message involes writing header first.
          write_body();
        } else {
          _room.leave(shared_from_this());
          leave();
        }
      });
}

// Writes message's body to the opened socket. It's working asynchronously.
void ConnectionSession::write_body() {
  std::vector<uint8_t> body_series = _write_msgs.front().serialize();
  _series_ptr_write.reset(new std::vector<uint8_t>(std::move(body_series)));

  asio::async_write(
      _socket,
      asio::buffer(_series_ptr_write->data(), _series_ptr_write->size()),
      [this](const asio::error_code &ec, size_t) {
        if (!ec) {
          // Pop written message from not-written queue.
          _write_msgs.pop_front();

          // Ff there are more messages to be written, start writing a new one.
          if (!_write_msgs.empty())
            write_header();
        } else {
          _room.leave(shared_from_this());
          leave();
        }
      });
}