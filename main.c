#include <math.h>
#include <stddef.h>

#include <raylib.h>

#define ARRAY_LEN(xs) (sizeof(xs)/sizeof((xs)[0]))

#define SCREEN_W    960
#define SCREEN_H    540
#define MAP_W       16
#define MAP_H       16
#define MAX_DEPTH   20.0f
#define MOVE_SPEED  3.0f
#define TURN_SPEED  2.0f
#define FOV_PLANE   0.66f
#define MINIMAP_CELL 8
#define MINIMAP_RAYS 41
#define MINIMAP_MARGIN 12

#define LIST_OF_TEXTURES \
    TEXTURE(TEX_BRICK, "assets/brick.png", 190,  33, 55) \
    TEXTURE(TEX_STONE, "assets/stone.png", 130, 130, 130) \
    TEXTURE(TEX_WOOD,  "assets/wood.png",  127, 106, 79)

typedef enum {
#define TEXTURE(id, path, r, g, b) id,
    LIST_OF_TEXTURES
#undef TEXTURE
    TEX_COUNT,
} Texture_Id;

static const char *texture_paths[TEX_COUNT] = {
#define TEXTURE(id, path, r, g, b) path,
    LIST_OF_TEXTURES
#undef TEXTURE
};

static const Color texture_fallbacks[TEX_COUNT] = {
#define TEXTURE(id, path, r, g, b) {r, g, b, 255},
    LIST_OF_TEXTURES
#undef TEXTURE
};

static const char *MAP[MAP_H] = {
    "1111111111111111",
    "1..............1",
    "1..22......22..1",
    "1..2........2..1",
    "1..............1",
    "1.......33.....1",
    "1.......33.....1",
    "1..............1",
    "1...2......2...1",
    "1...2......2...1",
    "1..............1",
    "1.....3333.....1",
    "1..2.........2.1",
    "1..2.........2.1",
    "1..............1",
    "1111111111111111",
};

typedef struct {
    float x, y, angle;
} Player;

typedef struct {
    Player player;
    Texture2D textures[TEX_COUNT];
} State;

typedef struct {
    float distance;
    int side;
    int texture_id;
} Ray_Hit;

static int map_cell(int x, int y)
{
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return 1;
    if (MAP[y][x] == '.') return 0;
   
    int id = MAP[y][x] - '0';

    return (id >= 1 && id <= TEX_COUNT) ? id : 1;
}

static void player_move(Player *p, float dx, float dy)
{
    const float margin = 0.2f;
    
    float nx = p->x + dx;
    float ny = p->y + dy;

    if (map_cell((int)(nx + (dx > 0 ? margin : -margin)), (int)p->y) == 0) p->x = nx;
    if (map_cell((int)p->x, (int)(ny + (dy > 0 ? margin : -margin))) == 0) p->y = ny;
}

static void player_update(Player *p, float dt)
{
    float fx = cosf(p->angle), fy = sinf(p->angle);
    float step = MOVE_SPEED*dt;

    if (IsKeyDown(KEY_W)) player_move(p,  fx*step,  fy*step);
    if (IsKeyDown(KEY_S)) player_move(p, -fx*step, -fy*step);
    if (IsKeyDown(KEY_A)) player_move(p,  fy*step, -fx*step);
    if (IsKeyDown(KEY_D)) player_move(p, -fy*step,  fx*step);
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_Q)) p->angle -= TURN_SPEED*dt;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_E)) p->angle += TURN_SPEED*dt;
}

static Ray_Hit cast_ray(const Player *p, float ray_x, float ray_y)
{
    int mx = (int)p->x, my = (int)p->y;
  
    float delta_x = ray_x == 0.0f ? 1e30f : fabsf(1.0f/ray_x);
    float delta_y = ray_y == 0.0f ? 1e30f : fabsf(1.0f / ray_y);
    
    int step_x = ray_x < 0 ? -1 : 1;
    int step_y = ray_y < 0 ? -1 : 1;
    
    float side_x = ray_x < 0 ? (p->x - mx)*delta_x : (mx + 1.0f - p->x)*delta_x;
    float side_y = ray_y < 0 ? (p->y - my) * delta_y : (my + 1.0f - p->y) * delta_y;
    
    int side = 0, id = 0;

    while (id == 0) {
        if (side_x < side_y) { side_x += delta_x; mx += step_x; side = 0; }
        else                 { side_y += delta_y; my += step_y; side = 1; }
        id = map_cell(mx, my);
        if (fminf(side_x, side_y) > MAX_DEPTH) {
            return (Ray_Hit) { .distance = MAX_DEPTH };
        }
    }

    return (Ray_Hit) {
        .distance = side == 0 ? side_x - delta_x : side_y - delta_y,
        .side = side,
        .texture_id = id,
    };
}

