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
    float dash_cooldown_timer;
    float dash_tracker[2];
    int dash_tracker_index;

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
    bool super_fast;

    enum MovmentStyle
    {
        Chase,
        Spiral,
    } move_style;
} Clumpnugget;

typedef struct Food
{
    Vector2 position;
    Vector2 velocity;
    bool consumed;
} Food;

Invader g_invader;
Clumpnugget g_clumpnuggets[200];
Food g_food[400];
Camera2D g_camera;
Font g_font;
Sound g_pickup_sound, g_low_hp_sound;
Music g_ambient_music;
Texture2D g_spritesheet;
Texture2D g_background[3];
Color g_background_color;

enum GameState
{
    Menu,
    GameWin,
    GameLose,
    InGame,
    GameInit,
    HowToPlay,
    Quit
} g_game_state;

typedef Rectangle Sprite;

enum SpriteType
{
    Clumpnugget_1,
    Clumpnugget_2,
    Clumpnugget_3,
};

enum BackgroundType
{    
    Background_1,
    Background_2,
    Background_3
};

Sprite g_sprites[3] = {
    {4.0f, 1.0f, 64.0f, 65.0f},
    {66.0f, 1.0f, 64.0f, 65.0f},
    {131.0f, 1.0f, 64.0f, 65.0f}
};

const Rectangle g_world_bounds = {-2000.0f, -2000.0f, 4000.0f, 4000.0f};
const float g_invader_start_radius = 30.0f;
const float g_invader_acceleration = 500.0f;
const float g_invader_dash_cooldown_timer_reset = 5.0f;
const float g_clump_nugget_radius = 20.0f;
const float g_clump_nugget_max_speed = 50.0f;
const float g_food_radius = 10.0f;
const int g_screen_width = 1000;
const int g_screen_height = 1000;
const float g_embed_distance = -5.0f;
const float g_friction = 0.98f;
const float g_hunger_timer_reset = 15.0f;
const float g_next_round_timer_reset = 3.0f;
const float g_dash_eligibility_period = 0.2f;
int g_attached_clumpnuggets = 0;
int g_food_consumed = 0;
float g_target_radius = 0.0f;
float g_hunger_timer = 0.0f;
float g_hunger_sound_timer = 0.0f;
float g_next_round_timer = 7.0f;
float g_round_start_timer = 5.0f;
int g_difficulty = 1;
int g_game_round = 0;
int g_menu_selection = 0;
const char* g_menu_items[] = {"Start", "How to play?", "Quit"};

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
void UpdateInGameState(const float frame_time);
void UpdateMenu();
void UpdateHowToPlay();
void UpdateGameWin(const float frame_time);
void Render(const float frame_time);
void RenderWorld(const float frame_time);
void RenderBackground();
void RenderInvader();
void RenderClumpnuggets();
void RenderFood();
void RenderUI();
void RenderInGameUI();
void RenderGameWinUI();
void RenderGameLoseUI();
void RenderMenu();
void RenderMenuBackdrop();
void RenderHowToPlay();
void FreeResources();
bool IsRunningGame();
void Log(const char* format, const float elapsed_seconds, ...);
Color LerpColor(const Color a, const Color b, const float t);

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
    SetExitKey(0);
    HideCursor();

    if(!g_debug_mode)
    {
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }

    g_font = LoadFont("assets/fonts/COOPBL.ttf");
    g_pickup_sound = LoadSound("assets/sfx/pickup.wav");
    g_low_hp_sound = LoadSound("assets/sfx/low_hp.wav");
    g_ambient_music = LoadMusicStream("assets/sfx/ambient_music.mp3");
    g_spritesheet = LoadTexture("assets/sprites/spritesheet.png");
    g_background[Background_1] = LoadTexture("assets/sprites/background_1.png");
    g_background[Background_2] = LoadTexture("assets/sprites/background_2.png");
    g_background[Background_3] = LoadTexture("assets/sprites/background_3.png");
    PlayMusicStream(g_ambient_music);
    SetMusicVolume(g_ambient_music, 0.5f);

    g_game_state = Menu;
}

