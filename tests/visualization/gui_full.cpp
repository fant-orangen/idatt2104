#include "netcode/visualization/game_window.hpp"
#include "netcode/visualization/network_utility.hpp"

using namespace netcode::visualization;

int main() {
    // Create window in standard mode for real networking
    netcode::visualization::GameWindow window("Netcode GUI Full", 800, 600, NetworkUtility::Mode::STANDARD);
    window.run();
    return 0;
} 