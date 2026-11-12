// A player on the server: physics, score, pickups and Attack Titan forms

#include "player.hpp"
#include <algorithm>

void Player::setCharacter(int id)
{
    character = id;
    introduced = true;
    runner_weapon = makeWeapon(weaponOf(id));
    runner_ultimate = makeUltimate(ultimateOf(id));
}

void Player::reset(int number, float spawn_y)
{
    player_number = number;
    x = 0;
    y = spawn_y;
    y_velocity = 0;
    score = 0;
    wanted_heat = 0;
    wanted_calm = 0;
    implant = 0;
    armor_timer = 0;
    boons = 0;
    glass_carry = 0;
    flipped = false;
    attack_cooldown = 0;
    form = Form::Normal;
    form_timer = 0;
    shields = upgrades[UPGRADE_SHIELD] > 0;
    rescued = false;
    rescue_timer = 0;
    sprint_timer = 0;
    berserk_timer = 0;
    grace_timer = 0;
    ultimate_charge = 0;
    runner_weapon = makeWeapon(weaponOf(character));
    runner_ultimate = makeUltimate(ultimateOf(character));
    collected_coins.clear();
}

// Pickups stay on the map for the other player, each one only counts once per player
bool Player::takePickup(int tile_x, int tile_y)
{
    return collected_coins.insert({tile_x, tile_y}).second;
}

bool Player::collectCoin(int tile_x, int tile_y)
{
    if (!takePickup(tile_x, tile_y))
        return false;
    addScore(1);
    return true;
}

// Every eddy earned charges the ultimate
void Player::addScore(int points)
{
    if (hasBoon(Boon::GlassCannon) && points > 0) {
        glass_carry += points * (GLASS_CANNON_EDDIES - 1);
        int extra = static_cast<int>(glass_carry);
        glass_carry -= extra;
        points += extra;
    }
    score += points;
    float rate = (implant == 3 ? SYNAPTIC_RATE : 1) * (hasBoon(Boon::Adrenaline) ? ADRENALINE_RATE : 1) *
                 (hasOwn(Boon::RunnerPact, JOHNNY) ? 2 : 1) * (hasOwn(Boon::RunnerPact, JINX) ? 0.5f : 1);
    ultimate_charge = std::min<float>(ultimateCharge(character), ultimate_charge + points * rate);
}

void Player::takeBoon(Boon b)
{
    if (b == Boon::None)
        return;
    boons |= 1u << static_cast<int>(b);
    if (b == Boon::GlassCannon)
        shields = 0;
    // One right away, on top of the one he may carry
    if (b == Boon::Plating)
        shields = std::min(2, shields + 1);
    if (b == Boon::PriceOnHead)
        wanted_heat = std::max(wanted_heat, static_cast<float>(PRICE_ON_HEAD_START * WANTED_KILLS_PER_STAR));
}

bool Player::buyImplant(int id)
{
    if (id < 1 || id > IMPLANTS || score < IMPLANT_PRICES[id])
        return false;
    score -= IMPLANT_PRICES[id];
    implant = id;
    shields = std::max(shields, static_cast<int>(id == 1));
    return true;
}

void Player::startShift()
{
    form = Form::Shifting;
    form_timer = SHIFT_TIME;
}

void Player::powerUp(char tile)
{
    if (tile == SHIELD_TILE)
        shields = std::min(hasBoon(Boon::Plating) ? 2 : 1, shields + 1);
    else if (tile == SANDEVISTAN_TILE)
        sprint_timer = sandevistanTime(upgrades) * (hasBoon(Boon::Overclock) ? 2 : 1) *
                       (hasOwn(Boon::RunnerPact, DAVID) ? CHROME_JUNKIE_TIME : 1);
    else if (tile == BERSERK_TILE)
        berserk_timer = BERSERK_TIME * (hasBoon(Boon::Rage) ? 2 : 1);
}

bool Player::absorbHit()
{
    if (shields == 0)
        return false;
    shields--;
    grace_timer = graceOf(SHIELD_GRACE);
    runner_weapon->hurt();
    return true;
}

