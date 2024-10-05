#include "raylib.h"
#include "raymath.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Camera2D g_camera;

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
    bool attached;
} Clumpnugget;

typedef struct Food
{
    Vector2 position;
    bool consumed;
} Food;

Invader g_invader;
Clumpnugget g_clumpnuggets[100];
Food g_food[1000];

const float g_clump_nugget_radius = 10.0f;
const float g_clump_nugget_speed = 10.0f;
const float g_food_radius = 10.0f;
const int g_screen_width = 1000;
const int g_screen_height = 1000;

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
    g_invader.radius = 30.0f;

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
}

void Update(const float frame_time)
{
    UpdateCamera2D(frame_time);
    UpdateInvader(frame_time);
    UpdateClumpnuggets(frame_time);
}

void UpdateCamera2D(const float frame_time)
{
    g_camera.target = Vector2Add(g_camera.target, Vector2Scale(g_invader.velocity, frame_time));
}

void UpdateInvader(const float frame_time)
{
    g_invader.state = IsKeyDown(KEY_SPACE) ? Moving : Idle;

    const float friction = 0.98f;
    const float acceleration = 300.0f;
    const float thrusters_on = g_invader.state == Moving ? 1.0f : 0.0f;
    g_invader.velocity = Vector2Add(g_invader.velocity, Vector2Scale(g_invader.look_at_direction, thrusters_on * acceleration * frame_time));
    g_invader.velocity = Vector2Scale(g_invader.velocity, friction);

    const Vector2 screen_center = g_camera.offset;
    g_invader.look_at_direction = Vector2Normalize(Vector2Subtract(GetMousePosition(), screen_center));

    g_invader.rotation = g_invader.look_at_direction.x > 0
        ? RAD2DEG * acosf(-g_invader.look_at_direction.y)
        : 180.0f + RAD2DEG * acosf(g_invader.look_at_direction.y);

    g_invader.position = g_camera.target;

    // consume food and grow invader
    for(int i = 0; i < _countof(g_food); ++i)
    {
        if(g_food[i].consumed)
        {
            continue;
        }

        g_food[i].consumed = CheckCollisionCircles(g_invader.position, g_invader.radius, g_food[i].position, g_food_radius);
        g_invader.radius += g_food[i].consumed ? 1.0f : 0.0f;
    }
}

void UpdateClumpnuggets(const float frame_time)
{
    for(int i = 0; i < _countof(g_clumpnuggets); ++i)
    {
        if(g_clumpnuggets[i].attached)
        {
            continue;
        }

        const Vector2 invader_direction = Vector2Normalize(Vector2Subtract(g_invader.position, g_clumpnuggets[i].position));
        const Vector2 acceleration = Vector2Scale(invader_direction, g_clump_nugget_speed);
        g_clumpnuggets[i].velocity = Vector2Add(g_clumpnuggets[i].velocity, Vector2Scale(acceleration, frame_time));
        g_clumpnuggets[i].velocity = Vector2Clamp(g_clumpnuggets[i].velocity, (Vector2){-g_clump_nugget_speed, -g_clump_nugget_speed}, (Vector2){g_clump_nugget_speed, g_clump_nugget_speed});
        g_clumpnuggets[i].position = Vector2Add(g_clumpnuggets[i].position, Vector2Scale(g_clumpnuggets[i].velocity, frame_time));

        const float embed_distance = 2.0f;
        if(CheckCollisionCircles(g_invader.position, g_invader.radius - embed_distance, g_clumpnuggets[i].position, g_clump_nugget_radius))
        {
            g_clumpnuggets[i].attached = true;
        }
    }
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
    DrawCircleV(g_invader.position, g_invader.radius, RED);
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
