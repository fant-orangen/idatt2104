#include "netcode/game_netcode/interpolation_system.hpp"
#include "netcode/visualization/player.hpp"
#include "raymath.h"
#include <algorithm>
#include <iostream>

namespace netcode::game_netcode {
InterpolationSystem::InterpolationSystem(uint32_t render_delay_frames)
  : render_delay_frames_(render_delay_frames) {}

Vector3 InterpolationSystem::lerp(const Vector3& start, const Vector3& end, float alpha) const {
    return Vector3Lerp(start, end, alpha);
}

void InterpolationSystem::add_state_snapshot(uint32_t player_id, const PlayerFrameState& snapshot) {
    EntityInterpolationBuffer& buffer = entity_buffers_[player_id];

    auto it = std::lower_bound(buffer.snapshots.begin(), buffer.snapshots.end(), snapshot.frame_number,
    [](const PlayerFrameState& s, uint32_t frame) {
      return s.frame_number < frame;
    });

    if (it != buffer.snapshots.end() && it->frame_number == snapshot.frame_number) {
        *it = snapshot;
    } else {
        buffer.snapshots.insert(it, snapshot);
    }

    while (buffer.snapshots.size() > MAX_SNAPSHOTS_PER_ENTITY) {
        buffer.snapshots.pop_front();
    }
}

void InterpolationSystem::update_player_visual_state(
    netcode::visualization::Player& remote_player,
    uint32_t client_render_frame) {

    if (remote_player.is_local()) return;

    uint32_t target_render_frame = 0;
    if (client_render_frame > render_delay_frames_) {
        target_render_frame = client_render_frame - render_delay_frames_;
    } else {
        target_render_frame = 0;
    }
    auto buffer_it = entity_buffers_.find(remote_player.get_id());
    if (buffer_it == entity_buffers_.end() || buffer_it->second.snapshots.empty()) {
        if (buffer_it != entity_buffers_.end() && !buffer_it->second.snapshots.empty()){
            remote_player.set_interpolated_visual_position(buffer_it->second.snapshots.back().position);
        }
        return;
    }

    std::deque<PlayerFrameState>& snapshots = buffer_it->second.snapshots;

    if (snapshots.size() < 2) {
        remote_player.set_interpolated_visual_position(snapshots.back().position);
        return;
    }
    PlayerFrameState* state_before = nullptr;
    PlayerFrameState* state_after = nullptr;

    for (size_t i = 0; i < snapshots.size(); ++i) {
        if (snapshots[i].frame_number <= target_render_frame) {
            state_before = &snapshots[i];
        }
        if (snapshots[i].frame_number >= target_render_frame) {
            state_after = &snapshots[i];
            if (state_before && state_before->frame_number <= target_render_frame) {
                if (state_before->frame_number == state_after->frame_number) {
                    remote_player.set_interpolated_visual_position(state_before->position);
                    return;
                }
                break;
            }
        }
    }

    if (state_before && state_after && state_before->frame_number < state_after->frame_number) {
        float frame_diff = static_cast<float>(state_after->frame_number - state_before->frame_number);
        float alpha = (static_cast<float>(target_render_frame - state_before->frame_number)) / frame_diff;
        alpha = std::max(0.0f, std::min(1.0f, alpha));

        Vector3 interpolated_position = lerp(state_before->position, state_after->position, alpha);
        remote_player.set_interpolated_visual_position(interpolated_position);
        buffer_it->second.last_interpolated_to_frame = target_render_frame;

    } else if (state_before) {
        remote_player.set_interpolated_visual_position(state_before->position);
        buffer_it->second.last_interpolated_to_frame = state_before->frame_number;
    } else if (state_after) {
        remote_player.set_interpolated_visual_position(state_after->position);
        buffer_it->second.last_interpolated_to_frame = state_after->frame_number;
    }

}
} // namespace netcode::game_netcode