// The bill: a share of the run's eddies, what goes to the wallet at its end
bool Player::callTraumaTeam()
{
    if (!canCallTraumaTeam())
        return false;
    rescued = true;
    score -= static_cast<int>(score * TRAUMA_BILL);
    grace_timer = TRAUMA_GRACE;
    rescue_timer = TRAUMA_RESCUE_TIME;
    runner_weapon->hurt();
    return true;
}

bool Player::awaitRescue(float dt, float max_y)
{
    if (rescue_timer <= 0)
        return false;
    rescue_timer -= dt;
    y = max_y;
    y_velocity = 0;
    return true;
}

// Shifting -> Titan -> Emerging -> Normal; bursting out of the smoke throws you up
void Player::updateForm(float dt)
{
    if (form == Form::Normal)
        return;

    form_timer -= dt;
    if (form_timer > 0)
        return;

    if (form == Form::Shifting) {
        form = Form::Titan;
        form_timer = titanTime(upgrades);
    } else if (form == Form::Titan) {
        form = Form::Emerging;
        form_timer = EMERGE_TIME;
        y_velocity = -MAX_VELOCITY;
    } else {
        form = Form::Normal;
        grace_timer = graceOf(END_GRACE);
    }
}

void Player::update(float dt, float forward_speed, float max_y)
{
    updateForm(dt);
    attack_cooldown = std::max(0.0f, attack_cooldown - dt * (isBerserk() ? BERSERK_ATTACK_RATE : 1) * (implant == 2 ? KERENZIKOV_RATE : 1));
    // Glass Cannon: no shield, from the floor or the Ripperdoc
    if (hasBoon(Boon::GlassCannon))
        shields = 0;
    // Subdermal Armor grows the shield back
    if (implant == 1 && shields == 0 && (armor_timer += dt) >= ARMOR_REGEN) {
        shields = 1;
        armor_timer = 0;
    }
    grace_timer = std::max(0.0f, grace_timer - dt);
    if ((wanted_calm += dt) > WANTED_CALM_TIME)
        wanted_heat -= WANTED_COOLING * dt * (hasBoon(Boon::Notorious) ? 0.5f : 1);
    float floor = hasBoon(Boon::PriceOnHead) ? PRICE_ON_HEAD_FLOOR * WANTED_KILLS_PER_STAR : 0;
    wanted_heat = std::max(floor, wanted_heat);

    // Standing still while the smoke rises; the titan runs on the floor. Fishbones holds Jinx still in
    // the air while it charges, then its recoil throws her backward and she picks her pace back up.
    bool charging = runner_ultimate->holdsStill();
    float pace = (isSprinting() ? SANDEVISTAN_SPEED : 1) * runner_ultimate->pace();
    runner_weapon->tick(*this, dt);
    runner_ultimate->tick(dt);
    if (form != Form::Shifting && !charging)
        x += forward_speed * pace * dt;
    if (sprint_timer > 0 && (sprint_timer -= dt) <= 0) {
        sprint_timer = 0;
        grace_timer = graceOf(END_GRACE);
        // Chrome Junkie: it burns through the plating
        if (hasOwn(Boon::RunnerPact, DAVID))
            shields = 0;
    }
    berserk_timer = std::max(0.0f, berserk_timer - dt);
    if (form == Form::Titan) {
        y = max_y;
        y_velocity = 0;
        return;
    }
    // Hanging in the air, aiming
    if (charging) {
        y_velocity = 0;
        return;
    }

    bool thrust = jet_active && form != Form::Shifting;
    y_velocity += (thrust ? -JET_POWER : GRAVITY) * (flipped ? -1 : 1) * agilityOf(character) *
                  (hasOwn(Boon::RunnerPact, MIKASA) ? GAS_OVERLOAD : 1) * dt;
    y_velocity = std::clamp(y_velocity, -MAX_VELOCITY, MAX_VELOCITY);
    y += y_velocity * dt;

    if (y < 0 || y > max_y) {
        y = std::clamp(y, 0.0f, max_y);
        y_velocity = 0;
    }
}