void RunGame()
{
    while (IsRunningGame()) 
    {
        const float frame_time = GetFrameTime();
        Update(frame_time);
        Render(frame_time);
    }
}

void CloseGame()
{
    FreeResources();
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

    const int x = (int)(g_world_bounds.width * 0.5f);
    const int y = (int)(g_world_bounds.height * 0.5f);
    memset(g_clumpnuggets, 0, sizeof(g_clumpnuggets));
    for(int i = 0; i < _countof(g_clumpnuggets); ++i)
    {
        g_clumpnuggets[i].position = (Vector2){(float)GetRandomValue(-x, x), (float)GetRandomValue(-y, y)};
        g_clumpnuggets[i].super_fast = GetRandomValue(0, 1000) < 300;
        g_clumpnuggets[i].move_style = GetRandomValue(0, 1000) < 600 ? Chase : Spiral;
    }

    memset(g_food, 0, sizeof(g_food));
    for(int i = 0; i < _countof(g_food); ++i)
    {
        g_food[i].position = (Vector2){(float)GetRandomValue(-x, x), (float)GetRandomValue(-y, y)};
        g_food[i].consumed = false;
    }
    
    ++g_difficulty;
    ++g_game_round;
    g_target_radius = g_invader_start_radius * (float)g_difficulty;
    g_game_state = InGame;
    g_hunger_timer = g_hunger_timer_reset;
    g_hunger_sound_timer = 0.25f;
    g_next_round_timer = g_next_round_timer_reset;
    g_round_start_timer = 0.0f;
    g_food_consumed = 0;
    g_background_color = ColorFromHSV(60.0f, 0.6f, 1.0f);//ColorFromHSV(fmodf(g_game_round * 60.0f, 360.0f), 0.6f, 1.0f);
    g_attached_clumpnuggets = 0;
}

void Update(const float frame_time)
{
    UpdateMusicStream(g_ambient_music);
    switch(g_game_state)
    {
        case GameInit:
        {
            InitializeGameSpecifics();
        }break;
        case InGame:
        {
            UpdateInGameState(frame_time);
            UpdateCamera2D(frame_time);
            UpdateInvader(frame_time);
            UpdateClumpnuggets(frame_time);
            UpdateFood(frame_time);
        }break;
        case GameWin:
        {
            UpdateGameWin(frame_time);
        }break;
        case GameLose:
        {
            g_difficulty = 1;
            g_game_round = 0;
            g_game_state = IsKeyPressed(KEY_ESCAPE) ? Menu : g_game_state;
        }break;
        case Menu:
        {
            UpdateMenu();
        }break;
        case HowToPlay:
        {
            UpdateHowToPlay();
        }break;
    }
}

void UpdateCamera2D(const float frame_time)
{
    g_camera.target = Vector2Add(g_camera.target, Vector2Scale(g_invader.velocity, frame_time));
}

