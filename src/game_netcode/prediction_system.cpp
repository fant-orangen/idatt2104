#include "netcode/game_netcode/prediction_system.hpp"
#include "netcode/visualization/player.hpp"
#include <algorithm>
#include <iostream>

namespace netcode::game_netcode {

PredictionSystem::PredictionSystem(uint32_t local_player_id) : local_player_id_(local_player_id), has_received_server_state_(false) {}


void PredictionSystem::add_local_input(const PlayerInput &input) {
  pending_inputs_.push_back(input);
  if (pending_inputs_.size() > MAX_INPUT_QUEUE_SIZE) {
    pending_inputs_.pop_front();
  }
}

void PredictionSystem::predict_and_update_player(netcode::visualization::Player &local_player, float delta_time) {
  if (local_player.get_id() != local_player_id_ || !local_player.is_local()) {
    return;
  }

  if (!pending_inputs_.empty()) {
    while (predicted_states_history_.size() > MAX_STATE_HISTORY_SIZE) {
      predicted_states_history_.erase(predicted_states_history_.begin());
    }
  } else {

    PlayerInput null_input = {};
    if (!pending_inputs_.empty()) null_input.frame_number = pending_inputs_.back().frame_number + 1;
    else if (has_received_server_state_) null_input.frame_number = last_confirmed_server_state_.frame_number + 1;
    else null_input.frame_number = 0;

    local_player.update_simulation(null_input, delta_time);
    PlayerFrameState predicted_state = local_player.get_current_frame_state(null_input.frame_number);
    predicted_states_history_[null_input.frame_number] = predicted_state;
      while (predicted_states_history_.size() > MAX_STATE_HISTORY_SIZE) {
        predicted_states_history_.erase(predicted_states_history_.begin());
      }
  }
}

void PredictionSystem::reconcile_with_server_state(netcode::visualization::Player &local_player, const PlayerFrameState &authoritative_server_state, float delta_time_per_simulation_step) {
  if (local_player.get_id() != local_player_id_ || authoritative_server_state.player_id != local_player_id_) {
    return;
  }

  last_confirmed_server_state_ = authoritative_server_state;
  has_received_server_state_ = true;

  auto it = predicted_states_history_.find(authoritative_server_state.frame_number);

  bool misprediction = true;
  if (it != predicted_states_history_.end()) {
    const PlayerFrameState& client_predicted_state_for_frame = it->second;

    if (client_predicted_state_for_frame.states_match(authoritative_server_state)) {
      misprediction = false;
    }
  }

  if (misprediction) {
    local_player.set_authoritative_state(authoritative_server_state);

    auto hist_it = predicted_states_history_.begin();
    while (hist_it != predicted_states_history_.end()) {
      if (hist_it->first <= authoritative_server_state.frame_number) {
        hist_it = predicted_states_history_.erase(hist_it);
      } else {
        ++hist_it;
      }
    }


    uint32_t replayed_frame = authoritative_server_state.frame_number;

    for (const auto& input_to_replay : pending_inputs_) {
      if (input_to_replay.frame_number > authoritative_server_state.frame_number) {
        local_player.update_simulation(input_to_replay, delta_time_per_simulation_step);
        replayed_frame = input_to_replay.frame_number;
        predict_states_history_[replayed_frame] = local_player.get_current_frame_state(replayed_frame);
      }
    }
  }

  pending_inputs_.erase(
    std::remove_if(pending_inputs_.begin(), pending_inputs_.end(),
      [&](const PlayerInput& inp) {
      return inp.frame_number <= authoritative_server_state.frame_number;
    }),
    pending_inputs_.end());

  auto hist_it = predicted_states_history_.begin();
  while (hist_it != predicted_states_history_.end()) {
    if (hist_it->first < authoritative_server_state.frame_number) {
      hist_it = predicted_states_history_.erase(hist_it);
    } else {
      ++hist_it;
    }
  }
}

std::optional<uint32_t> PredictionSystem::get_last_input_frame() const {
  if (!pending_inputs_.empty()) {
    return pending_inputs_.back().frame_number;
  }
  return std::nullopt;
}

void PredictionSystem::clear_inputs_acknowledged_by_server(uint32_t up_to_frame_number) {
  pending_inputs_.erase(
    std::remove_if(pending_inputs_.begin(), pending_inputs_.end(),
      [up_to_frame_number](const PlayerInput& inp) {
        return inp.frame_number <= up_to_frame_number;
      }),
      pending_inputs_.end());
}

}