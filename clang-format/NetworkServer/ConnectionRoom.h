#ifndef CONNECTIONROOM_H
#define CONNECTIONROOM_H

#include <set>

#include "../GameLogic/Game.h"
#include "../NetworkCommon/common.h"
#include "ConnectionParticipant.h"
#include "DeliverType.h"

// Rensposible for participant manipulation and message delivery.
class ConnectionRoom {
public:
  bool is_full() const;
  void join(participant_ptr participant);
  void leave(participant_ptr participant);
  void deliver(Message &msg, DeliverType d_type);
  std::size_t number_of_participants() const;
  uint8_t get_filled_tickets() const;
  void increment_filled_tickets();
  std::vector<std::pair<score_t, owner_t>> get_results() const;

private:
  std::set<participant_ptr> _participants;
  Game _game;
  uint8_t _filled_tickets;
};

typedef std::shared_ptr<ConnectionRoom> room_ptr;

#endif