static void draw_minimap(const Player *p)
{
    const int map_w = MAP_W*MINIMAP_CELL;
    const int map_h = MAP_H * MINIMAP_CELL;
    
    const int ox = SCREEN_W - map_w - MINIMAP_MARGIN;
    const int oy = MINIMAP_MARGIN;
    
    const Vector2 player = {
        ox + p->x * MINIMAP_CELL,
        oy + p->y * MINIMAP_CELL,
    };
    
    float fx = cosf(p->angle), fy = sinf(p->angle);

    DrawRectangle(ox - 2, oy - 2, map_w + 4, map_h + 4, (Color){10, 10, 15, 220});
    DrawRectangle(ox, oy, map_w, map_h, (Color){20, 20, 28, 210});

    for (int i = 0; i < MINIMAP_RAYS; ++i) {
        float camera_x = 2.0f*i/(MINIMAP_RAYS - 1) - 1.0f;
        float ray_x = fx - fy*FOV_PLANE*camera_x;
        float ray_y = fy + fx*FOV_PLANE*camera_x;
        Ray_Hit ray_hit = cast_ray(p, ray_x, ray_y);

        Vector2 hit = {
            player.x + ray_x*ray_hit.distance*MINIMAP_CELL,
            player.y + ray_y*ray_hit.distance*MINIMAP_CELL,
        };
        
        DrawLineV(player, hit, (Color){255, 220, 80, 50});
    }

    for (int y = 0; y < MAP_H; ++y) {
        for (int x = 0; x < MAP_W; ++x) {
            int id = map_cell(x, y);
            if (id == 0) continue;
            Color color = texture_fallbacks[id - 1];
            color.a = 230;
            DrawRectangle(ox + x*MINIMAP_CELL, oy + y*MINIMAP_CELL, MINIMAP_CELL, MINIMAP_CELL, color);
        }
    }

    DrawRectangleLines(ox, oy, map_w, map_h, (Color){230, 230, 240, 180});
    DrawCircleV(player, 3.0f, (Color){80, 255, 120, 255});
    DrawLineEx(player, (Vector2){
        player.x + fx*MINIMAP_CELL,
        player.y + fy*MINIMAP_CELL,
    }, 2.0f, WHITE);
}

static void draw_column(const State *s, int column)
{
    const Player *p = &s->player;

    float camera_x = 2.0f * column / SCREEN_W - 1.0f;
    
    float ray_x = cosf(p->angle) - sinf(p->angle)*FOV_PLANE*camera_x;
    float ray_y = sinf(p->angle) + cosf(p->angle)*FOV_PLANE*camera_x;

    Ray_Hit hit = cast_ray(p, ray_x, ray_y);

    if (hit.texture_id == 0) return;

    float dist = hit.distance;
    if (dist < 0.01f) dist = 0.01f;
    
    int line_h = (int)(SCREEN_H/dist);
    int top = SCREEN_H/2 - line_h/2;

    Color tint = hit.side == 1 ? (Color){180, 180, 180, 255} : WHITE;
    Texture2D t = s->textures[hit.texture_id - 1];

    if (t.id == 0) {
        Color c = texture_fallbacks[hit.texture_id - 1];
        DrawRectangle(column, top, 1, line_h, (Color){c.r*tint.r/255, c.g*tint.g/255, c.b*tint.b/255, 255});
        return;
    }

    float wall_x = hit.side == 0 ? p->y + dist*ray_y : p->x + dist*ray_x;
    wall_x -= floorf(wall_x);
    int tx = (int)(wall_x*t.width);
    
    if ((hit.side == 0 && ray_x > 0) || (hit.side == 1 && ray_y < 0)) tx = t.width - tx - 1;
    if (tx < 0) tx = 0;
    if (tx >= t.width) tx = t.width - 1;

    Rectangle src = { (float)tx, 0, 1, (float)t.height };
    Rectangle dst = { (float)column, (float)top, 1, (float)line_h };
    DrawTexturePro(t, src, dst, (Vector2){0}, 0, tint);
}

int main(void)
{
    State s = { 
        .player = { 
            .x = 2.5f,
            .y = 2.5f, 
            .angle = 0.5f 
        } 
    };

    InitWindow(SCREEN_W, SCREEN_H, "raycaster");
    SetTargetFPS(60);

    for (size_t i = 0; i < ARRAY_LEN(s.textures); ++i) {
        if (!FileExists(texture_paths[i])) continue;
        s.textures[i] = LoadTexture(texture_paths[i]);
        SetTextureFilter(s.textures[i], TEXTURE_FILTER_POINT);
    }

    while (!WindowShouldClose()) {
        player_update(&s.player, GetFrameTime());

        BeginDrawing();
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H/2, (Color){40, 40, 60, 255});
        DrawRectangle(0, SCREEN_H/2, SCREEN_W, SCREEN_H/2, (Color){60, 60, 60, 255});
        for (int x = 0; x < SCREEN_W; ++x) draw_column(&s, x);
        draw_minimap(&s.player);
        DrawFPS(10, 10);
        EndDrawing();
    }

    for (size_t i = 0; i < ARRAY_LEN(s.textures); ++i) if (s.textures[i].id != 0) UnloadTexture(s.textures[i]);
    CloseWindow();
    return 0;
}
