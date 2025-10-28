#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include "raylib.h"

// Константы игры
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TARGET_FPS = 60;

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
const Color COLOR_JOYSTICK_BG = { 100, 100, 100, 150 };
const Color COLOR_JOYSTICK = { 200, 200, 200, 200 };
const Color COLOR_WAVE_ATTACK = { 0, 100, 255, 150 };
const Color COLOR_BOMB = { 100, 0, 0, 255 };
const Color COLOR_EXPLOSION = { 255, 165, 0, 150 };
const Color COLOR_FREEZE = { 100, 200, 255, 150 };
const Color COLOR_FIREBALL = { 255, 69, 0, 255 };
const Color COLOR_FIREBALL_EXPLOSION = { 255, 140, 0, 150 };

// Состояния игры
enum GameState {
    MAIN_MENU,
    PLAYING,
    GAME_OVER
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
    UPGRADE_FIREBALL
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
    joystick.outerRadius = 60.0f;
    joystick.innerRadius = 25.0f;
    joystick.position = { joystick.outerRadius + 20, SCREEN_HEIGHT - joystick.outerRadius - 20 };
    joystick.touchPosition = joystick.position;
    joystick.isActive = false;
    joystick.direction = { 0, 0 };
    return joystick;
}

void UpdateJoystick(Joystick& joystick) {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 touchPos = GetMousePosition();
        float distance = Vector2Distance(touchPos, joystick.position);

        if (!joystick.isActive && distance < joystick.outerRadius) {
            joystick.isActive = true;
        }

        if (joystick.isActive) {
            Vector2 direction = { touchPos.x - joystick.position.x, touchPos.y - joystick.position.y };
            float dist = Vector2Length(direction);

            if (dist > joystick.outerRadius - joystick.innerRadius) {
                float scale = (joystick.outerRadius - joystick.innerRadius) / dist;
                direction = Vector2Scale(direction, scale);
            }

            joystick.touchPosition = Vector2Add(joystick.position, direction);

            if (dist > 0) {
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
Player CreatePlayer() {
    Player player;
    player.position = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    player.radius = 15.0f;
    player.speed = 5.0f;
    player.health = 100;
    player.maxHealth = 100;
    player.attackSpeed = 1.0f;
    player.damage = 20;
    player.lastShotTime = -1.0;

    player.hasWaveAttack = false;
    player.waveCooldown = 45.0;
    player.lastWaveTime = -45.0;
    player.waveDamage = 50;

    player.hasDoubleShot = false;

    player.hasBombAttack = false;
    player.bombCooldown = 15.0;
    player.lastBombTime = -15.0;
    player.bombDamage = 40;
    player.bombRadius = 80.0f;

    player.hasFreezeAttack = false;
    player.freezeCooldown = 25.0;
    player.lastFreezeTime = -25.0;
    player.freezeDuration = 3.0f;
    player.freezeRadius = 100.0f;

    player.hasFireballAttack = false;
    player.fireballCooldown = 8.0;
    player.lastFireballTime = -8.0;
    player.fireballDamage = 30; // Базовый урон + зависит от урона игрока
    player.fireballExplosionRadius = 60.0f;
    player.fireballSpeed = 8.0f;

    return player;
}

void UpdatePlayer(Player& player, const Joystick& joystick, double currentTime, std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, std::vector<Shockwave>& shockwaves, std::vector<Bomb>& bombs, std::vector<FreezeArea>& freezeAreas, std::vector<Fireball>& fireballs) {
    Vector2 movement = { 0, 0 };

    if (IsKeyDown(KEY_A) && player.position.x - player.speed > 0) movement.x -= 1;
    if (IsKeyDown(KEY_D) && player.position.x + player.speed < SCREEN_WIDTH) movement.x += 1;
    if (IsKeyDown(KEY_W) && player.position.y - player.speed > 0) movement.y -= 1;
    if (IsKeyDown(KEY_S) && player.position.y + player.speed < SCREEN_HEIGHT) movement.y += 1;

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

        player.position.x = std::max(player.radius, std::min(SCREEN_WIDTH - player.radius, player.position.x));
        player.position.y = std::max(player.radius, std::min(SCREEN_HEIGHT - player.radius, player.position.y));
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

                // Основной выстрел
                Bullet bullet;
                bullet.position = player.position;
                bullet.velocity.x = direction.x * 10.0f;
                bullet.velocity.y = direction.y * 10.0f;
                bullet.radius = 5.0f;
                bullet.damage = player.damage;
                bullets.push_back(bullet);

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

                    secondBullet.radius = 5.0f;
                    secondBullet.damage = player.damage;
                    bullets.push_back(secondBullet);
                }

                player.lastShotTime = currentTime;
            }
        }
    }

    // Активация волновой атаки - направление в сторону наибольшего скопления врагов
    if (player.hasWaveAttack && currentTime - player.lastWaveTime > player.waveCooldown) {
        if (!enemies.empty()) {
            Shockwave shockwave;
            shockwave.position = player.position;
            shockwave.radius = 10.0f;

            // Находим направление в сторону наибольшего скопления врагов
            Vector2 averagePosition = { 0, 0 };
            int enemyCount = 0;

            // Считаем среднюю позицию всех врагов в определенном радиусе
            for (const auto& enemy : enemies) {
                float distance = Vector2Distance(player.position, enemy.position);
                if (distance < 300.0f) { // Рассматриваем только врагов в радиусе 300 пикселей
                    averagePosition.x += enemy.position.x;
                    averagePosition.y += enemy.position.y;
                    enemyCount++;
                }
            }

            Vector2 waveDirection = { 0, 1 }; // Направление по умолчанию (вниз)

            if (enemyCount > 0) {
                // Вычисляем среднюю позицию
                averagePosition.x /= enemyCount;
                averagePosition.y /= enemyCount;

                // Направление от игрока к средней позиции врагов
                waveDirection = Vector2Subtract(averagePosition, player.position);
                waveDirection = Vector2Normalize(waveDirection);
            }
            else {
                // Если нет врагов в радиусе, ищем ближайшего врага
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
        // Ищем ближайшего врага для направления фаербола
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

            // Направление к ближайшему врагу
            Vector2 direction = Vector2Subtract(nearest_enemy->position, player.position);
            direction = Vector2Normalize(direction);

            fireball.velocity.x = direction.x * player.fireballSpeed;
            fireball.velocity.y = direction.y * player.fireballSpeed;
            fireball.radius = 8.0f;
            fireball.damage = player.fireballDamage + player.damage / 2; // Урон зависит от базовой атаки
            fireball.explosionRadius = player.fireballExplosionRadius;
            fireball.active = true;
            fireball.exploded = false;

            fireballs.push_back(fireball);
            player.lastFireballTime = currentTime;
        }
    }
}

