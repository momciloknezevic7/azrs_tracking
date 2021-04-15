#ifndef CONNECTIONPARTICIPANT_H
#define CONNECTIONPARTICIPANT_H

#include "../GameLogic/Player.h"
#include "../NetworkCommon/Message.h"
#include "../NetworkCommon/common.h"

// Abstract class that represents participant in the chat.
class ConnectionParticipant {
public:
  virtual ~ConnectionParticipant() = default;
  virtual void deliver(Message &msg) = 0;

  owner_t get_owner_id() const;
  game_t get_game_id() const;
  Player *get_player();

protected:
  Player *_player;
  owner_t _owner_id;
  game_t _game_id;
};

typedef std::shared_ptr<ConnectionParticipant> participant_ptr;

#endif