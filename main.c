#include "raylib.h"
#include "raymath.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef struct Invader
{
    Vector2 position;
    Vector2 velocity;
    Vector2 look_at_direction;
    float radius;
    float rotation;

    enum InvaderState
    {
        Idle,
        Moving,
        Dead
    } state;
} Invader;

typedef struct Clumpnugget
{
    Vector2 position;
    Vector2 velocity;
    Vector2 attach_position;
    bool attached;
} Clumpnugget;

typedef struct Food
{
    Vector2 position;
    Vector2 velocity;
    bool consumed;
} Food;

Invader g_invader;
Clumpnugget g_clumpnuggets[100];
Food g_food[1000];
Camera2D g_camera;

enum GameState
{
    GameWin,
    GameLose,
    InGame
} g_game_state;

const float g_invader_start_radius = 30.0f;
const float g_clump_nugget_radius = 10.0f;
const float g_clump_nugget_speed = 10.0f;
const float g_food_radius = 10.0f;
const int g_screen_width = 1000;
const int g_screen_height = 1000;
const float g_embed_distance = -5.0f;
const float g_friction = 0.98f;
const float g_hunger_timer_reset = 15.0f;
int g_food_consumed = 0;
float g_target_radius = 0.0f;
float g_hunger_timer = 0.0f;
int g_difficulty = 1;

#ifdef NDEBUG
const bool g_debug_mode = false;
#else
const bool g_debug_mode = true;
#endif

void InitializeGame();
void RunGame();
void CloseGame();
void InitializeGameSpecifics();
void Update(const float frame_time);
void UpdateCamera2D(const float frame_time);
void UpdateInvader(const float frame_time);
void UpdateClumpnuggets(const float frame_time);
void UpdateFood(const float frame_time);
void UpdateGameState(const float frame_time);
void Render(const float frame_time);
void RenderWorld(const float frame_time);
void RenderInvader();
void RenderClumpnuggets();
void RenderFood();
void RenderUI();
void Log(const char* format, const float elapsed_seconds, ...);

int main(int n, char** args) 
{
    InitializeGame();
    RunGame();
    CloseGame();
}

void InitializeGame()
{
    SetTraceLogLevel(g_debug_mode ? LOG_ALL : LOG_NONE);
    InitWindow(g_screen_width, g_screen_height, "Clumpnuggets");
    InitAudioDevice();
    HideCursor();
    InitializeGameSpecifics();
}

void RunGame()
{
    while (!WindowShouldClose()) 
    {
        const float frame_time = GetFrameTime();
        Update(frame_time);
        Render(frame_time);
    }
}

void CloseGame()
{
    CloseAudioDevice();
    CloseWindow();
}

void InitializeGameSpecifics()
{
    g_camera.offset = (Vector2){g_screen_width / 2.0f, g_screen_height / 2.0f};
    g_camera.target = (Vector2){0.0f, 0.0f};
    g_camera.rotation = 0.0f;
    g_camera.zoom = 1.0f;

    g_invader.position = Vector2Zero();
    g_invader.radius = g_invader_start_radius;

    memset(g_clumpnuggets, 0, sizeof(g_clumpnuggets));
    for(int i = 0; i < _countof(g_clumpnuggets); ++i)
    {
        g_clumpnuggets[i].position = (Vector2){(float)GetRandomValue(-5000, 5000), (float)GetRandomValue(-5000, 5000)};
    }

    memset(g_food, 0, sizeof(g_food));
    for(int i = 0; i < _countof(g_food); ++i)
    {
        g_food[i].position = (Vector2){(float)GetRandomValue(-5000, 5000), (float)GetRandomValue(-5000, 5000)};
        g_food[i].consumed = false;
    }
    
    ++g_difficulty;
    g_target_radius = g_invader_start_radius * (float)g_difficulty;
    g_game_state = InGame;
    g_hunger_timer = g_hunger_timer_reset;
}

void Update(const float frame_time)
{
    UpdateGameState(frame_time);

    if(g_game_state == InGame)
    {
        UpdateCamera2D(frame_time);
        UpdateInvader(frame_time);
        UpdateClumpnuggets(frame_time);
        UpdateFood(frame_time);
    }
}

void UpdateCamera2D(const float frame_time)
{
    g_camera.target = Vector2Add(g_camera.target, Vector2Scale(g_invader.velocity, frame_time));
}

void UpdateInvader(const float frame_time)
{
    g_invader.state = IsKeyDown(KEY_SPACE) ? Moving : Idle;

    const float acceleration = 300.0f;
    const float thrusters_on = g_invader.state == Moving ? 1.0f : 0.0f;
    g_invader.velocity = Vector2Add(g_invader.velocity, Vector2Scale(g_invader.look_at_direction, thrusters_on * acceleration * frame_time));
    g_invader.velocity = Vector2Scale(g_invader.velocity, g_friction);

    const Vector2 screen_center = g_camera.offset;
    g_invader.look_at_direction = Vector2Normalize(Vector2Subtract(GetMousePosition(), screen_center));

    g_invader.rotation = g_invader.look_at_direction.x > 0
        ? RAD2DEG * acosf(-g_invader.look_at_direction.y)
        : 180.0f + RAD2DEG * acosf(g_invader.look_at_direction.y);

    g_invader.position = g_camera.target;

    // consume food and grow invader
    g_invader.radius = g_invader_start_radius;
    g_invader.radius += g_food_consumed;
}

