#include "ConnectionRoom.h"

bool ConnectionRoom::is_full() const {
  return number_of_participants() == ROOM_LIMIT;
}

void ConnectionRoom::join(participant_ptr participant) {
  if (participant->get_player()) {
    // This check is neccesarry because of waiting room.
    _game.add_player(participant->get_player());
  }
  _participants.insert(participant);
}

void ConnectionRoom::leave(participant_ptr participant) {
  _participants.erase(participant);
}

void ConnectionRoom::deliver(Message &msg, DeliverType d_type) {
  if (d_type == DeliverType::ALL) {
    for (const participant_ptr &participant : _participants)
      participant->deliver(msg);
  } else if (d_type == DeliverType::SAME) {
    for (const participant_ptr &participant : _participants) {
      if (participant->get_owner_id() == msg.get_header().get_owner_id())
        participant->deliver(msg);
    }
  } else if (d_type == DeliverType::OPPOSITE) {
    for (const participant_ptr &participant : _participants) {
      if (participant->get_owner_id() != msg.get_header().get_owner_id())
        participant->deliver(msg);
    }
  }
}

std::size_t ConnectionRoom::number_of_participants() const {
  return _participants.size();
}

uint8_t ConnectionRoom::get_filled_tickets() const { return _filled_tickets; }

void ConnectionRoom::increment_filled_tickets() { _filled_tickets += 1; }

// Calculating score for both participants when they filled theirs tickets.
std::vector<std::pair<score_t, owner_t>> ConnectionRoom::get_results() const {
  std::vector<std::pair<score_t, owner_t>> scores;
  for (auto p : _participants) {
    score_t score = p->get_player()->get_ticket().calculate_score();
    owner_t owner_id = p->get_owner_id();

    scores.push_back(std::make_pair(score, owner_id));
  }

  return scores;
}