void UpdateInvader(const float frame_time)
{
    const enum InvaderState last_state = g_invader.state;
    g_invader.state = IsKeyDown(KEY_SPACE) ? Moving : Idle;
    const bool state_changed = last_state != g_invader.state;

    float speed_boost = 1.0f;
    if(state_changed && g_invader.state == Moving)
    {
        g_invader.dash_tracker[g_invader.dash_tracker_index % 2] = (float)GetTime();
        g_invader.dash_tracker_index++;

        const float time_since_last_state_change = fabsf(g_invader.dash_tracker[1] - g_invader.dash_tracker[0]);
        const bool is_dashing = time_since_last_state_change < g_dash_eligibility_period;
        const bool can_dash = is_dashing && g_invader.dash_cooldown_timer <= 0.0f;
        speed_boost = can_dash ? 5.0f : 1.0f;
        g_invader.dash_cooldown_timer = can_dash ? g_invader_dash_cooldown_timer_reset + g_attached_clumpnuggets * 0.3f : g_invader.dash_cooldown_timer;
    }

    const float thrusters_on = g_invader.state == Moving ? 1.0f : 0.0f;
    const float acceleration = max(100.0f, g_invader_acceleration - g_game_round * 50.0f);
    g_invader.velocity = Vector2Add(g_invader.velocity, Vector2Scale(g_invader.look_at_direction, thrusters_on * acceleration * frame_time));
    g_invader.velocity = Vector2Add(g_invader.velocity, Vector2Scale(g_invader.velocity, -g_friction * frame_time));
    g_invader.velocity = Vector2Scale(g_invader.velocity, speed_boost);

    const Vector2 screen_center = g_camera.offset;
    g_invader.look_at_direction = Vector2Normalize(Vector2Subtract(GetMousePosition(), screen_center));

    g_invader.rotation = g_invader.look_at_direction.x > 0
        ? RAD2DEG * acosf(-g_invader.look_at_direction.y)
        : 180.0f + RAD2DEG * acosf(g_invader.look_at_direction.y);

    g_invader.position = g_camera.target;

    // consume food and grow invader
    g_invader.radius = g_invader_start_radius;
    g_invader.radius += g_food_consumed;

    g_invader.dash_cooldown_timer -= frame_time;
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

        const Vector2 invader_direction = Vector2Subtract(g_invader.position, g_clumpnuggets[i].position);

        // only clumpnuggets within sight will chase
        if(Vector2Length(invader_direction) > 600.0f)
        {
            continue;
        }

        const float speed = g_clumpnuggets[i].super_fast ? g_clump_nugget_max_speed * 2.0f : g_clump_nugget_max_speed;
        const Vector2 acceleration = Vector2Scale(Vector2Normalize(invader_direction), speed);
        g_clumpnuggets[i].velocity = Vector2Add(g_clumpnuggets[i].velocity, Vector2Scale(acceleration, frame_time));
        g_clumpnuggets[i].velocity = Vector2Clamp(g_clumpnuggets[i].velocity, (Vector2){-speed, -speed}, (Vector2){speed, speed});
        g_clumpnuggets[i].position = Vector2Add(g_clumpnuggets[i].position, Vector2Scale(g_clumpnuggets[i].velocity, frame_time));

        if(g_clumpnuggets[i].move_style == Spiral)
        {
            const float t = (float)GetTime();
            const float frequency = 2.0f;
            const Vector2 spiral_velocity = (Vector2) {
                0.0f,
                sinf(t * frequency) * 300.0f
            };

            g_clumpnuggets[i].position = Vector2Add(g_clumpnuggets[i].position, Vector2Scale(spiral_velocity, frame_time));
        }

        g_clumpnuggets[i].attached = CheckCollisionCircles(g_invader.position, g_invader.radius - g_embed_distance, g_clumpnuggets[i].position, g_clump_nugget_radius);
        
        if(g_clumpnuggets[i].attached)
        {
            g_attached_clumpnuggets++;
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
                g_food[i].position = Vector2Add(g_food[i].position, Vector2Scale(direction, amount));
            }
        }

        g_food[i].consumed = CheckCollisionCircles(g_invader.position, g_invader.radius - g_embed_distance, g_food[i].position, g_food_radius);
        
        const int last_consumed = g_food_consumed;
        g_food_consumed = g_food[i].consumed ? g_food_consumed + 1 : g_food_consumed;
        const bool is_consumed = g_food_consumed > last_consumed;
        g_hunger_timer = is_consumed ? min(g_hunger_timer_reset, g_hunger_timer + 5) : g_hunger_timer;

        if(is_consumed)
        {
            SetSoundVolume(g_pickup_sound, Lerp(0.01f, 0.1f, (float)GetRandomValue(0, 100) / 100.0f));
            SetSoundPitch(g_pickup_sound, Lerp(0.5f, 1.0f, (float)GetRandomValue(0, 100) / 100.0f));
            PlaySound(g_pickup_sound);
        }
    }
}

void UpdateMenu()
{
    const int items_count = _countof(g_menu_items);
    g_menu_selection = g_menu_selection + (int)(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S));
    g_menu_selection = items_count + g_menu_selection - (int)(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W));
    g_menu_selection %= items_count;

    if(IsKeyPressed(KEY_ENTER))
    {
        switch(g_menu_selection)
        {
            case 0:
            {
                g_game_round = 0;
                g_difficulty = 1;
                g_game_state = GameInit;
            }break;
            case 1:
            {
                g_game_state = HowToPlay;
            }break;
            case 2:
            {
                g_game_state = Quit;
            }break;
        }
    }
}