void DrawPlayer(const Player& player) {
    DrawCircleV(player.position, player.radius, COLOR_PLAYER);

    float healthBarWidth = 40.0f;
    float healthBarHeight = 5.0f;
    Vector2 healthBarPos = {
        player.position.x - healthBarWidth / 2,
        player.position.y - player.radius - 10
    };

    DrawRectangle((int)healthBarPos.x, (int)healthBarPos.y, (int)healthBarWidth, (int)healthBarHeight, RED);
    DrawRectangle((int)healthBarPos.x, (int)healthBarPos.y, (int)(healthBarWidth * (player.health / (float)player.maxHealth)), (int)healthBarHeight, GREEN);
}

// Функции для пуль
void UpdateBullets(std::vector<Bullet>& bullets) {
    for (auto it = bullets.begin(); it != bullets.end();) {
        it->position.x += it->velocity.x;
        it->position.y += it->velocity.y;

        if (it->position.x < 0 || it->position.x > SCREEN_WIDTH ||
            it->position.y < 0 || it->position.y > SCREEN_HEIGHT) {
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
        it->position.x += it->direction.x * it->speed;
        it->position.y += it->direction.y * it->speed;
        it->radius += 0.5f;

        if (it->position.x < -100 || it->position.x > SCREEN_WIDTH + 100 ||
            it->position.y < -100 || it->position.y > SCREEN_HEIGHT + 100 ||
            it->radius > 200) {
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
        it->timer -= (float)deltaTime;

        if (it->timer <= 0) {
            it = bombs.erase(it);
        }
        else {
            ++it;
        }
    }
}

void DrawBombs(const std::vector<Bomb>& bombs) {
    for (const auto& bomb : bombs) {
        Color bombColor = COLOR_BOMB;
        if (bomb.timer < 0.5f) {
            bombColor = (fmod(bomb.timer, 0.1f) > 0.05f) ? RED : COLOR_BOMB;
        }

        DrawCircleV(bomb.position, 8.0f, bombColor);

        if (bomb.timer < 1.0f) {
            DrawCircleLines((int)bomb.position.x, (int)bomb.position.y, (int)bomb.explosionRadius, COLOR_EXPLOSION);
        }
    }
}

// Функции для заморозки
void UpdateFreezeAreas(std::vector<FreezeArea>& freezeAreas, double deltaTime) {
    for (auto it = freezeAreas.begin(); it != freezeAreas.end();) {
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
        if (!it->exploded) {
            // Двигаем фаербол
            it->position.x += it->velocity.x;
            it->position.y += it->velocity.y;

            // Проверяем столкновение с врагами
            bool hitEnemy = false;
            for (auto& enemy : enemies) {
                float distance = Vector2Distance(it->position, enemy.position);
                if (distance < it->radius + enemy.radius) {
                    hitEnemy = true;
                    break;
                }
            }

            // Проверяем выход за границы или столкновение
            if (it->position.x < 0 || it->position.x > SCREEN_WIDTH ||
                it->position.y < 0 || it->position.y > SCREEN_HEIGHT ||
                hitEnemy) {
                it->exploded = true;
            }
        }
        else {
            // Фаербол взорвался, удаляем через короткое время
            it = fireballs.erase(it);
        }

        if (!it->exploded) {
            ++it;
        }
    }
}

void DrawFireballs(const std::vector<Fireball>& fireballs) {
    for (const auto& fireball : fireballs) {
        if (!fireball.exploded) {
            // Рисуем сам фаербол
            DrawCircleV(fireball.position, fireball.radius, COLOR_FIREBALL);

            // Эффект пламени
            for (int i = 0; i < 3; i++) {
                float offset = (float)GetTime() * 10.0f + i * 2.0f;
                float pulse = (sinf(offset) + 1.0f) * 0.5f;
                float trailRadius = fireball.radius * (0.7f + pulse * 0.3f);
                Color trailColor = { 255, 140, 0, (unsigned char)(150 * pulse) };
                DrawCircleV(fireball.position, trailRadius, trailColor);
            }
        }
        else {
            // Рисуем взрыв
            DrawCircleV(fireball.position, fireball.explosionRadius, COLOR_FIREBALL_EXPLOSION);
            DrawCircleLines((int)fireball.position.x, (int)fireball.position.y, (int)fireball.explosionRadius, ORANGE);
        }
    }
}

// Функции для врагов
Enemy CreateEnemy(EnemyType type, Vector2 position) {
    Enemy enemy;
    enemy.type = type;
    enemy.position = position;
    enemy.radius = 15.0f;
    enemy.isFrozen = false;
    enemy.frozenUntil = 0.0;

    switch (type) {
    case ENEMY_GREEN:
        enemy.color = COLOR_GREEN_ENEMY;
        enemy.speed = 2.5f;
        enemy.health = 30;
        enemy.maxHealth = 30;
        enemy.damage = 5;
        enemy.attackRange = 20.0f;
        enemy.isRanged = false;
        enemy.attackCooldown = 1.0f;
        break;

    case ENEMY_PURPLE:
        enemy.color = COLOR_PURPLE_ENEMY;
        enemy.speed = 1.0f;
        enemy.health = 50;
        enemy.maxHealth = 50;
        enemy.damage = 8;
        enemy.attackRange = 150.0f;
        enemy.isRanged = true;
        enemy.attackCooldown = 1.0f;
        break;

    case ENEMY_RED:
        enemy.color = COLOR_RED_ENEMY;
        enemy.speed = 0.8f;
        enemy.health = 150;
        enemy.maxHealth = 150;
        enemy.damage = 15;
        enemy.attackRange = 25.0f;
        enemy.isRanged = false;
        enemy.attackCooldown = 1.0f;
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
                    projectile.radius = 7.0f;
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
            it->position.x += it->velocity.x;
            it->position.y += it->velocity.y;

            if (it->position.x < 0 || it->position.x > SCREEN_WIDTH ||
                it->position.y < 0 || it->position.y > SCREEN_HEIGHT) {
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

        float healthBarWidth = 30.0f;
        float healthBarHeight = 4.0f;
        Vector2 healthBarPos = {
            enemy.position.x - healthBarWidth / 2,
            enemy.position.y - enemy.radius - 8
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
    upgrade.radius = 10.0f;

    int type = GetRandomValue(0, 8); // Теперь 9 типов улучшений
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
    }

    return upgrade;
}

void ApplyUpgrade(Upgrade& upgrade, Player& player) {
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
            player.bombRadius += 10.0f;
        }
        break;

    case UPGRADE_FREEZE:
        if (!player.hasFreezeAttack) {
            player.hasFreezeAttack = true;
        }
        else {
            player.freezeCooldown = std::max(12.0, player.freezeCooldown * 0.8f);
            player.freezeDuration += 0.5f;
            player.freezeRadius += 15.0f;
        }
        break;

    case UPGRADE_FIREBALL:
        if (!player.hasFireballAttack) {
            player.hasFireballAttack = true;
        }
        else {
            player.fireballCooldown = std::max(4.0, player.fireballCooldown * 0.8f);
            player.fireballDamage += 5;
            player.fireballExplosionRadius += 10.0f;
        }
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

// Проверка коллизий
void CheckCollisions(Player& player, std::vector<Bullet>& bullets, std::vector<Enemy>& enemies, std::vector<Upgrade>& upgrades, std::vector<Shockwave>& shockwaves, std::vector<Bomb>& bombs, std::vector<FreezeArea>& freezeAreas, std::vector<Fireball>& fireballs) {
    // Пули - враги
    for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
        bool bulletHit = false;

        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            float distance = Vector2Distance(bulletIt->position, enemyIt->position);

            if (distance < bulletIt->radius + enemyIt->radius) {
                enemyIt->health -= bulletIt->damage;
                bulletHit = true;

                if (enemyIt->health <= 0) {
                    enemyIt = enemies.erase(enemyIt);
                    continue;
                }
                break;
            }
            ++enemyIt;
        }

        if (bulletHit) {
            bulletIt = bullets.erase(bulletIt);
        }
        else {
            ++bulletIt;
        }
    }

    // Шоквейвы - враги
    for (auto waveIt = shockwaves.begin(); waveIt != shockwaves.end();) {
        bool waveHit = false;

        for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
            float distance = Vector2Distance(waveIt->position, enemyIt->position);

            if (distance < waveIt->radius + enemyIt->radius) {
                enemyIt->health -= waveIt->damage;

                if (enemyIt->health <= 0) {
                    enemyIt = enemies.erase(enemyIt);
                    continue;
                }
            }
            ++enemyIt;
        }
        ++waveIt;
    }

    // Бомбы - враги
    for (auto bombIt = bombs.begin(); bombIt != bombs.end();) {
        if (bombIt->timer <= 0) {
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                float distance = Vector2Distance(bombIt->position, enemyIt->position);

                if (distance < bombIt->explosionRadius + enemyIt->radius) {
                    enemyIt->health -= bombIt->damage;

                    if (enemyIt->health <= 0) {
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
            // Взрыв фаербола наносит урон по области
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                float distance = Vector2Distance(fireballIt->position, enemyIt->position);

                if (distance < fireballIt->explosionRadius + enemyIt->radius) {
                    enemyIt->health -= fireballIt->damage;

                    if (enemyIt->health <= 0) {
                        enemyIt = enemies.erase(enemyIt);
                        continue;
                    }
                }
                ++enemyIt;
            }
            fireballIt = fireballs.erase(fireballIt);
        }
        else {
            // Обычное столкновение фаербола с врагом
            bool hitEnemy = false;
            for (auto enemyIt = enemies.begin(); enemyIt != enemies.end();) {
                float distance = Vector2Distance(fireballIt->position, enemyIt->position);

                if (distance < fireballIt->radius + enemyIt->radius) {
                    enemyIt->health -= fireballIt->damage;
                    hitEnemy = true;

                    if (enemyIt->health <= 0) {
                        enemyIt = enemies.erase(enemyIt);
                        continue;
                    }
                    break;
                }
                ++enemyIt;
            }

            if (hitEnemy) {
                fireballIt->exploded = true;
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
            ApplyUpgrade(*upgradeIt, player);
            upgradeIt = upgrades.erase(upgradeIt);
        }
        else {
            ++upgradeIt;
        }
    }
}

// Основная функция игры
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Survival Shooter");
    SetTargetFPS(TARGET_FPS);

    GameState gameState = MAIN_MENU;

    // Создание кнопок
    Button startButton = { { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 25, 200, 50 }, "Start Game", false };
    Button exitButton = { { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50, 200, 50 }, "Exit", false };

    // Игровые объекты
    Player player;
    Joystick joystick;
    std::vector<Bullet> bullets;
    std::vector<Enemy> enemies;
    std::vector<Upgrade> upgrades;
    std::vector<Shockwave> shockwaves;
    std::vector<Bomb> bombs;
    std::vector<FreezeArea> freezeAreas;
    std::vector<Fireball> fireballs;

    // Игровые переменные
    double lastEnemySpawnTime = 0;
    double enemySpawnCooldown = 2.0;
    int score = 0;
    double gameTime = 0;

    while (!WindowShouldClose()) {
        double currentTime = GetTime();
        double deltaTime = GetFrameTime();

        switch (gameState) {
        case MAIN_MENU: {
            IsButtonHovered(startButton);
            IsButtonHovered(exitButton);

            if (IsButtonClicked(startButton)) {
                // Инициализация игры
                player = CreatePlayer();
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
                lastEnemySpawnTime = 0;
                gameState = PLAYING;
            }

            if (IsButtonClicked(exitButton)) {
                CloseWindow();
                return 0;
            }

            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("SURVIVAL SHOOTER", SCREEN_WIDTH / 2 - MeasureText("SURVIVAL SHOOTER", 40) / 2, 100, 40, WHITE);
            DrawButton(startButton);
            DrawButton(exitButton);

            EndDrawing();
            break;
        }

        case PLAYING: {
            gameTime += deltaTime;

            // Обновление джойстика
            UpdateJoystick(joystick);

            // Спавн врагов
            if (currentTime - lastEnemySpawnTime > enemySpawnCooldown) {
                Vector2 spawnPos;
                int side = GetRandomValue(0, 3);

                switch (side) {
                case 0: // Сверху
                    spawnPos = { (float)GetRandomValue(0, SCREEN_WIDTH), -20 };
                    break;
                case 1: // Справа
                    spawnPos = { (float)SCREEN_WIDTH + 20, (float)GetRandomValue(0, SCREEN_HEIGHT) };
                    break;
                case 2: // Снизу
                    spawnPos = { (float)GetRandomValue(0, SCREEN_WIDTH), (float)SCREEN_HEIGHT + 20 };
                    break;
                case 3: // Слева
                    spawnPos = { -20, (float)GetRandomValue(0, SCREEN_HEIGHT) };
                    break;
                }

                int enemyType = GetRandomValue(0, 2);
                enemies.push_back(CreateEnemy((EnemyType)enemyType, spawnPos));

                lastEnemySpawnTime = currentTime;
                enemySpawnCooldown = std::max(0.5, enemySpawnCooldown * 0.99);
            }

            // Спавн улучшений
            if (GetRandomValue(0, 1000) < 2) {
                Vector2 spawnPos = {
                    (float)GetRandomValue(50, SCREEN_WIDTH - 50),
                    (float)GetRandomValue(50, SCREEN_HEIGHT - 50)
                };
                upgrades.push_back(CreateUpgrade(spawnPos));
            }

            // Обновление игрока
            UpdatePlayer(player, joystick, currentTime, bullets, enemies, shockwaves, bombs, freezeAreas, fireballs);

            // Обновление пуль
            UpdateBullets(bullets);

            // Обновление врагов
            UpdateEnemies(enemies, player, currentTime, freezeAreas);

            // Обновление шоквейвов
            UpdateShockwaves(shockwaves);

            // Обновление бомб
            UpdateBombs(bombs, deltaTime);

            // Обновление заморозки
            UpdateFreezeAreas(freezeAreas, deltaTime);

            // Обновление фаерболов
            UpdateFireballs(fireballs, enemies, deltaTime);

            // Проверка коллизий
            CheckCollisions(player, bullets, enemies, upgrades, shockwaves, bombs, freezeAreas, fireballs);

            // Увеличение счета
            score += (int)(deltaTime * 10);

            // Проверка конца игры
            if (player.health <= 0) {
                gameState = GAME_OVER;
            }

            // Отрисовка
            BeginDrawing();
            ClearBackground(BLACK);

            // Отрисовка игровых объектов
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

            // Отображение способностей
            int yPos = 100;
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
            // Обновляем позиции кнопок для экрана Game Over
            Button restartButton = { { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 200, 50 }, "Play Again", false };
            Button menuButton = { { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 70, 200, 50 }, "Main Menu", false };

            IsButtonHovered(restartButton);
            IsButtonHovered(menuButton);

            if (IsButtonClicked(restartButton)) {
                // Перезапуск игры
                player = CreatePlayer();
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
                lastEnemySpawnTime = 0;
                gameState = PLAYING;
            }

            if (IsButtonClicked(menuButton)) {
                gameState = MAIN_MENU;
            }

            BeginDrawing();
            ClearBackground(BLACK);

            // Полупрозрачный overlay
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, { 0, 0, 0, 200 });

            DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 50) / 2, 150, 50, RED);
            DrawText(TextFormat("Final Score: %d", score), SCREEN_WIDTH / 2 - MeasureText(TextFormat("Final Score: %d", score), 30) / 2, 220, 30, WHITE);
            DrawText(TextFormat("Survival Time: %.1f seconds", gameTime), SCREEN_WIDTH / 2 - MeasureText(TextFormat("Survival Time: %.1f seconds", gameTime), 25) / 2, 260, 25, WHITE);

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