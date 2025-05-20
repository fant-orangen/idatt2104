#include "netcode/visualization/game_window.hpp"
#include "netcode/visualization/network_utility.hpp"

using namespace netcode::visualization;

int main() {
    // Create window in test mode for simulated networking
    netcode::visualization::GameWindow window("Netcode GUI Test", 800, 600, NetworkUtility::Mode::TEST);
    window.run();
    return 0;
} 