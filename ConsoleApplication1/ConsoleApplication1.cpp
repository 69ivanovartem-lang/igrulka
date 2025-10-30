#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <float.h>
#include "raylib.h"

// Константы игры
const int TARGET_FPS = 60;

// Множитель размера (увеличиваем на 40%)
const float SIZE_MULTIPLIER = 1.4f;

// Цвета
const Color COLOR_PLAYER = BLUE;
const Color COLOR_GREEN_ENEMY = GREEN;
const Color COLOR_PURPLE_ENEMY = PURPLE;
const Color COLOR_RED_ENEMY = RED;
const Color COLOR_BULLET = YELLOW;
const Color COLOR_PROJECTILE = PURPLE;
const Color COLOR_UPGRADE_HEALTH = GREEN;
const Color COLOR_UPGRADE_ATTACK_SPEED = SKYBLUE;
const Color COLOR_UPGRADE_DAMAGE = ORANGE;
const Color COLOR_UPGRADE_SPEED = WHITE;
const Color COLOR_UPGRADE_WAVE = BLUE;
const Color COLOR_UPGRADE_DOUBLE_SHOT = { 255, 105, 180, 255 };
const Color COLOR_UPGRADE_BOMB = { 139, 69, 19, 255 };
const Color COLOR_UPGRADE_FREEZE = { 0, 191, 255, 255 };
const Color COLOR_UPGRADE_FIREBALL = { 255, 69, 0, 255 };
const Color COLOR_BUTTON = { 100, 100, 200, 255 };
const Color COLOR_BUTTON_HOVER = { 120, 120, 220, 255 };
const Color COLOR_UPGRADE_BUTTON = { 80, 80, 160, 255 };
const Color COLOR_UPGRADE_BUTTON_HOVER = { 100, 100, 180, 255 };
const Color COLOR_UPGRADE_BUTTON_MAXED = { 60, 60, 60, 255 };
const Color COLOR_JOYSTICK_BG = { 100, 100, 100, 150 };
const Color COLOR_JOYSTICK = { 200, 200, 200, 200 };
const Color COLOR_WAVE_ATTACK = { 0, 100, 255, 150 };
const Color COLOR_BOMB = { 100, 0, 0, 255 };
const Color COLOR_EXPLOSION = { 255, 165, 0, 150 };
const Color COLOR_FREEZE = { 100, 200, 255, 150 };
const Color COLOR_FIREBALL = { 255, 69, 0, 255 };
const Color COLOR_FIREBALL_EXPLOSION = { 255, 140, 0, 150 };
const Color COLOR_PROJECTILE_COUNT = { 255, 215, 0, 255 }; // Золотой цвет для улучшения количества снарядов

// Состояния игры
enum GameState {
    MAIN_MENU,
    UPGRADE_MENU,
    PLAYING,
    GAME_OVER,
    RESET_CONFIRM
};

// Структура для мета-прогрессии
struct MetaProgression {
    int totalPoints;        // Всего заработанных очков
    int availablePoints;    // Доступно для траты
    int healthLevel;        // Уровень здоровья (бесконечно)
    int damageLevel;        // Уровень урона (бесконечно)
    int speedLevel;         // Уровень скорости (бесконечно)
    int attackSpeedLevel;   // Уровень скорости атаки (бесконечно)
    int projectileCountLevel; // Уровень количества снарядов (бесконечно)

    // Премиум способности (покупаются один раз)
    bool hasBombAbility;
    bool hasFreezeAbility;
    bool hasWaveAbility;

    // Стоимость улучшений (растет с уровнем)
    int GetHealthCost() const { return (healthLevel + 1) * 2; }
    int GetDamageCost() const { return (damageLevel + 1) * 2; }
    int GetSpeedCost() const { return (speedLevel + 1) * 2; }
    int GetAttackSpeedCost() const { return (attackSpeedLevel + 1) * 2; }
    int GetProjectileCountCost() const { return (projectileCountLevel + 1) * 10; } // Дороже, так как мощное улучшение

    // Стоимость премиум способностей
    int GetBombAbilityCost() const { return 50; }
    int GetFreezeAbilityCost() const { return 75; }
    int GetWaveAbilityCost() const { return 100; }

    // Бонусы от улучшений (бесконечное масштабирование)
    float GetHealthBonus() const { return 1.0f + healthLevel * 0.2f; }        // +20% за уровень
    float GetDamageBonus() const { return 1.0f + damageLevel * 0.15f; }       // +15% за уровень
    float GetSpeedBonus() const { return 1.0f + speedLevel * 0.1f; }          // +10% за уровень
    float GetAttackSpeedBonus() const { return 1.0f + attackSpeedLevel * 0.15f; } // +15% за уровень
    int GetProjectileCount() const { return 2 + projectileCountLevel; }       // 2 снаряда базово + по 1 за уровень

    // Добавление очков за игру
    void AddPoints(int points) {
        totalPoints += points;
        availablePoints += points;
    }

    // Покупка улучшения
    bool BuyUpgrade(int& points, int cost) {
        if (points >= cost) {
            points -= cost;
            return true;
        }
        return false;
    }

    // Сброс прогресса (возвращает часть очков)
    void ResetProgress() {
        int refund = totalPoints / 2; // Возвращаем 50% очков
        totalPoints = refund;
        availablePoints = refund;
        healthLevel = 0;
        damageLevel = 0;
        speedLevel = 0;
        attackSpeedLevel = 0;
        projectileCountLevel = 0;
        // Премиум способности сохраняются после сброса
    }
};

// Переименованная структура волны
struct Shockwave {
    Vector2 position;
    float radius;
    Vector2 direction;
    float speed;
    int damage;
    bool active;
};

// Структура бомбы
struct Bomb {
    Vector2 position;
    float timer;
    float explosionRadius;
    int damage;
    bool active;
    bool exploded;
};

// Структура заморозки
struct FreezeArea {
    Vector2 position;
    float radius;
    float duration;
    float timer;
    bool active;
};

// Структура фаербола
struct Fireball {
    Vector2 position;
    Vector2 velocity;
    float radius;
    int damage;
    float explosionRadius;
    bool active;
    bool exploded;
    float explosionTimer;
};

// Структура кнопки
struct Button {
    Rectangle bounds;
    const char* text;
    bool hovered;
};

// Структура джойстика
struct Joystick {
    Vector2 position;
    float outerRadius;
    float innerRadius;
    Vector2 touchPosition;
    bool isActive;
    Vector2 direction;
};

// Структура игрока
struct Player {
    Vector2 position;
    float radius;
    float speed;
    int health;
    int maxHealth;
    float attackSpeed;
    int damage;
    double lastShotTime;
    int projectileCount; // Количество выпускаемых снарядов

    // Новые способности
    bool hasWaveAttack;
    double waveCooldown;
    double lastWaveTime;
    int waveDamage;

    bool hasDoubleShot;

    bool hasBombAttack;
    double bombCooldown;
    double lastBombTime;
    int bombDamage;
    float bombRadius;

    bool hasFreezeAttack;
    double freezeCooldown;
    double lastFreezeTime;
    float freezeDuration;
    float freezeRadius;

    bool hasFireballAttack;
    double fireballCooldown;
    double lastFireballTime;
    int fireballDamage;
    float fireballExplosionRadius;
    float fireballSpeed;
};

// Структура пули
struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float radius;
    int damage;
};

// Структура снаряда врага
struct EnemyProjectile {
    Vector2 position;
    Vector2 velocity;
    float radius;
    int damage;
};

// Типы врагов
enum EnemyType {
    ENEMY_GREEN,
    ENEMY_PURPLE,
    ENEMY_RED
};

// Структура врага
struct Enemy {
    EnemyType type;
    Vector2 position;
    float radius;
    Color color;
    float speed;
    int health;
    int maxHealth;
    int damage;
    float attackRange;
    bool isRanged;
    double lastAttackTime;
    float attackCooldown;
    std::vector<EnemyProjectile> projectiles;
    bool isFrozen;
    double frozenUntil;
};

// Типы улучшений
enum UpgradeType {
    UPGRADE_HEALTH,
    UPGRADE_ATTACK_SPEED,
    UPGRADE_DAMAGE,
    UPGRADE_SPEED,
    UPGRADE_WAVE,
    UPGRADE_DOUBLE_SHOT,
    UPGRADE_BOMB,
    UPGRADE_FREEZE,
    UPGRADE_FIREBALL,
    UPGRADE_PROJECTILE_COUNT
};

// Структура улучшения
struct Upgrade {
    Vector2 position;
    float radius;
    UpgradeType type;
    Color color;
};

// Вспомогательные функции для векторов
Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
    return { v1.x + v2.x, v1.y + v2.y };
}

Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
    return { v1.x - v2.x, v1.y - v2.y };
}

Vector2 Vector2Scale(Vector2 v, float scale) {
    return { v.x * scale, v.y * scale };
}

