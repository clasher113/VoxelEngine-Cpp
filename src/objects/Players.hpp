#pragma once

#include <memory>
#include <unordered_map>

#include "typedefs.hpp"
#include "interfaces/Serializable.hpp"

inline constexpr float DEF_PLAYER_Y = 100.0f;
inline constexpr float DEF_PLAYER_SPEED = 4.0f;
inline constexpr int DEF_PLAYER_INVENTORY_SIZE = 40;

class Level;
class Player;

class Players : public Serializable {
    Level* level;
    std::unordered_map<int64_t, std::unique_ptr<Player>> players;

    void addPlayer(std::unique_ptr<Player> player);
public:
    Players(Level* level);

    Player* get(int64_t id) const;

    Player* create();

    dv::value serialize() const override;

    void deserialize(const dv::value& src) override;

    auto begin() const {
        return players.begin();
    }

    auto end() const {
        return players.end();
    }
};
