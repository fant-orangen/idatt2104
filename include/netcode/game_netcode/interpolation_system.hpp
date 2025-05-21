#ifndef INTERPOLATION_SYSTEM_HPP
#define INTERPOLATION_SYSTEM_HPP


#include "netcode/game_data_types.hpp"
#include <deque>
#include <map>
#include <cstdint>
#include <chrono>

namespace netcode::visualization { class Player; }

namespace netcode::game_netcode {

class InterpolationSystem {
  public:
    InterpolationSystem(uint32_t render_delay_frames = 3);

    void add_state_snapshot(uint32_t player_id, const PlayerFrameState& snapshot);

    void update_player_visual_state(
      netcode::visualization::Player& remote_player,
      uint32_t client_render_frame
    );
  private:
    struct EntityInterpolationBuffer {
      std::deque<PlayerFrameState> snapshots;
      uint32_t last_interpolated_to_frame = 0;
      
    };

    std::map<uint32_t, EntityInterpolationBuffer> entity_buffers_;
    uint32_t render_delay_frames_;

    Vector3 lerp(const Vector3& start, const Vector3& end, float alpha) const;
};

}
#endif //INTERPOLATION_SYSTEM_HPP
