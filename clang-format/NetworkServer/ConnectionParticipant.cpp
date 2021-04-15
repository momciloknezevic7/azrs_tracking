#include "ConnectionParticipant.h"

owner_t ConnectionParticipant::get_owner_id() const { return _owner_id; }

game_t ConnectionParticipant::get_game_id() const { return _game_id; }

Player *ConnectionParticipant::get_player() { return _player; }