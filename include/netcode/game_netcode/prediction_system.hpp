#ifndef PREDICTION_SYSTEM_HPP
#define PREDICTION_SYSTEM_HPP

#include "netcode/game_data_types.hpp"
#include <deque>
#include <cstdint>
#include <optional>
#include <map>

namespace netcode::visualization { class Player; }

namespace netcode::game_netcode {

class PredictionSystem {
  public:
    PredictionSystem(uint32_t local_player_id);

    void add_local_input(const PlayerInput& input);

    void predict_and_update_player(netcode::visualization::Player& local_player, float delta_time);

    void reconcile_with_server_state(
      netcode::visualization::Player& local_player,
      const PlayerFrameState& authoritative_server_state,
      float delta_time_per_simulation_step
      );

    std::optional<uint32_t> get_last_input_frame() const;

    const std::deque<PlayerInput>& get_pending_inputs() const { return pending_inputs_; }
    void clear_inputs_acknowledged_by_server(uint32_t up_to_frame_number);

  private:
    uint32_t local_player_id_;
    std::deque<PlayerInput> pending_inputs_;

    PlayerFrameState last_confirmed_server_state_;
    bool has_received_server_state_ = false;

    std::map<uint32_t, PlayerFrameState> predicted_states_history_;

    static const size_t MAX_INPUT_QUEUE_SIZE = 60;
    static const size_t MAX_STATE_HISTORY_SIZE = 120;

};

}


#endif //PREDICTION_SYSTEM_HPP