void UpdateInGameState(const float frame_time)
{
    g_hunger_timer -= frame_time;
    g_round_start_timer += frame_time;
    g_game_state = g_target_radius <= g_invader.radius ? GameWin : g_game_state;
    g_game_state = g_hunger_timer <= 0.0f ? GameLose : g_game_state;
    g_game_state = IsKeyPressed(KEY_ESCAPE) ? Menu : g_game_state;

    if(g_hunger_timer <= 5.0f)
    {
        g_hunger_sound_timer -= frame_time;
        if(g_hunger_sound_timer <= 0.0f)
        {
            static int beat;
            ++beat;
            g_hunger_sound_timer = 0.25f;
            PlaySound(g_low_hp_sound);
        }
    }
}

void UpdateGameWin(const float frame_time)
{
    g_next_round_timer -= frame_time;
    g_game_state = g_next_round_timer <= 0.0f ? GameInit : g_game_state;
}

void UpdateHowToPlay()
{
    g_game_state = IsKeyPressed(KEY_ESCAPE) ? Menu : g_game_state;
}

void Render(const float frame_time)
{
    BeginDrawing();
    ClearBackground(ColorFromHSV(60.0f, 0.6f, 0.7f));
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
    RenderBackground();
    RenderClumpnuggets();
    RenderInvader();
    RenderFood();
}

void RenderBackground()
{
    const float period = 0.3f;
    const float frequency = (2.0f * PI) / period;
    const enum BackgroundType type = (enum BackgroundType)roundf(sinf((float)GetTime() * frequency) + 1.0f);
    const float rotation = 0.0f;
    const Vector2 origin = Vector2Zero();
    DrawRectangle((int)g_world_bounds.x, (int)g_world_bounds.y, (int)g_world_bounds.width, (int)g_world_bounds.height, g_background_color);
    DrawTexturePro(g_background[type], g_world_bounds, g_world_bounds, origin, 0.0f, RED);
}

void RenderInvader()
{
    const float brightness = Lerp(0.0f, -1.0f, 1.0f - g_hunger_timer / g_hunger_timer_reset);
    const float target_radius_completed = g_invader.radius / g_target_radius;
    const Color color = ColorBrightness(LerpColor(RED, GREEN, target_radius_completed), brightness);
    DrawCircleV(g_invader.position, g_invader.radius, color);
    DrawCircleLinesV(g_invader.position, g_target_radius, ORANGE);
    const Vector2 head_origin = Vector2Add(g_invader.position, Vector2Scale(g_invader.look_at_direction, g_invader.radius));
    DrawCircleV(head_origin, 10.0f, color);
}

void RenderClumpnuggets()
{
    const float rotation = 0.0f;
    for(int i = 0; i < _countof(g_clumpnuggets); ++i)
    {
        const float period = 0.3f;
        const float frequency = (2.0f * PI) / period;
        const enum SpriteType type = (enum SpriteType)roundf(sinf((float)GetTime() * frequency) + 1.0f);
        const float x = g_clumpnuggets[i].position.x;
        const float y = g_clumpnuggets[i].position.y;
        const float width = g_sprites[type].width;
        const float height = g_sprites[type].height;
        const Rectangle dest = (Rectangle){x, y, width, height};
        const Vector2 origin = (Vector2){width * 0.5f, height * 0.5f};
        DrawTexturePro(g_spritesheet, g_sprites[type], dest, origin, rotation, GRAY);
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

        DrawRectangleV(g_food[i].position, (Vector2){g_food_radius, g_food_radius}, RED);
    }
}

void RenderUI()
{
    switch(g_game_state)
    {
        case Menu:
        {
            RenderBackground();
            RenderMenuBackdrop();
            RenderMenu();
        }break;
        case HowToPlay:
        {
            RenderBackground();
            RenderMenuBackdrop();
            RenderHowToPlay();
        }break;
        case InGame:
        {
            RenderInGameUI();
        }break;
        case GameWin:
        {
            RenderGameWinUI();
        }break;
        case GameLose:
        {
            RenderGameLoseUI();
        }break;
    }

    if(g_debug_mode)
    {
        DrawFPS(10, 10);
    }
}