void UpdateClumpnuggets(const float frame_time)
{
    for(int i = 0; i < _countof(g_clumpnuggets); ++i)
    {
        if(g_clumpnuggets[i].attached)
        {
            g_clumpnuggets[i].attach_position = Vector2Scale(Vector2Normalize(g_clumpnuggets[i].attach_position), g_invader.radius - g_embed_distance);
            g_clumpnuggets[i].position = Vector2Add(g_clumpnuggets[i].attach_position, g_invader.position);
            continue;
        }

        const Vector2 invader_direction = Vector2Normalize(Vector2Subtract(g_invader.position, g_clumpnuggets[i].position));
        const Vector2 acceleration = Vector2Scale(invader_direction, g_clump_nugget_speed);
        g_clumpnuggets[i].velocity = Vector2Add(g_clumpnuggets[i].velocity, Vector2Scale(acceleration, frame_time));
        g_clumpnuggets[i].velocity = Vector2Clamp(g_clumpnuggets[i].velocity, (Vector2){-g_clump_nugget_speed, -g_clump_nugget_speed}, (Vector2){g_clump_nugget_speed, g_clump_nugget_speed});
        g_clumpnuggets[i].position = Vector2Add(g_clumpnuggets[i].position, Vector2Scale(g_clumpnuggets[i].velocity, frame_time));

        g_clumpnuggets[i].attached = CheckCollisionCircles(g_invader.position, g_invader.radius - g_embed_distance, g_clumpnuggets[i].position, g_clump_nugget_radius);
        
        if(g_clumpnuggets[i].attached)
        {
            g_clumpnuggets[i].attach_position = Vector2Subtract(g_clumpnuggets[i].position, g_invader.position);
        }

        // clumpnuggets cant attach if there's already one attached at this spot
        for(int j = 0; j < _countof(g_clumpnuggets); ++j)
        {
            if(!g_clumpnuggets[j].attached)
            {
                continue;
            }

            if(CheckCollisionCircles(g_clumpnuggets[i].position, g_clump_nugget_radius, g_clumpnuggets[j].position, g_clump_nugget_radius))
            {
                // try moving perpendicular
                const float x = g_clumpnuggets[i].velocity.x;
                const float y = g_clumpnuggets[i].velocity.y;
                g_clumpnuggets[i].velocity = (Vector2){-y, x};
            }
        }
    }
}

void UpdateFood(const float frame_time)
{
    for(int i = 0; i < _countof(g_food); ++i)
    {
        if(g_food[i].consumed)
        {
            continue;
        }

        g_food[i].velocity = Vector2Scale(g_food[i].velocity, g_friction);
        g_food[i].position = Vector2Add(g_food[i].position, Vector2Scale(g_food[i].velocity, frame_time));
        g_food[i].consumed = CheckCollisionCircles(g_invader.position, g_invader.radius - g_embed_distance, g_food[i].position, g_food_radius);
        
        int last_consumed = g_food_consumed;
        g_food_consumed = g_food[i].consumed ? g_food_consumed + 1 : g_food_consumed;
        g_hunger_timer = g_food_consumed > last_consumed ? g_hunger_timer_reset : g_hunger_timer;

        // food can't be consumed if a clumpnugget is attached and in the way
        for(int j = 0; j < _countof(g_clumpnuggets); ++j)
        {
            if(!g_clumpnuggets[j].attached)
            {
                continue;
            }

            if(CheckCollisionCircles(g_clumpnuggets[j].position, g_clump_nugget_radius, g_food[i].position, g_food_radius))
            {
                const Vector2 direction = Vector2Normalize(Vector2Subtract(g_food[i].position, g_clumpnuggets[j].position));
                const float amount = Vector2DotProduct(direction, Vector2Normalize(g_invader.velocity));
                g_food[i].velocity = Vector2Scale(direction, Vector2Distance(g_invader.velocity, Vector2Zero()) * amount);
            }
        }
    }
}

void UpdateGameState(const float frame_time)
{
    g_hunger_timer -= frame_time;
    g_game_state = g_target_radius <= g_invader.radius ? GameWin : g_game_state;
    g_game_state = g_hunger_timer <= 0.0f ? GameLose : g_game_state;
}

void Render(const float frame_time)
{
    BeginDrawing();
    ClearBackground(DARKPURPLE); 
    BeginScissorMode(0, 0, g_screen_width, g_screen_height);
    BeginMode2D(g_camera);
    RenderWorld(frame_time);
    EndMode2D();
    RenderUI();
    EndScissorMode();
    EndDrawing();
}

void RenderWorld(const float frame_time)
{
    RenderInvader();
    RenderClumpnuggets();
    RenderFood();
}

void RenderInvader()
{
    const float brightness = Lerp(0.0f, -1.0f, 1.0f - g_hunger_timer / g_hunger_timer_reset);
    DrawCircleV(g_invader.position, g_invader.radius, ColorBrightness(RED, brightness));
    DrawCircleLinesV(g_invader.position, g_target_radius, ORANGE);
}

void RenderClumpnuggets()
{
    for(int i = 0; i < _countof(g_clumpnuggets); ++i)
    {
        DrawCircleV(g_clumpnuggets[i].position, g_clump_nugget_radius, YELLOW);
    }
}

void RenderFood()
{
    for(int i = 0; i < _countof(g_food); ++i)
    {
        if(g_food[i].consumed)
        {
            continue;
        }

        DrawCircleV(g_food[i].position, g_food_radius, RED);
    }
}

void RenderUI()
{
    DrawFPS(10, 10);
    DrawRectangleV(GetMousePosition(), (Vector2){5, 5}, WHITE);
}

void Log(const char* format, const float elapsed_seconds, ...)
{
    if(!g_debug_mode)
    {
        return;
    }

    char buffer[1024];
    strcpy_s(buffer, 1024, "%0.3f   ");

    va_list args;
    va_start(args, elapsed_seconds);
    vsprintf_s(buffer + strlen(buffer), 1024 - strlen(buffer), format, args);
    va_end(args);

    TraceLog(LOG_DEBUG, buffer, elapsed_seconds);
}