float Vector2Length(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

Vector2 Vector2Normalize(Vector2 v) {
    float length = Vector2Length(v);
    if (length > 0) {
        return { v.x / length, v.y / length };
    }
    return { 0, 0 };
}

float Vector2Distance(Vector2 v1, Vector2 v2) {
    float dx = v1.x - v2.x;
    float dy = v1.y - v2.y;
    return sqrtf(dx * dx + dy * dy);
}

// Функции для кнопок
bool IsButtonHovered(Button& button) {
    button.hovered = CheckCollisionPointRec(GetMousePosition(), button.bounds);
    return button.hovered;
}

bool IsButtonClicked(Button& button) {
    return IsButtonHovered(button) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void DrawButton(Button& button) {
    Color color = button.hovered ? COLOR_BUTTON_HOVER : COLOR_BUTTON;
    DrawRectangleRec(button.bounds, color);
    DrawRectangleLinesEx(button.bounds, 2, WHITE);

    int textWidth = MeasureText(button.text, 20);
    int textX = button.bounds.x + (button.bounds.width - textWidth) / 2;
    int textY = button.bounds.y + (button.bounds.height - 20) / 2;

    DrawText(button.text, textX, textY, 20, WHITE);
}

// Функции для джойстика
Joystick CreateJoystick() {
    Joystick joystick;
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    joystick.outerRadius = 100.0f * SIZE_MULTIPLIER;
    joystick.innerRadius = 40.0f * SIZE_MULTIPLIER;

    joystick.position = {
        joystick.outerRadius + 80.0f,
        screenHeight - joystick.outerRadius - 60.0f
    };

    joystick.touchPosition = joystick.position;
    joystick.isActive = false;
    joystick.direction = { 0, 0 };
    return joystick;
}

void UpdateJoystick(Joystick& joystick) {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 touchPos = GetMousePosition();
        float distance = Vector2Distance(touchPos, joystick.position);

        if (!joystick.isActive && distance < joystick.outerRadius * 1.5f) {
            joystick.isActive = true;
        }

        if (joystick.isActive) {
            Vector2 direction = { touchPos.x - joystick.position.x, touchPos.y - joystick.position.y };
            float dist = Vector2Length(direction);

            float maxDistance = joystick.outerRadius - joystick.innerRadius * 0.3f;
            if (dist > maxDistance) {
                float scale = maxDistance / dist;
                direction = Vector2Scale(direction, scale);
            }

            joystick.touchPosition = Vector2Add(joystick.position, direction);

            if (dist > 5.0f) {
                joystick.direction = Vector2Normalize(direction);
            }
            else {
                joystick.direction = { 0, 0 };
            }
        }
    }
    else {
        joystick.isActive = false;
        joystick.touchPosition = joystick.position;
        joystick.direction = { 0, 0 };
    }
}

void DrawJoystick(const Joystick& joystick) {
    DrawCircleV(joystick.position, joystick.outerRadius, COLOR_JOYSTICK_BG);
    DrawCircleV(joystick.touchPosition, joystick.innerRadius, COLOR_JOYSTICK);
}

// Функции для игрока
Player CreatePlayer(const MetaProgression& meta) {
    Player player;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    player.position = { screenWidth / 2.0f, screenHeight / 2.0f };
    player.radius = 15.0f * SIZE_MULTIPLIER;

    float baseSpeed = 5.0f;
    int baseHealth = 100;
    int baseDamage = 20;
    float baseAttackSpeed = 1.0f;

    player.speed = std::max(1.0f, baseSpeed * meta.GetSpeedBonus());
    player.maxHealth = std::max(50, (int)(baseHealth * meta.GetHealthBonus()));
    player.health = player.maxHealth;
    player.damage = std::max(5, (int)(baseDamage * meta.GetDamageBonus()));
    player.attackSpeed = std::max(0.1f, baseAttackSpeed * meta.GetAttackSpeedBonus());
    player.projectileCount = meta.GetProjectileCount();

    player.lastShotTime = -1.0;

    player.hasWaveAttack = meta.hasWaveAbility;
    player.waveCooldown = 45.0;
    player.lastWaveTime = -45.0;
    player.waveDamage = 50;

    player.hasDoubleShot = false;

    player.hasBombAttack = meta.hasBombAbility;
    player.bombCooldown = 15.0;
    player.lastBombTime = -15.0;
    player.bombDamage = 40;
    player.bombRadius = 80.0f * SIZE_MULTIPLIER;

    player.hasFreezeAttack = meta.hasFreezeAbility;
    player.freezeCooldown = 25.0;
    player.lastFreezeTime = -25.0;
    player.freezeDuration = 3.0f;
    player.freezeRadius = 100.0f * SIZE_MULTIPLIER;

    player.hasFireballAttack = false;
    player.fireballCooldown = 8.0;
    player.lastFireballTime = -8.0;
    player.fireballDamage = 30 + player.damage / 2;
    player.fireballExplosionRadius = 60.0f * SIZE_MULTIPLIER;
    player.fireballSpeed = 8.0f;

    return player;
}

void UpdatePlayer(Player& player, const Joystick& joystick, double currentTime, std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, std::vector<Shockwave>& shockwaves, std::vector<Bomb>& bombs, std::vector<FreezeArea>& freezeAreas, std::vector<Fireball>& fireballs) {
    Vector2 movement = { 0, 0 };

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if (IsKeyDown(KEY_A) && player.position.x - player.speed > 0) movement.x -= 1;
    if (IsKeyDown(KEY_D) && player.position.x + player.speed < screenWidth) movement.x += 1;
    if (IsKeyDown(KEY_W) && player.position.y - player.speed > 0) movement.y -= 1;
    if (IsKeyDown(KEY_S) && player.position.y + player.speed < screenHeight) movement.y += 1;

    if (joystick.isActive) {
        movement.x += joystick.direction.x;
        movement.y += joystick.direction.y;
    }

    if (movement.x != 0 || movement.y != 0) {
        float length = sqrtf(movement.x * movement.x + movement.y * movement.y);
        movement.x /= length;
        movement.y /= length;

        player.position.x += movement.x * player.speed;
        player.position.y += movement.y * player.speed;

        player.position.x = std::max(player.radius, std::min(screenWidth - player.radius, player.position.x));
        player.position.y = std::max(player.radius, std::min(screenHeight - player.radius, player.position.y));
    }

    // Автоматическая стрельба по ближайшему врагу
    if (currentTime - player.lastShotTime > 1.0 / player.attackSpeed) {
        float min_distance = FLT_MAX;
        Enemy* nearest_enemy = nullptr;
        for (auto& enemy : enemies) {
            float distance = Vector2Distance(player.position, enemy.position);
            if (distance < min_distance) {
                min_distance = distance;
                nearest_enemy = &enemy;
            }
        }

        if (nearest_enemy != nullptr) {
            Vector2 direction = Vector2Subtract(nearest_enemy->position, player.position);
            float length = Vector2Length(direction);

            if (length > 0) {
                direction.x /= length;
                direction.y /= length;

                // Создаем снаряды в зависимости от их количества
                for (int i = 0; i < player.projectileCount; i++) {
                    Bullet bullet;
                    bullet.position = player.position;

                    if (player.projectileCount == 1) {
                        // Один снаряд - летит прямо
                        bullet.velocity.x = direction.x * 10.0f;
                        bullet.velocity.y = direction.y * 10.0f;
                    }
                    else {
                        // Несколько снарядов - распределяем веером
                        float angleOffset = (i - (player.projectileCount - 1) / 2.0f) * 0.2f;
                        Vector2 rotatedDirection = {
                            direction.x * cosf(angleOffset) - direction.y * sinf(angleOffset),
                            direction.x * sinf(angleOffset) + direction.y * cosf(angleOffset)
                        };
                        bullet.velocity.x = rotatedDirection.x * 10.0f;
                        bullet.velocity.y = rotatedDirection.y * 10.0f;
                    }

                    bullet.radius = 5.0f * SIZE_MULTIPLIER;
                    bullet.damage = player.damage;
                    bullets.push_back(bullet);
                }

                // Дополнительный выстрел при улучшении
                if (player.hasDoubleShot) {
                    Bullet secondBullet;
                    secondBullet.position = player.position;

                    Vector2 perpendicular = { -direction.y, direction.x };
                    secondBullet.velocity.x = direction.x * 8.0f + perpendicular.x * 3.0f;
                    secondBullet.velocity.y = direction.y * 8.0f + perpendicular.y * 3.0f;

                    float velLength = Vector2Length(secondBullet.velocity);
                    secondBullet.velocity.x = secondBullet.velocity.x / velLength * 10.0f;
                    secondBullet.velocity.y = secondBullet.velocity.y / velLength * 10.0f;

                    secondBullet.radius = 5.0f * SIZE_MULTIPLIER;
                    secondBullet.damage = player.damage;
                    bullets.push_back(secondBullet);
                }

                player.lastShotTime = currentTime;
            }
        }
    }

    // Активация волновой атаки
    if (player.hasWaveAttack && currentTime - player.lastWaveTime > player.waveCooldown) {
        if (!enemies.empty()) {
            Shockwave shockwave;
            shockwave.position = player.position;
            shockwave.radius = 10.0f * SIZE_MULTIPLIER;

            Vector2 averagePosition = { 0, 0 };
            int enemyCount = 0;

            for (const auto& enemy : enemies) {
                float distance = Vector2Distance(player.position, enemy.position);
                if (distance < 300.0f * SIZE_MULTIPLIER) {
                    averagePosition.x += enemy.position.x;
                    averagePosition.y += enemy.position.y;
                    enemyCount++;
                }
            }

            Vector2 waveDirection = { 0, 1 };

            if (enemyCount > 0) {
                averagePosition.x /= enemyCount;
                averagePosition.y /= enemyCount;
                waveDirection = Vector2Subtract(averagePosition, player.position);
                waveDirection = Vector2Normalize(waveDirection);
            }
            else {
                float min_distance = FLT_MAX;
                for (const auto& enemy : enemies) {
                    float distance = Vector2Distance(player.position, enemy.position);
                    if (distance < min_distance) {
                        min_distance = distance;
                        waveDirection = Vector2Subtract(enemy.position, player.position);
                        waveDirection = Vector2Normalize(waveDirection);
                    }
                }
            }

            shockwave.direction = waveDirection;
            shockwave.speed = 3.0f;
            shockwave.damage = player.waveDamage;
            shockwave.active = true;
            shockwaves.push_back(shockwave);

            player.lastWaveTime = currentTime;
        }
    }

    // Активация бомбы
    if (player.hasBombAttack && currentTime - player.lastBombTime > player.bombCooldown) {
        Bomb bomb;
        bomb.position = player.position;
        bomb.timer = 2.0f;
        bomb.explosionRadius = player.bombRadius;
        bomb.damage = player.bombDamage;
        bomb.active = true;
        bomb.exploded = false;
        bombs.push_back(bomb);

        player.lastBombTime = currentTime;
    }

    // Активация заморозки
    if (player.hasFreezeAttack && currentTime - player.lastFreezeTime > player.freezeCooldown) {
        FreezeArea freeze;
        freeze.position = player.position;
        freeze.radius = player.freezeRadius;
        freeze.duration = player.freezeDuration;
        freeze.timer = freeze.duration;
        freeze.active = true;
        freezeAreas.push_back(freeze);

        player.lastFreezeTime = currentTime;
    }

    // Активация фаербола
    if (player.hasFireballAttack && currentTime - player.lastFireballTime > player.fireballCooldown) {
        float min_distance = FLT_MAX;
        Enemy* nearest_enemy = nullptr;
        for (auto& enemy : enemies) {
            float distance = Vector2Distance(player.position, enemy.position);
            if (distance < min_distance) {
                min_distance = distance;
                nearest_enemy = &enemy;
            }
        }

        if (nearest_enemy != nullptr) {
            Fireball fireball;
            fireball.position = player.position;

            Vector2 direction = Vector2Subtract(nearest_enemy->position, player.position);
            direction = Vector2Normalize(direction);

            fireball.velocity.x = direction.x * player.fireballSpeed;
            fireball.velocity.y = direction.y * player.fireballSpeed;
            fireball.radius = 8.0f * SIZE_MULTIPLIER;
            fireball.damage = player.fireballDamage + player.damage / 2;
            fireball.explosionRadius = player.fireballExplosionRadius;
            fireball.active = true;
            fireball.exploded = false;
            fireball.explosionTimer = 0.3f;

            fireballs.push_back(fireball);
            player.lastFireballTime = currentTime;
        }
    }
}

void DrawPlayer(const Player& player) {
    DrawCircleV(player.position, player.radius, COLOR_PLAYER);

    float healthBarWidth = 40.0f * SIZE_MULTIPLIER;
    float healthBarHeight = 5.0f * SIZE_MULTIPLIER;
    Vector2 healthBarPos = {
            player.position.x - healthBarWidth / 2,
            player.position.y - player.radius - 15.0f * SIZE_MULTIPLIER
    };

    DrawRectangle((int)healthBarPos.x, (int)healthBarPos.y, (int)healthBarWidth, (int)healthBarHeight, RED);
    DrawRectangle((int)healthBarPos.x, (int)healthBarPos.y, (int)(healthBarWidth * (player.health / (float)player.maxHealth)), (int)healthBarHeight, GREEN);
}

// Функции для пуль
void UpdateBullets(std::vector<Bullet>& bullets) {
    for (auto it = bullets.begin(); it != bullets.end();) {
        if (it == bullets.end()) break;

        it->position.x += it->velocity.x;
        it->position.y += it->velocity.y;

        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        if (it->position.x < -100 || it->position.x > screenWidth + 100 ||
            it->position.y < -100 || it->position.y > screenHeight + 100) {
            it = bullets.erase(it);
        }
        else {
            ++it;
        }
    }
}

void DrawBullets(const std::vector<Bullet>& bullets) {
    for (const auto& bullet : bullets) {
        DrawCircleV(bullet.position, bullet.radius, COLOR_BULLET);
    }
}

// Функции для шоквейвов
void UpdateShockwaves(std::vector<Shockwave>& shockwaves) {
    for (auto it = shockwaves.begin(); it != shockwaves.end();) {
        if (it == shockwaves.end()) break;

        it->position.x += it->direction.x * it->speed;
        it->position.y += it->direction.y * it->speed;
        it->radius += 0.8f;

        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();

        if (it->position.x < -100 || it->position.x > screenWidth + 100 ||
            it->position.y < -100 || it->position.y > screenHeight + 100 ||
            it->radius > 200 * SIZE_MULTIPLIER) {
            it = shockwaves.erase(it);
        }
        else {
            ++it;
        }
    }
}

void DrawShockwaves(const std::vector<Shockwave>& shockwaves) {
    for (const auto& shockwave : shockwaves) {
        DrawCircleV(shockwave.position, shockwave.radius, COLOR_WAVE_ATTACK);
        DrawCircleLines((int)shockwave.position.x, (int)shockwave.position.y, (int)shockwave.radius, BLUE);
    }
}

// Функции для бомб
void UpdateBombs(std::vector<Bomb>& bombs, double deltaTime) {
    for (auto it = bombs.begin(); it != bombs.end();) {
        if (it == bombs.end()) break;

        it->timer -= (float)deltaTime;

        if (it->timer <= 0 && !it->exploded) {
            it->exploded = true;
            it->timer = 0.3f;
        }

        if (it->exploded && it->timer <= 0) {
            it = bombs.erase(it);
        }
        else {
            ++it;
        }
    }
}

void DrawBombs(const std::vector<Bomb>& bombs) {
    for (const auto& bomb : bombs) {
        if (!bomb.exploded) {
            Color bombColor = COLOR_BOMB;
            if (bomb.timer < 0.5f) {
                bombColor = (fmod(bomb.timer, 0.1f) > 0.05f) ? RED : COLOR_BOMB;
            }
            DrawCircleV(bomb.position, 12.0f * SIZE_MULTIPLIER, bombColor);
        }
        else {
            DrawCircleV(bomb.position, bomb.explosionRadius, COLOR_EXPLOSION);
            DrawCircleLines((int)bomb.position.x, (int)bomb.position.y, (int)bomb.explosionRadius, RED);
        }
    }
}

// Функции для заморозки
void UpdateFreezeAreas(std::vector<FreezeArea>& freezeAreas, double deltaTime) {
    for (auto it = freezeAreas.begin(); it != freezeAreas.end();) {
        if (it == freezeAreas.end()) break;

        it->timer -= (float)deltaTime;

        if (it->timer <= 0) {
            it = freezeAreas.erase(it);
        }
        else {
            ++it;
        }
    }
}

void DrawFreezeAreas(const std::vector<FreezeArea>& freezeAreas) {
    for (const auto& freeze : freezeAreas) {
        DrawCircleV(freeze.position, freeze.radius, COLOR_FREEZE);
        DrawCircleLines((int)freeze.position.x, (int)freeze.position.y, (int)freeze.radius, BLUE);
    }
}

// Функции для фаерболов
void UpdateFireballs(std::vector<Fireball>& fireballs, std::vector<Enemy>& enemies, double deltaTime) {
    for (auto it = fireballs.begin(); it != fireballs.end();) {
        if (it == fireballs.end()) break;

        if (!it->exploded) {
            it->position.x += it->velocity.x;
            it->position.y += it->velocity.y;

            bool hitEnemy = false;
            for (auto& enemy : enemies) {
                float distance = Vector2Distance(it->position, enemy.position);
                if (distance < it->radius + enemy.radius) {
                    hitEnemy = true;
                    break;
                }
            }

            int screenWidth = GetScreenWidth();
            int screenHeight = GetScreenHeight();

            if (it->position.x < 0 || it->position.x > screenWidth ||
                it->position.y < 0 || it->position.y > screenHeight ||
                hitEnemy) {
                it->exploded = true;
            }
        }
        else {
            it->explosionTimer -= (float)deltaTime;
            if (it->explosionTimer <= 0) {
                it = fireballs.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void DrawFireballs(const std::vector<Fireball>& fireballs) {
    for (const auto& fireball : fireballs) {
        if (!fireball.exploded) {
            DrawCircleV(fireball.position, fireball.radius, COLOR_FIREBALL);

            for (int i = 0; i < 3; i++) {
                float offset = (float)GetTime() * 10.0f + i * 2.0f;
                float pulse = (sinf(offset) + 1.0f) * 0.5f;
                float trailRadius = fireball.radius * (0.7f + pulse * 0.3f);
                Color trailColor = { 255, 140, 0, (unsigned char)(150 * pulse) };
                DrawCircleV(fireball.position, trailRadius, trailColor);
            }
        }
        else {
            DrawCircleV(fireball.position, fireball.explosionRadius, COLOR_FIREBALL_EXPLOSION);
            DrawCircleLines((int)fireball.position.x, (int)fireball.position.y, (int)fireball.explosionRadius, ORANGE);
        }
    }
}

// Функции для врагов (бесконечное усложнение)
Enemy CreateEnemy(EnemyType type, Vector2 position, float difficultyScale, int waveNumber) {
    Enemy enemy;
    enemy.type = type;
    enemy.position = position;
    enemy.radius = 15.0f * SIZE_MULTIPLIER;
    enemy.isFrozen = false;
    enemy.frozenUntil = 0.0;

    // Бесконечное масштабирование сложности
    float waveMultiplier = 1.0f + (waveNumber * 0.1f); // +10% за каждую волну
    float healthMultiplier = 1.0f + difficultyScale * 0.5f + (waveNumber * 0.05f);
    float damageMultiplier = 1.0f + difficultyScale * 0.3f + (waveNumber * 0.03f);
    float speedMultiplier = 1.0f + difficultyScale * 0.2f + (waveNumber * 0.02f);

    switch (type) {
    case ENEMY_GREEN:
        enemy.color = COLOR_GREEN_ENEMY;
        enemy.speed = 2.5f * speedMultiplier;
        enemy.health = std::max(1, (int)(30 * healthMultiplier * waveMultiplier));
        enemy.maxHealth = enemy.health;
        enemy.damage = std::max(1, (int)(5 * damageMultiplier * waveMultiplier));
        enemy.attackRange = 20.0f * SIZE_MULTIPLIER;
        enemy.isRanged = false;
        enemy.attackCooldown = 1.0f / waveMultiplier;
        break;

    case ENEMY_PURPLE:
        enemy.color = COLOR_PURPLE_ENEMY;
        enemy.speed = 1.0f * speedMultiplier;
        enemy.health = std::max(1, (int)(50 * healthMultiplier * waveMultiplier));
        enemy.maxHealth = enemy.health;
        enemy.damage = std::max(1, (int)(8 * damageMultiplier * waveMultiplier));
        enemy.attackRange = 150.0f * SIZE_MULTIPLIER;
        enemy.isRanged = true;
        enemy.attackCooldown = 1.0f / waveMultiplier;
        break;

    case ENEMY_RED:
        enemy.color = COLOR_RED_ENEMY;
        enemy.speed = 0.8f * speedMultiplier;
        enemy.health = std::max(1, (int)(150 * healthMultiplier * waveMultiplier));
        enemy.maxHealth = enemy.health;
        enemy.damage = std::max(1, (int)(15 * damageMultiplier * waveMultiplier));
        enemy.attackRange = 25.0f * SIZE_MULTIPLIER;
        enemy.isRanged = false;
        enemy.attackCooldown = 1.0f / waveMultiplier;
        break;
    }

    enemy.lastAttackTime = -enemy.attackCooldown;
    return enemy;
}

void UpdateEnemies(std::vector<Enemy>& enemies, Player& player, double currentTime, const std::vector<FreezeArea>& freezeAreas) {
    for (auto& enemy : enemies) {
        enemy.isFrozen = false;
        for (const auto& freeze : freezeAreas) {
            if (Vector2Distance(enemy.position, freeze.position) <= freeze.radius) {
                enemy.isFrozen = true;
                enemy.frozenUntil = currentTime + 0.1;
                break;
            }
        }

        if (!enemy.isFrozen || currentTime > enemy.frozenUntil) {
            Vector2 direction = Vector2Subtract(player.position, enemy.position);
            float distance = Vector2Length(direction);

            if (distance > enemy.attackRange) {
                if (distance > 0) {
                    direction.x /= distance;
                    direction.y /= distance;
                }

                enemy.position.x += direction.x * enemy.speed;
                enemy.position.y += direction.y * enemy.speed;
            }
            else if (currentTime - enemy.lastAttackTime > enemy.attackCooldown) {
                if (enemy.isRanged) {
                    Vector2 projDirection = Vector2Normalize(direction);
                    EnemyProjectile projectile;
                    projectile.position = enemy.position;
                    projectile.velocity.x = projDirection.x * 4.0f;
                    projectile.velocity.y = projDirection.y * 4.0f;
                    projectile.radius = 7.0f * SIZE_MULTIPLIER;
                    projectile.damage = enemy.damage;

                    enemy.projectiles.push_back(projectile);
                }
                else {
                    player.health -= enemy.damage;
                }

                enemy.lastAttackTime = currentTime;
            }
        }

        for (auto it = enemy.projectiles.begin(); it != enemy.projectiles.end();) {
            if (it == enemy.projectiles.end()) break;

            it->position.x += it->velocity.x;
            it->position.y += it->velocity.y;

            int screenWidth = GetScreenWidth();
            int screenHeight = GetScreenHeight();

            if (it->position.x < 0 || it->position.x > screenWidth ||
                it->position.y < 0 || it->position.y > screenHeight) {
                it = enemy.projectiles.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

void DrawEnemies(const std::vector<Enemy>& enemies) {
    for (const auto& enemy : enemies) {
        Color enemyColor = enemy.color;
        if (enemy.isFrozen) {
            enemyColor = BLUE;
        }

        DrawCircleV(enemy.position, enemy.radius, enemyColor);

        float healthBarWidth = 30.0f * SIZE_MULTIPLIER;
        float healthBarHeight = 4.0f * SIZE_MULTIPLIER;
        Vector2 healthBarPos = {
                enemy.position.x - healthBarWidth / 2,
                enemy.position.y - enemy.radius - 12.0f * SIZE_MULTIPLIER
        };

        DrawRectangle((int)healthBarPos.x, (int)healthBarPos.y, (int)healthBarWidth, (int)healthBarHeight, RED);
        DrawRectangle((int)healthBarPos.x, (int)healthBarPos.y, (int)(healthBarWidth * (enemy.health / (float)enemy.maxHealth)), (int)healthBarHeight, GREEN);

        for (const auto& projectile : enemy.projectiles) {
            DrawCircleV(projectile.position, projectile.radius, COLOR_PROJECTILE);
        }
    }
}

// Функции для улучшений
Upgrade CreateUpgrade(Vector2 position) {
    Upgrade upgrade;
    upgrade.position = position;
    upgrade.radius = 10.0f * SIZE_MULTIPLIER;

    int type = GetRandomValue(0, 9); // Добавили UPGRADE_PROJECTILE_COUNT
    switch (type) {
    case 0:
        upgrade.type = UPGRADE_HEALTH;
        upgrade.color = COLOR_UPGRADE_HEALTH;
        break;
    case 1:
        upgrade.type = UPGRADE_ATTACK_SPEED;
        upgrade.color = COLOR_UPGRADE_ATTACK_SPEED;
        break;
    case 2:
        upgrade.type = UPGRADE_DAMAGE;
        upgrade.color = COLOR_UPGRADE_DAMAGE;
        break;
    case 3:
        upgrade.type = UPGRADE_SPEED;
        upgrade.color = COLOR_UPGRADE_SPEED;
        break;
    case 4:
        upgrade.type = UPGRADE_WAVE;
        upgrade.color = COLOR_UPGRADE_WAVE;
        break;
    case 5:
        upgrade.type = UPGRADE_DOUBLE_SHOT;
        upgrade.color = COLOR_UPGRADE_DOUBLE_SHOT;
        break;
    case 6:
        upgrade.type = UPGRADE_BOMB;
        upgrade.color = COLOR_UPGRADE_BOMB;
        break;
    case 7:
        upgrade.type = UPGRADE_FREEZE;
        upgrade.color = COLOR_UPGRADE_FREEZE;
        break;
    case 8:
        upgrade.type = UPGRADE_FIREBALL;
        upgrade.color = COLOR_UPGRADE_FIREBALL;
        break;
    case 9:
        upgrade.type = UPGRADE_PROJECTILE_COUNT;
        upgrade.color = COLOR_PROJECTILE_COUNT;
        break;
    }

    return upgrade;
}

void ApplyUpgrade(Upgrade& upgrade, Player& player, MetaProgression& meta) {
    switch (upgrade.type) {
    case UPGRADE_HEALTH:
        player.maxHealth += 10;
        player.health = std::min(player.maxHealth, player.health + 20);
        break;

    case UPGRADE_ATTACK_SPEED:
        player.attackSpeed += 0.2f;
        break;

    case UPGRADE_DAMAGE:
        player.damage += 5;
        break;

    case UPGRADE_SPEED:
        player.speed += 0.5f;
        break;

    case UPGRADE_WAVE:
        if (!player.hasWaveAttack) {
            player.hasWaveAttack = true;
        }
        else {
            player.waveCooldown = std::max(10.0, player.waveCooldown * 0.8f);
            player.waveDamage += 10;
        }
        break;

    case UPGRADE_DOUBLE_SHOT:
        player.hasDoubleShot = true;
        break;

    case UPGRADE_BOMB:
        if (!player.hasBombAttack) {
            player.hasBombAttack = true;
        }
        else {
            player.bombCooldown = std::max(8.0, player.bombCooldown * 0.8f);
            player.bombDamage += 10;
            player.bombRadius += 10.0f * SIZE_MULTIPLIER;
        }
        break;

    case UPGRADE_FREEZE:
        if (!player.hasFreezeAttack) {
            player.hasFreezeAttack = true;
        }
        else {
            player.freezeCooldown = std::max(12.0, player.freezeCooldown * 0.8f);
            player.freezeDuration += 0.5f;
            player.freezeRadius += 15.0f * SIZE_MULTIPLIER;
        }
        break;

    case UPGRADE_FIREBALL:
        if (!player.hasFireballAttack) {
            player.hasFireballAttack = true;
        }
        else {
            player.fireballCooldown = std::max(4.0, player.fireballCooldown * 0.8f);
            player.fireballDamage += 5;
            player.fireballExplosionRadius += 10.0f * SIZE_MULTIPLIER;
        }
        break;

    case UPGRADE_PROJECTILE_COUNT:
        player.projectileCount++;
        break;
    }
}

void DrawUpgrades(const std::vector<Upgrade>& upgrades) {
    for (const auto& upgrade : upgrades) {
        if (upgrade.type >= UPGRADE_WAVE) {
            DrawRectangle((int)(upgrade.position.x - upgrade.radius), (int)(upgrade.position.y - upgrade.radius),
                (int)(upgrade.radius * 2), (int)(upgrade.radius * 2), upgrade.color);
        }
        else {
            DrawCircleV(upgrade.position, upgrade.radius, upgrade.color);
        }
    }
}

// Безопасная проверка коллизий
void CheckCollisions(Player& player, std::vector<Bullet>& bullets, std::vector<Enemy>& enemies,
    std::vector<Upgrade>& upgrades, std::vector<Shockwave>& shockwaves,
    std::vector<Bomb>& bombs, std::vector<FreezeArea>& freezeAreas,
    std::vector<Fireball>& fireballs, int& score, MetaProgression& meta) {

    // Пули - враги
    for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
        bool bulletHit = false;

        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (bulletIt == bullets.end() || enemyIt == enemies.end()) break;

            float distance = Vector2Distance(bulletIt->position, enemyIt->position);

            if (distance < bulletIt->radius + enemyIt->radius) {
                enemyIt->health -= bulletIt->damage;
                bulletHit = true;

                if (enemyIt->health <= 0) {
                    switch (enemyIt->type) {
                    case ENEMY_GREEN: score += 10; break;
                    case ENEMY_PURPLE: score += 20; break;
                    case ENEMY_RED: score += 50; break;
                    }
                    enemyIt = enemies.erase(enemyIt);
                    continue;
                }
                else {
                    ++enemyIt;
                }
                break;
            }
            else {
                ++enemyIt;
            }
        }

        if (bulletHit) {
            bulletIt = bullets.erase(bulletIt);
        }
        else {
            ++bulletIt;
        }
    }

    // Шоквейвы - враги
    for (auto waveIt = shockwaves.begin(); waveIt != shockwaves.end(); ++waveIt) {
        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            if (waveIt == shockwaves.end() || enemyIt == enemies.end()) break;

            float distance = Vector2Distance(waveIt->position, enemyIt->position);

            if (distance < waveIt->radius + enemyIt->radius) {
                enemyIt->health -= waveIt->damage;

                if (enemyIt->health <= 0) {
                    switch (enemyIt->type) {
                    case ENEMY_GREEN: score += 10; break;
                    case ENEMY_PURPLE: score += 20; break;
                    case ENEMY_RED: score += 50; break;
                    }
                    enemyIt = enemies.erase(enemyIt);
                    continue;
                }
            }
            ++enemyIt;
        }
    }

    // Бомбы - враги
    for (auto bombIt = bombs.begin(); bombIt != bombs.end();) {
        if (bombIt->exploded) {
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                if (bombIt == bombs.end() || enemyIt == enemies.end()) break;

                float distance = Vector2Distance(bombIt->position, enemyIt->position);

                if (distance < bombIt->explosionRadius + enemyIt->radius) {
                    enemyIt->health -= bombIt->damage;

                    if (enemyIt->health <= 0) {
                        switch (enemyIt->type) {
                        case ENEMY_GREEN: score += 10; break;
                        case ENEMY_PURPLE: score += 20; break;
                        case ENEMY_RED: score += 50; break;
                        }
                        enemyIt = enemies.erase(enemyIt);
                        continue;
                    }
                }
                ++enemyIt;
            }
            bombIt = bombs.erase(bombIt);
        }
        else {
            ++bombIt;
        }
    }

    // Фаерболы - враги
    for (auto fireballIt = fireballs.begin(); fireballIt != fireballs.end();) {
        if (fireballIt->exploded) {
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                if (fireballIt == fireballs.end() || enemyIt == enemies.end()) break;

                float distance = Vector2Distance(fireballIt->position, enemyIt->position);

                if (distance < fireballIt->explosionRadius + enemyIt->radius) {
                    enemyIt->health -= fireballIt->damage;

                    if (enemyIt->health <= 0) {
                        switch (enemyIt->type) {
                        case ENEMY_GREEN: score += 10; break;
                        case ENEMY_PURPLE: score += 20; break;
                        case ENEMY_RED: score += 50; break;
                        }
                        enemyIt = enemies.erase(enemyIt);
                        continue;
                    }
                }
                ++enemyIt;
            }
            ++fireballIt;
        }
        else {
            bool hitEnemy = false;
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                if (fireballIt == fireballs.end() || enemyIt == enemies.end()) break;

                float distance = Vector2Distance(fireballIt->position, enemyIt->position);

                if (distance < fireballIt->radius + enemyIt->radius) {
                    enemyIt->health -= fireballIt->damage;
                    hitEnemy = true;

                    if (enemyIt->health <= 0) {
                        switch (enemyIt->type) {
                        case ENEMY_GREEN: score += 10; break;
                        case ENEMY_PURPLE: score += 20; break;
                        case ENEMY_RED: score += 50; break;
                        }
                        enemyIt = enemies.erase(enemyIt);
                        continue;
                    }
                    break;
                }
                ++enemyIt;
            }

            if (hitEnemy) {
                fireballIt->exploded = true;
                ++fireballIt;
            }
            else {
                ++fireballIt;
            }
        }
    }

    // Вражеские снаряды - игрок
    for (auto& enemy : enemies) {
        for (auto projIt = enemy.projectiles.begin(); projIt != enemy.projectiles.end();) {
            float distance = Vector2Distance(projIt->position, player.position);

            if (distance < projIt->radius + player.radius) {
                player.health -= projIt->damage;
                projIt = enemy.projectiles.erase(projIt);
            }
            else {
                ++projIt;
            }
        }
    }

    // Враги - игрок (ближний бой)
    for (auto& enemy : enemies) {
        float distance = Vector2Distance(enemy.position, player.position);

        if (distance < enemy.radius + player.radius) {
            player.health -= enemy.damage;
        }
    }

    // Улучшения - игрок
    for (auto upgradeIt = upgrades.begin(); upgradeIt != upgrades.end();) {
        float distance = Vector2Distance(upgradeIt->position, player.position);

        if (distance < upgradeIt->radius + player.radius) {
            ApplyUpgrade(*upgradeIt, player, meta);
            upgradeIt = upgrades.erase(upgradeIt);
        }
        else {
            ++upgradeIt;
        }
    }
}

// Функции для меню улучшений (расширенное с бесконечной прокачкой)
void DrawUpgradeMenu(MetaProgression& meta, Button& healthButton, Button& damageButton, Button& speedButton,
    Button& attackSpeedButton, Button& projectileCountButton, Button& bombAbilityButton,
    Button& freezeAbilityButton, Button& waveAbilityButton, Button& resetButton, Button& backButton) {
    BeginDrawing();
    ClearBackground(BLACK);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    DrawText("UPGRADES", screenWidth / 2 - MeasureText("UPGRADES", 40) / 2, 30, 40, WHITE);
    DrawText(TextFormat("Available Points: %d", meta.availablePoints), screenWidth / 2 - MeasureText(TextFormat("Available Points: %d", meta.availablePoints), 25) / 2, 90, 25, YELLOW);
    DrawText(TextFormat("Total Points: %d", meta.totalPoints), screenWidth / 2 - MeasureText(TextFormat("Total Points: %d", meta.totalPoints), 20) / 2, 120, 20, LIGHTGRAY);

    int yPos = 160;

    // Кнопка здоровья (бесконечная)
    healthButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    if (meta.availablePoints >= meta.GetHealthCost()) {
        healthButton.text = TextFormat("Health (Level %d) - Cost: %d", meta.healthLevel, meta.GetHealthCost());
        DrawButton(healthButton);
    }
    else {
        healthButton.text = TextFormat("Health (Level %d) - Need: %d", meta.healthLevel, meta.GetHealthCost());
        DrawRectangleRec(healthButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
        DrawRectangleLinesEx(healthButton.bounds, 2, WHITE);
        DrawText(healthButton.text, healthButton.bounds.x + (healthButton.bounds.width - MeasureText(healthButton.text, 18)) / 2,
            healthButton.bounds.y + (healthButton.bounds.height - 18) / 2, 18, GRAY);
    }
    yPos += 50;

    // Кнопка урона (бесконечная)
    damageButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    if (meta.availablePoints >= meta.GetDamageCost()) {
        damageButton.text = TextFormat("Damage (Level %d) - Cost: %d", meta.damageLevel, meta.GetDamageCost());
        DrawButton(damageButton);
    }
    else {
        damageButton.text = TextFormat("Damage (Level %d) - Need: %d", meta.damageLevel, meta.GetDamageCost());
        DrawRectangleRec(damageButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
        DrawRectangleLinesEx(damageButton.bounds, 2, WHITE);
        DrawText(damageButton.text, damageButton.bounds.x + (damageButton.bounds.width - MeasureText(damageButton.text, 18)) / 2,
            damageButton.bounds.y + (damageButton.bounds.height - 18) / 2, 18, GRAY);
    }
    yPos += 50;

    // Кнопка скорости (бесконечная)
    speedButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    if (meta.availablePoints >= meta.GetSpeedCost()) {
        speedButton.text = TextFormat("Speed (Level %d) - Cost: %d", meta.speedLevel, meta.GetSpeedCost());
        DrawButton(speedButton);
    }
    else {
        speedButton.text = TextFormat("Speed (Level %d) - Need: %d", meta.speedLevel, meta.GetSpeedCost());
        DrawRectangleRec(speedButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
        DrawRectangleLinesEx(speedButton.bounds, 2, WHITE);
        DrawText(speedButton.text, speedButton.bounds.x + (speedButton.bounds.width - MeasureText(speedButton.text, 18)) / 2,
            speedButton.bounds.y + (speedButton.bounds.height - 18) / 2, 18, GRAY);
    }
    yPos += 50;

    // Кнопка скорости атаки (бесконечная)
    attackSpeedButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    if (meta.availablePoints >= meta.GetAttackSpeedCost()) {
        attackSpeedButton.text = TextFormat("Attack Speed (Level %d) - Cost: %d", meta.attackSpeedLevel, meta.GetAttackSpeedCost());
        DrawButton(attackSpeedButton);
    }
    else {
        attackSpeedButton.text = TextFormat("Attack Speed (Level %d) - Need: %d", meta.attackSpeedLevel, meta.GetAttackSpeedCost());
        DrawRectangleRec(attackSpeedButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
        DrawRectangleLinesEx(attackSpeedButton.bounds, 2, WHITE);
        DrawText(attackSpeedButton.text, attackSpeedButton.bounds.x + (attackSpeedButton.bounds.width - MeasureText(attackSpeedButton.text, 18)) / 2,
            attackSpeedButton.bounds.y + (attackSpeedButton.bounds.height - 18) / 2, 18, GRAY);
    }
    yPos += 50;

    // Кнопка количества снарядов (бесконечная)
    projectileCountButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    if (meta.availablePoints >= meta.GetProjectileCountCost()) {
        projectileCountButton.text = TextFormat("Projectiles (Level %d) - Cost: %d", meta.projectileCountLevel, meta.GetProjectileCountCost());
        DrawButton(projectileCountButton);
    }
    else {
        projectileCountButton.text = TextFormat("Projectiles (Level %d) - Need: %d", meta.projectileCountLevel, meta.GetProjectileCountCost());
        DrawRectangleRec(projectileCountButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
        DrawRectangleLinesEx(projectileCountButton.bounds, 2, WHITE);
        DrawText(projectileCountButton.text, projectileCountButton.bounds.x + (projectileCountButton.bounds.width - MeasureText(projectileCountButton.text, 18)) / 2,
            projectileCountButton.bounds.y + (projectileCountButton.bounds.height - 18) / 2, 18, GRAY);
    }
    yPos += 60;

    // Премиум способности (покупаются один раз)
    DrawText("Premium Abilities:", 50, yPos, 22, GOLD);
    yPos += 35;

    // Бомба
    bombAbilityButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    if (!meta.hasBombAbility) {
        if (meta.availablePoints >= meta.GetBombAbilityCost()) {
            bombAbilityButton.text = TextFormat("Bomb Ability - Cost: %d", meta.GetBombAbilityCost());
            DrawButton(bombAbilityButton);
        }
        else {
            bombAbilityButton.text = TextFormat("Bomb Ability - Need: %d", meta.GetBombAbilityCost());
            DrawRectangleRec(bombAbilityButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
            DrawRectangleLinesEx(bombAbilityButton.bounds, 2, WHITE);
            DrawText(bombAbilityButton.text, bombAbilityButton.bounds.x + (bombAbilityButton.bounds.width - MeasureText(bombAbilityButton.text, 18)) / 2,
                bombAbilityButton.bounds.y + (bombAbilityButton.bounds.height - 18) / 2, 18, GRAY);
        }
    }
    else {
        bombAbilityButton.text = "Bomb Ability - PURCHASED";
        DrawRectangleRec(bombAbilityButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
        DrawRectangleLinesEx(bombAbilityButton.bounds, 2, WHITE);
        DrawText(bombAbilityButton.text, bombAbilityButton.bounds.x + (bombAbilityButton.bounds.width - MeasureText(bombAbilityButton.text, 18)) / 2,
            bombAbilityButton.bounds.y + (bombAbilityButton.bounds.height - 18) / 2, 18, GREEN);
    }
    yPos += 50;

    // Заморозка
    freezeAbilityButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    if (!meta.hasFreezeAbility) {
        if (meta.availablePoints >= meta.GetFreezeAbilityCost()) {
            freezeAbilityButton.text = TextFormat("Freeze Ability - Cost: %d", meta.GetFreezeAbilityCost());
            DrawButton(freezeAbilityButton);
        }
        else {
            freezeAbilityButton.text = TextFormat("Freeze Ability - Need: %d", meta.GetFreezeAbilityCost());
            DrawRectangleRec(freezeAbilityButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
            DrawRectangleLinesEx(freezeAbilityButton.bounds, 2, WHITE);
            DrawText(freezeAbilityButton.text, freezeAbilityButton.bounds.x + (freezeAbilityButton.bounds.width - MeasureText(freezeAbilityButton.text, 18)) / 2,
                freezeAbilityButton.bounds.y + (freezeAbilityButton.bounds.height - 18) / 2, 18, GRAY);
        }
    }
    else {
        freezeAbilityButton.text = "Freeze Ability - PURCHASED";
        DrawRectangleRec(freezeAbilityButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
        DrawRectangleLinesEx(freezeAbilityButton.bounds, 2, WHITE);
        DrawText(freezeAbilityButton.text, freezeAbilityButton.bounds.x + (freezeAbilityButton.bounds.width - MeasureText(freezeAbilityButton.text, 18)) / 2,
            freezeAbilityButton.bounds.y + (freezeAbilityButton.bounds.height - 18) / 2, 18, GREEN);
    }
    yPos += 50;

    // Волна
    waveAbilityButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    if (!meta.hasWaveAbility) {
        if (meta.availablePoints >= meta.GetWaveAbilityCost()) {
            waveAbilityButton.text = TextFormat("Wave Ability - Cost: %d", meta.GetWaveAbilityCost());
            DrawButton(waveAbilityButton);
        }
        else {
            waveAbilityButton.text = TextFormat("Wave Ability - Need: %d", meta.GetWaveAbilityCost());
            DrawRectangleRec(waveAbilityButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
            DrawRectangleLinesEx(waveAbilityButton.bounds, 2, WHITE);
            DrawText(waveAbilityButton.text, waveAbilityButton.bounds.x + (waveAbilityButton.bounds.width - MeasureText(waveAbilityButton.text, 18)) / 2,
                waveAbilityButton.bounds.y + (waveAbilityButton.bounds.height - 18) / 2, 18, GRAY);
        }
    }
    else {
        waveAbilityButton.text = "Wave Ability - PURCHASED";
        DrawRectangleRec(waveAbilityButton.bounds, COLOR_UPGRADE_BUTTON_MAXED);
        DrawRectangleLinesEx(waveAbilityButton.bounds, 2, WHITE);
        DrawText(waveAbilityButton.text, waveAbilityButton.bounds.x + (waveAbilityButton.bounds.width - MeasureText(waveAbilityButton.text, 18)) / 2,
            waveAbilityButton.bounds.y + (waveAbilityButton.bounds.height - 18) / 2, 18, GREEN);
    }
    yPos += 70;

    // Кнопка сброса прогресса
    resetButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(yPos), 300, 40 };
    resetButton.text = "Reset Progress (Get 50% points back)";
    DrawButton(resetButton);
    yPos += 60;

    // Кнопка назад
    backButton.bounds = { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(yPos), 200, 50 };
    backButton.text = "Back to Menu";
    DrawButton(backButton);

    // Отображение текущих бонусов
    DrawText(TextFormat("Current Bonuses:"), 50, screenHeight - 150, 20, WHITE);
    DrawText(TextFormat("Health: +%.0f%%", (meta.GetHealthBonus() - 1.0f) * 100), 70, screenHeight - 120, 18, GREEN);
    DrawText(TextFormat("Damage: +%.0f%%", (meta.GetDamageBonus() - 1.0f) * 100), 70, screenHeight - 95, 18, ORANGE);
    DrawText(TextFormat("Speed: +%.0f%%", (meta.GetSpeedBonus() - 1.0f) * 100), 70, screenHeight - 70, 18, WHITE);
    DrawText(TextFormat("Attack Speed: +%.0f%%", (meta.GetAttackSpeedBonus() - 1.0f) * 100), 70, screenHeight - 45, 18, SKYBLUE);
    DrawText(TextFormat("Projectile Count: %d", meta.GetProjectileCount()), 70, screenHeight - 20, 18, GOLD);

    EndDrawing();
}

// Основная функция игры
int main() {
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Survival Shooter");
    SetTargetFPS(TARGET_FPS);
    HideCursor();

    GameState gameState = MAIN_MENU;
    MetaProgression meta = { 0, 0, 0, 0, 0, 0, 0, false, false, false };

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    Button startButton = { { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(screenHeight) / 2 - 25, 200, 50 }, "Start Game", false };
    Button upgradeButton = { { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(screenHeight) / 2 + 50, 200, 50 }, "Upgrades", false };
    Button exitButton = { { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(screenHeight) / 2 + 125, 200, 50 }, "Exit", false };

    Button healthButton = { { 0, 0, 0, 0 }, "", false };
    Button damageButton = { { 0, 0, 0, 0 }, "", false };
    Button speedButton = { { 0, 0, 0, 0 }, "", false };
    Button attackSpeedButton = { { 0, 0, 0, 0 }, "", false };
    Button projectileCountButton = { { 0, 0, 0, 0 }, "", false };
    Button bombAbilityButton = { { 0, 0, 0, 0 }, "", false };
    Button freezeAbilityButton = { { 0, 0, 0, 0 }, "", false };
    Button waveAbilityButton = { { 0, 0, 0, 0 }, "", false };
    Button resetButton = { { 0, 0, 0, 0 }, "", false };
    Button backButton = { { 0, 0, 0, 0 }, "", false };

    Button confirmResetButton = { { 0, 0, 0, 0 }, "Confirm Reset", false };
    Button cancelResetButton = { { 0, 0, 0, 0 }, "Cancel", false };

    Player player;
    Joystick joystick;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Upgrade> upgrades;
    std::vector<Shockwave> shockwaves;
    std::vector<Bomb> bombs;
    std::vector<FreezeArea> freezeAreas;
    std::vector<Fireball> fireballs;

    double lastEnemySpawnTime = 0;
    double enemySpawnCooldown = 2.0;
    int score = 0;
    double gameTime = 0;
    float difficultyScale = 0.0f;
    int waveNumber = 1;
    int enemiesPerWave = 5;
    int enemiesSpawnedThisWave = 0;
    bool waveInProgress = false;

    while (!WindowShouldClose()) {
        double currentTime = GetTime();
        double deltaTime = GetFrameTime();
        deltaTime = std::min(deltaTime, 0.1);

        switch (gameState) {
        case MAIN_MENU: {
            IsButtonHovered(startButton);
            IsButtonHovered(upgradeButton);
            IsButtonHovered(exitButton);

            if (IsButtonClicked(startButton)) {
                try {
                    player = CreatePlayer(meta);
                    joystick = CreateJoystick();
                    bullets.clear();
                    enemies.clear();
                    upgrades.clear();
                    shockwaves.clear();
                    bombs.clear();
                    freezeAreas.clear();
                    fireballs.clear();
                    score = 0;
                    gameTime = 0;
                    difficultyScale = 0.0f;
                    waveNumber = 1;
                    enemiesPerWave = 5;
                    enemiesSpawnedThisWave = 0;
                    waveInProgress = true;
                    lastEnemySpawnTime = 0;
                    gameState = PLAYING;
                }
                catch (...) {
                    gameState = MAIN_MENU;
                }
            }

            if (IsButtonClicked(upgradeButton)) {
                gameState = UPGRADE_MENU;
            }

            if (IsButtonClicked(exitButton)) {
                CloseWindow();
                return 0;
            }

            BeginDrawing();
            ClearBackground(BLACK);

            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();

            DrawText("SURVIVAL SHOOTER", screenWidth / 2 - MeasureText("SURVIVAL SHOOTER", 40) / 2, 100, 40, WHITE);
            DrawText(TextFormat("Total Points: %d", meta.totalPoints), screenWidth / 2 - MeasureText(TextFormat("Total Points: %d", meta.totalPoints), 25) / 2, 160, 25, YELLOW);

            startButton.bounds = { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(screenHeight) / 2 - 25, 200, 50 };
            upgradeButton.bounds = { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(screenHeight) / 2 + 50, 200, 50 };
            exitButton.bounds = { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(screenHeight) / 2 + 125, 200, 50 };

            DrawButton(startButton);
            DrawButton(upgradeButton);
            DrawButton(exitButton);

            EndDrawing();
            break;
        }

        case UPGRADE_MENU: {
            IsButtonHovered(healthButton);
            IsButtonHovered(damageButton);
            IsButtonHovered(speedButton);
            IsButtonHovered(attackSpeedButton);
            IsButtonHovered(projectileCountButton);
            IsButtonHovered(bombAbilityButton);
            IsButtonHovered(freezeAbilityButton);
            IsButtonHovered(waveAbilityButton);
            IsButtonHovered(resetButton);
            IsButtonHovered(backButton);

            // Бесконечные улучшения
            if (IsButtonClicked(healthButton) && meta.availablePoints >= meta.GetHealthCost()) {
                meta.availablePoints -= meta.GetHealthCost();
                meta.healthLevel++;
            }

            if (IsButtonClicked(damageButton) && meta.availablePoints >= meta.GetDamageCost()) {
                meta.availablePoints -= meta.GetDamageCost();
                meta.damageLevel++;
            }

            if (IsButtonClicked(speedButton) && meta.availablePoints >= meta.GetSpeedCost()) {
                meta.availablePoints -= meta.GetSpeedCost();
                meta.speedLevel++;
            }

            if (IsButtonClicked(attackSpeedButton) && meta.availablePoints >= meta.GetAttackSpeedCost()) {
                meta.availablePoints -= meta.GetAttackSpeedCost();
                meta.attackSpeedLevel++;
            }

            if (IsButtonClicked(projectileCountButton) && meta.availablePoints >= meta.GetProjectileCountCost()) {
                meta.availablePoints -= meta.GetProjectileCountCost();
                meta.projectileCountLevel++;
            }

            // Премиум способности (покупаются один раз)
            if (IsButtonClicked(bombAbilityButton) && !meta.hasBombAbility && meta.availablePoints >= meta.GetBombAbilityCost()) {
                meta.availablePoints -= meta.GetBombAbilityCost();
                meta.hasBombAbility = true;
            }

            if (IsButtonClicked(freezeAbilityButton) && !meta.hasFreezeAbility && meta.availablePoints >= meta.GetFreezeAbilityCost()) {
                meta.availablePoints -= meta.GetFreezeAbilityCost();
                meta.hasFreezeAbility = true;
            }

            if (IsButtonClicked(waveAbilityButton) && !meta.hasWaveAbility && meta.availablePoints >= meta.GetWaveAbilityCost()) {
                meta.availablePoints -= meta.GetWaveAbilityCost();
                meta.hasWaveAbility = true;
            }

            if (IsButtonClicked(resetButton)) {
                gameState = RESET_CONFIRM;
            }

            if (IsButtonClicked(backButton)) {
                gameState = MAIN_MENU;
            }

            DrawUpgradeMenu(meta, healthButton, damageButton, speedButton, attackSpeedButton,
                projectileCountButton, bombAbilityButton, freezeAbilityButton,
                waveAbilityButton, resetButton, backButton);
            break;
        }

        case RESET_CONFIRM: {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();

            confirmResetButton.bounds = { static_cast<float>(screenWidth) / 2 - 150, static_cast<float>(screenHeight) / 2 + 20, 140, 50 };
            cancelResetButton.bounds = { static_cast<float>(screenWidth) / 2 + 10, static_cast<float>(screenHeight) / 2 + 20, 140, 50 };

            IsButtonHovered(confirmResetButton);
            IsButtonHovered(cancelResetButton);

            if (IsButtonClicked(confirmResetButton)) {
                meta.ResetProgress();
                gameState = UPGRADE_MENU;
            }

            if (IsButtonClicked(cancelResetButton)) {
                gameState = UPGRADE_MENU;
            }

            BeginDrawing();
            ClearBackground(BLACK);

            DrawRectangle(0, 0, screenWidth, screenHeight, { 0, 0, 0, 200 });

            DrawText("Reset Progress?", screenWidth / 2 - MeasureText("Reset Progress?", 40) / 2, screenHeight / 2 - 80, 40, YELLOW);
            DrawText("You will get 50% of your total points back", screenWidth / 2 - MeasureText("You will get 50% of your total points back", 25) / 2, screenHeight / 2 - 20, 25, WHITE);
            DrawText("Premium abilities will be kept", screenWidth / 2 - MeasureText("Premium abilities will be kept", 25) / 2, screenHeight / 2 + 10, 25, WHITE);

            DrawButton(confirmResetButton);
            DrawButton(cancelResetButton);

            EndDrawing();
            break;
        }

        case PLAYING: {
            gameTime += deltaTime;
            difficultyScale = std::min(1.0f, (float)gameTime / 300.0f);

            UpdateJoystick(joystick);

            // Система волн с бесконечным усложнением
            if (enemies.empty() && !waveInProgress) {
                waveNumber++;
                enemiesPerWave = 5 + waveNumber * 2;
                enemiesSpawnedThisWave = 0;
                waveInProgress = true;
                lastEnemySpawnTime = currentTime;
            }

            // Спавн врагов в волнах
            if (waveInProgress && currentTime - lastEnemySpawnTime > enemySpawnCooldown && enemiesSpawnedThisWave < enemiesPerWave) {
                Vector2 spawnPos;
                int side = GetRandomValue(0, 3);

                screenWidth = GetScreenWidth();
                screenHeight = GetScreenHeight();

                switch (side) {
                case 0: spawnPos = { (float)GetRandomValue(0, screenWidth), -20 }; break;
                case 1: spawnPos = { (float)screenWidth + 20, (float)GetRandomValue(0, screenHeight) }; break;
                case 2: spawnPos = { (float)GetRandomValue(0, screenWidth), (float)screenHeight + 20 }; break;
                case 3: spawnPos = { -20, (float)GetRandomValue(0, screenHeight) }; break;
                }

                int enemyType = GetRandomValue(0, 2);
                // Передаем waveNumber для бесконечного усложнения
                enemies.push_back(CreateEnemy((EnemyType)enemyType, spawnPos, difficultyScale, waveNumber));

                lastEnemySpawnTime = currentTime;
                enemiesSpawnedThisWave++;

                if (enemiesSpawnedThisWave >= enemiesPerWave) {
                    waveInProgress = false;
                }

                enemySpawnCooldown = std::max(0.3, enemySpawnCooldown * 0.99);
            }

            // Спавн улучшений
            if (GetRandomValue(0, 1000) < 2) {
                screenWidth = GetScreenWidth();
                screenHeight = GetScreenHeight();
                Vector2 spawnPos = {
                    (float)GetRandomValue(50, screenWidth - 50),
                    (float)GetRandomValue(50, screenHeight - 50)
                };
                upgrades.push_back(CreateUpgrade(spawnPos));
            }

            // Обновление игровых объектов
            UpdatePlayer(player, joystick, currentTime, bullets, enemies, shockwaves, bombs, freezeAreas, fireballs);
            UpdateBullets(bullets);
            UpdateEnemies(enemies, player, currentTime, freezeAreas);
            UpdateShockwaves(shockwaves);
            UpdateBombs(bombs, deltaTime);
            UpdateFreezeAreas(freezeAreas, deltaTime);
            UpdateFireballs(fireballs, enemies, deltaTime);

            CheckCollisions(player, bullets, enemies, upgrades, shockwaves, bombs, freezeAreas, fireballs, score, meta);

            // Дополнительные очки за выживание
            score += (int)(deltaTime);

            if (player.health <= 0) {
                int pointsEarned = std::max(1, score / 10); // Очки основаны на score
                meta.AddPoints(pointsEarned);
                gameState = GAME_OVER;
            }

            BeginDrawing();
            ClearBackground(BLACK);

            DrawShockwaves(shockwaves);
            DrawBombs(bombs);
            DrawFreezeAreas(freezeAreas);
            DrawFireballs(fireballs);
            DrawBullets(bullets);
            DrawEnemies(enemies);
            DrawUpgrades(upgrades);
            DrawPlayer(player);
            DrawJoystick(joystick);

            // Отрисовка UI
            DrawText(TextFormat("Health: %d/%d", player.health, player.maxHealth), 10, 10, 20, WHITE);
            DrawText(TextFormat("Score: %d", score), 10, 40, 20, WHITE);
            DrawText(TextFormat("Time: %.1f", gameTime), 10, 70, 20, WHITE);
            DrawText(TextFormat("Wave: %d", waveNumber), 10, 100, 20, ORANGE);
            DrawText(TextFormat("Enemies: %d/%d", enemies.size(), enemiesPerWave), 10, 130, 20, ORANGE);
            DrawText(TextFormat("Projectiles: %d", player.projectileCount), 10, 160, 20, GOLD);

            int yPos = 190;
            if (player.hasWaveAttack) {
                double waveCooldownRemaining = player.waveCooldown - (currentTime - player.lastWaveTime);
                if (waveCooldownRemaining < 0) waveCooldownRemaining = 0;
                DrawText(TextFormat("Wave: %.1f", waveCooldownRemaining), 10, yPos, 20, COLOR_UPGRADE_WAVE);
                yPos += 25;
            }
            if (player.hasBombAttack) {
                double bombCooldownRemaining = player.bombCooldown - (currentTime - player.lastBombTime);
                if (bombCooldownRemaining < 0) bombCooldownRemaining = 0;
                DrawText(TextFormat("Bomb: %.1f", bombCooldownRemaining), 10, yPos, 20, COLOR_UPGRADE_BOMB);
                yPos += 25;
            }
            if (player.hasFreezeAttack) {
                double freezeCooldownRemaining = player.freezeCooldown - (currentTime - player.lastFreezeTime);
                if (freezeCooldownRemaining < 0) freezeCooldownRemaining = 0;
                DrawText(TextFormat("Freeze: %.1f", freezeCooldownRemaining), 10, yPos, 20, COLOR_UPGRADE_FREEZE);
                yPos += 25;
            }
            if (player.hasFireballAttack) {
                double fireballCooldownRemaining = player.fireballCooldown - (currentTime - player.lastFireballTime);
                if (fireballCooldownRemaining < 0) fireballCooldownRemaining = 0;
                DrawText(TextFormat("Fireball: %.1f", fireballCooldownRemaining), 10, yPos, 20, COLOR_UPGRADE_FIREBALL);
                yPos += 25;
            }
            if (player.hasDoubleShot) {
                DrawText("Double Shot", 10, yPos, 20, COLOR_UPGRADE_DOUBLE_SHOT);
            }

            EndDrawing();
            break;
        }

        case GAME_OVER: {
            screenWidth = GetScreenWidth();
            screenHeight = GetScreenHeight();

            Button restartButton = { { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(screenHeight) / 2, 200, 50 }, "Play Again", false };
            Button menuButton = { { static_cast<float>(screenWidth) / 2 - 100, static_cast<float>(screenHeight) / 2 + 70, 200, 50 }, "Main Menu", false };

            IsButtonHovered(restartButton);
            IsButtonHovered(menuButton);

            if (IsButtonClicked(restartButton)) {
                try {
                    player = CreatePlayer(meta);
                    joystick = CreateJoystick();
                    bullets.clear();
                    enemies.clear();
                    upgrades.clear();
                    shockwaves.clear();
                    bombs.clear();
                    freezeAreas.clear();
                    fireballs.clear();
                    score = 0;
                    gameTime = 0;
                    difficultyScale = 0.0f;
                    waveNumber = 1;
                    enemiesPerWave = 5;
                    enemiesSpawnedThisWave = 0;
                    waveInProgress = true;
                    lastEnemySpawnTime = 0;
                    gameState = PLAYING;
                }
                catch (...) {
                    gameState = MAIN_MENU;
                }
            }

            if (IsButtonClicked(menuButton)) {
                gameState = MAIN_MENU;
            }

            BeginDrawing();
            ClearBackground(BLACK);

            DrawRectangle(0, 0, screenWidth, screenHeight, { 0, 0, 0, 200 });

            DrawText("GAME OVER", screenWidth / 2 - MeasureText("GAME OVER", 50) / 2, 150, 50, RED);
            DrawText(TextFormat("Final Score: %d", score), screenWidth / 2 - MeasureText(TextFormat("Final Score: %d", score), 30) / 2, 220, 30, WHITE);
            DrawText(TextFormat("Survival Time: %.1f seconds", gameTime), screenWidth / 2 - MeasureText(TextFormat("Survival Time: %.1f seconds", gameTime), 25) / 2, 260, 25, WHITE);
            DrawText(TextFormat("Wave Reached: %d", waveNumber), screenWidth / 2 - MeasureText(TextFormat("Wave Reached: %d", waveNumber), 25) / 2, 290, 25, ORANGE);
            DrawText(TextFormat("Points Earned: %d", std::max(1, score / 10)), screenWidth / 2 - MeasureText(TextFormat("Points Earned: %d", std::max(1, score / 10)), 25) / 2, 320, 25, YELLOW);

            DrawButton(restartButton);
            DrawButton(menuButton);

            EndDrawing();
            break;
        }
        }
    }

    CloseWindow();
    return 0;
}