void RenderMenu()
{
    const Vector2 position = {210, 227};
    const Vector2 origin = Vector2Zero();
    const float rotation = 0.0f;
    const float font_size = 92.0f;
    const float spacing = 2.0f;

    DrawTextPro(g_font, "Clumpnuggets", position, origin, rotation, font_size, spacing, WHITE);

    for(int i = 0; i < _countof(g_menu_items); ++i)
    {
        const float y_spacing = 55.0f;
        const Vector2 position = {586, 424 + y_spacing * i};
        const Vector2 origin = Vector2Zero();
        const float rotation = 0.0f;
        const float font_size = 36.0f;
        const float spacing = 2.0f;
        const Color color = i == g_menu_selection ? YELLOW : LIGHTGRAY;
        DrawTextPro(g_font, g_menu_items[i], position, origin, rotation, font_size, spacing, color);
    }
}

void RenderInGameUI()
{
    const float value = 2.0f * expf(-g_round_start_timer);
    const float alpha = value > 1.0f ? 1.0f : value;

    if(alpha > 0.1f)
    {
        DrawTextPro(g_font, TextFormat("Round %d", g_game_round), (Vector2){121.0f, 621.0f}, Vector2Zero(), 0.0f, 92.0f, 2.0f, Fade(BLACK, alpha));
    }
}

void RenderGameWinUI()
{
    const float value = 2.0f * expf(-g_next_round_timer);
    const float alpha = value > 1.0f ? 1.0f : value;
    if(alpha > 0.1f)
    {
        DrawTextPro(g_font, TextFormat("Round %d Completed", g_game_round), (Vector2){121.0f, 621.0f}, Vector2Zero(), 0.0f, 92.0f, 2.0f, Fade(BLACK, alpha));
    }
}

void RenderGameLoseUI()
{
    DrawTextPro(g_font, "Clumpnuggets Win", (Vector2){121.0f, 621.0f}, Vector2Zero(), 0.0f, 92.0f, 2.0f, BLACK);
}

void RenderHowToPlay()
{
    static const char* help_text = "* Use the mouse and space bar to move the invader,\n\n"
        "* Double-tap space bar to dash after a cooldown period,\n\n"
        "* Collect food (red squares) to grow to target size,\n\n"
        "* You advance to next game round once\n\n"
        "   you've reached the target size,\n\n"
        "* Eat quickly or you'll starve,\n\n"
        "* Avoid clumpnuggets (yellow circles),\n\n"
        "* At any point, press ESC to return to the menu\n\n";

    DrawTextPro(g_font, help_text, (Vector2){96.0f, 300.0f}, Vector2Zero(), 0.0f, 36.0f, 2.0f, WHITE);
}

void RenderMenuBackdrop()
{
    DrawRectangleRounded((Rectangle){48.0f, 48.0f, 907.0f, 907.0f}, 1.0f, 1, Fade(BLACK, 0.7f));
}

void FreeResources()
{
    UnloadTexture(g_background[Background_1]);
    UnloadTexture(g_background[Background_2]);
    UnloadTexture(g_background[Background_3]);
    UnloadTexture(g_spritesheet);
    UnloadMusicStream(g_ambient_music);
    UnloadSound(g_low_hp_sound);
    UnloadSound(g_pickup_sound);
    UnloadFont(g_font);
}

bool IsRunningGame()
{
    return !WindowShouldClose() && g_game_state != Quit;
}

Color LerpColor(const Color a, const Color b, const float t)
{
    return (Color)
    {
        (unsigned char)Lerp((float)a.r, (float)b.r, t),
        (unsigned char)Lerp((float)a.g, (float)b.g, t),
        (unsigned char)Lerp((float)a.b, (float)b.b, t),
        (unsigned char)Lerp((float)a.a, (float)b.a, t)
    };
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