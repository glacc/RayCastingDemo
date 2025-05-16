#define _USE_MATH_DEFINES

#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <math.h>

#include <SDL3/SDL.h>

#include "KeyStatesSDL.h"
#include "WindowCreationSDL.h"

#define SCREEN_WIDTH    512
#define SCREEN_HEIGHT   384
#define CHANNELS        3
#define SCALE_FACTOR    2

#define LEVEL_SIZE_X    8
#define LEVEL_SIZE_Y    8

const char window_title[] = "RayCast Demo qwq";

const int screen_width = SCREEN_WIDTH;
const int screen_height = SCREEN_HEIGHT;
const int screen_channels = CHANNELS;
const int scale_factor = SCALE_FACTOR;

const char wall_texture_name[] = "bricks.png";

const float fade_distance = 8.0F;

const float half_fov = 40.0F / 180.0F * (float)M_PI;

const float mouse_sensitivity = 0.0025F;
const float player_turn_speed_per_tick = 2.5F / 180.0F * (float)M_PI;

const float player_vel_walk_per_tick = 0.025F;
const float player_vel_sprint_per_tick = 0.05F;
const float player_accel_per_tick = 0.0025F;
const float player_fraction = 0.8F;

const uint8_t level_data[LEVEL_SIZE_X][LEVEL_SIZE_Y] =
{
    { 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 0, 1, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 1 },
    { 1, 0, 0, 0, 0, 1, 0, 1 },
    { 1, 0, 0, 0, 0, 0, 0, 1 },
    { 1, 0, 0, 0, 1, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1 }
};
const int level_size_x = LEVEL_SIZE_X;
const int level_size_y = LEVEL_SIZE_Y;

const float player_wall_inside_threshold = 0.4F;
const float player_block_offset = 0.0001F;

const float player_start_x = 3;
const float player_start_y = 3;

const float ray_unstable_threshold = 0.0001F;

const float z_cutoff = 0.0001F;

static float player_x, player_y;
static float player_vel_x, player_vel_y;
static float player_angle;

static float *z_list;
static float *texture_x_list;

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;

static SDL_Surface *wall_texture = NULL;

static KeyStatesSDL key_states;

static bool initialized = false;

static bool quit = false;

static const char program_log_tag[] = "[RayCastEngine.c]";

static float RayCast_Vec2Len(float x, float y)
{
    return sqrtf((x * x) + (y * y));
}

static void RayCast_VecClampLength(float *x, float *y, float max_length)
{
    float vec_length = RayCast_Vec2Len(*x, *y);

    if (vec_length <= 0.0001F)
        return;

    if (vec_length >= max_length)
    {
        float vec_norm = 1.0F / vec_length * max_length;

        *x *= vec_norm;
        *y *= vec_norm;
    }
}

static inline float RayCast_WrapAngle(float angle)
{
    float output = fmodf(angle, 2.0F * (float)M_PI);

    if (output < 0.0F)
        output = (2.0F * (float)M_PI) +  output;

    if (output > M_PI)
        output = -(float)M_PI + (output - (float)M_PI);

    return output;
}

bool RayCast_Initialize(void);
void RayCast_Deinitialize(void);

bool RayCast_Initialize(void)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Failed to initialize: %s", SDL_GetError());
        return false;
    }

    KeyStatesSDL_ClearStates(&key_states);

    int pixel_count = screen_width * screen_height;

    z_list = (float *)malloc(sizeof(float) * screen_width);
    if (z_list == NULL)
    {
        SDL_Log("%s Failed to allocate memory for z-list", program_log_tag);
        return false;
    }

    texture_x_list = (float *)malloc(sizeof(float) * screen_width);
    if (texture_x_list == NULL)
    {
        SDL_Log("%s Failed to allocate memory for texture x-list", program_log_tag);
        return false;
    }

    int window_width = screen_width * scale_factor;
    int window_height = screen_height * scale_factor;

    window = WindowCreationSDL_CreateWindowCentered(window_title, window_width, window_height, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == NULL)
    {
        SDL_Log("%s Failed to create window", program_log_tag);
        goto Error;
    }
    SDL_SetWindowRelativeMouseMode(window, true);

    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL)
    {
        SDL_Log("%s Failed to create renderer: %s", program_log_tag, SDL_GetError());
        goto Error;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
    if (texture == NULL)
    {
        SDL_Log("%s Failed to create texture: %s", program_log_tag, SDL_GetError());
        goto Error;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    wall_texture = SDL_LoadBMP("bricks.bmp");
    if (wall_texture == NULL)
        SDL_Log("%s Failed to load texture for wall: %s", program_log_tag, SDL_GetError());

    player_x = player_start_x + 0.5F;
    player_y = player_start_y + 0.5F;
    player_angle = 0;

    quit = false;

    initialized = true;

    return true;

Error:
    RayCast_Deinitialize();

    return false;
}

void RayCast_Deinitialize(void)
{
    if (z_list != NULL)
    {
        free(z_list);
        z_list = NULL;
    }

    if (texture_x_list != NULL)
    {
        free(texture_x_list);
        texture_x_list = NULL;
    }

    if (window != NULL)
    {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    if (renderer != NULL)
    {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }

    if (texture != NULL)
    {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }

    if (wall_texture != NULL)
    {
        SDL_DestroySurface(wall_texture);
        wall_texture = NULL;
    }

    initialized = false;
}

static bool RayCast_CheckIsWall(int x, int y)
{
    if (x < 0 || x >= level_size_x)
        return true;

    if (y < 0 || y >= level_size_y)
        return true;

    return (level_data[y][x] != 0);
}

static void RayCast_PlayerCollisionDetection(void)
{
    const float offset_from_tile_center = 0.5F + player_block_offset;

    float player_x_floor = floorf(player_x);
    float player_y_floor = floorf(player_y);
    int player_x_int = (int)player_x_floor;
    int player_y_int = (int)player_y_floor;

    if (level_data[player_y_int][player_x_int] != 0)
    {
        float tile_center_x = player_x_floor + 0.5F;
        float tile_center_y = player_y_floor + 0.5F;

        float tile_center_to_player_x = player_x - tile_center_x;
        float tile_center_to_player_y = player_y - tile_center_y;

        bool horz_inside = fabsf(tile_center_to_player_x) < player_wall_inside_threshold;
        bool vert_inside = fabsf(tile_center_to_player_y) < player_wall_inside_threshold;
        bool both_inside = horz_inside && vert_inside;

        bool is_wall_l = RayCast_CheckIsWall(player_x_int - 1, player_y_int);
        bool is_wall_r = RayCast_CheckIsWall(player_x_int + 1, player_y_int);
        bool is_wall_u = RayCast_CheckIsWall(player_x_int, player_y_int - 1);
        bool is_wall_d = RayCast_CheckIsWall(player_x_int, player_y_int + 1);

        bool is_corner;
        if (tile_center_to_player_x >= 0.0F && tile_center_to_player_y >= 0)
            is_corner = is_wall_d && is_wall_r;
        else if (tile_center_to_player_x < 0.0F && tile_center_to_player_y >= 0)
            is_corner = is_wall_d && is_wall_l;
        else if (tile_center_to_player_x < 0.0F && tile_center_to_player_y < 0)
            is_corner = is_wall_u && is_wall_l;
        else if (tile_center_to_player_x >= 0.0F && tile_center_to_player_y < 0)
            is_corner = is_wall_u && is_wall_r;
        else
            is_corner = true;

        bool push_both_dir = both_inside || is_corner;

        // Horizontal //

        if (!horz_inside || push_both_dir)
        {
            if (tile_center_to_player_x >= 0.0F && (!is_wall_r || push_both_dir))
            {
                player_x = tile_center_x + offset_from_tile_center;
                player_vel_x = 0.0F;
            }
            else if (tile_center_to_player_x < 0.0F && (!is_wall_l || push_both_dir))
            {
                player_x = tile_center_x - offset_from_tile_center;
                player_vel_x = 0.0F;
            }
        }

        // Vertical //

        if (!vert_inside || push_both_dir)
        {
            if (tile_center_to_player_y >= 0.0F && (!is_wall_d || push_both_dir))
            {
                player_y = tile_center_y + offset_from_tile_center;
                player_vel_y = 0.0F;
            }
            else if (tile_center_to_player_y < 0.0F && (!is_wall_u || push_both_dir))
            {
                player_y = tile_center_y - offset_from_tile_center;
                player_vel_y = 0.0F;
            }
        }
    }
}

static void RayCast_DoRayCastAndRender(void)
{
    float half_screen_width = screen_width / 2.0F;

    float max_norm_offset_x = tanf(half_fov);

    float player_dir_x = cosf(player_angle);
    float player_dir_y = sinf(player_angle);

    float height_z_one = (float)screen_width / (max_norm_offset_x * 2.0F);

    for (int x = 0; x < screen_width; x++)
    {
        float ray_pos_x = player_x;
        float ray_pos_y = player_y;

        float norm_offset_x = (x - half_screen_width) / half_screen_width;

        float angle_offset = atanf(norm_offset_x * max_norm_offset_x);
        float angle_ray = RayCast_WrapAngle(player_angle + angle_offset);

        float ray_dir_x = cosf(angle_ray);
        float ray_dir_y = sinf(angle_ray);

        int center_pos_x = (int)ray_pos_x;
        int center_pos_y = (int)ray_pos_y;

        // U = 1; D = 2; L = 3; R = 4;
        int hit_from_udlr = 1;

        while (true)
        {
            int edge_x_l = center_pos_x;
            int edge_x_r = edge_x_l + 1;
            int edge_y_u = center_pos_y;
            int edge_y_d = edge_y_u + 1;

            if (fabsf(ray_dir_x) < ray_unstable_threshold)
            {
                // Parallels Vertically //
                if (ray_dir_y > 0)
                {
                    // Down //
                    ray_pos_y = (float)edge_y_d;
                    center_pos_y++;

                    hit_from_udlr = 1;
                }
                else
                {
                    // Up //
                    ray_pos_y = (float)edge_y_u;
                    center_pos_y--;

                    hit_from_udlr = 2;
                }
            }
            else if (fabsf(ray_dir_y) < ray_unstable_threshold)
            {
                // Parallels Horizontally //
                if (ray_dir_x > 0)
                {
                    // Right //
                    ray_pos_x = (float)edge_x_r;
                    center_pos_x++;

                    hit_from_udlr = 3;
                }
                else
                {
                    // Left //
                    ray_pos_x = (float)edge_x_l;
                    center_pos_x--;

                    hit_from_udlr = 4;
                }
            }
            else
            {
                // Normal Conditions //

                float new_pos_x, new_pos_y;

                float dist_to_edge_u, dist_to_edge_d, dist_to_edge_l, dist_to_edge_r;

                if (angle_ray >= (float)M_PI * -0.75F && angle_ray < (float)M_PI * -0.25F)
                {
                    // Up //

                    dist_to_edge_u = ray_pos_y - edge_y_u;

                    new_pos_x = ray_pos_x + ((ray_dir_x / (-ray_dir_y)) * dist_to_edge_u);

                    if (new_pos_x >= edge_x_r)
                    {
                        dist_to_edge_r = edge_x_r - ray_pos_x;

                        new_pos_y = ray_pos_y + ((ray_dir_y / ray_dir_x) * dist_to_edge_r);
                        new_pos_x = (float)edge_x_r;

                        center_pos_x++;

                        hit_from_udlr = 3;
                    }
                    else if (new_pos_x < edge_x_l)
                    {
                        dist_to_edge_l = ray_pos_x - edge_x_l;

                        new_pos_y = ray_pos_y + ((ray_dir_y / (-ray_dir_x)) * dist_to_edge_l);
                        new_pos_x = (float)edge_x_l;

                        center_pos_x--;

                        hit_from_udlr = 4;
                    }
                    else
                    {
                        new_pos_y = (float)edge_y_u;

                        center_pos_y--;

                        hit_from_udlr = 2;
                    }
                }
                else if (angle_ray >= (float)M_PI * -0.25F && angle_ray < (float)M_PI * 0.25F)
                {
                    // Right //

                    dist_to_edge_r = edge_x_r - ray_pos_x;

                    new_pos_y = ray_pos_y + ((ray_dir_y / ray_dir_x) * dist_to_edge_r);

                    if (new_pos_y >= edge_y_d)
                    {
                        dist_to_edge_d = edge_y_d - ray_pos_y;

                        new_pos_x = ray_pos_x + ((ray_dir_x / ray_dir_y) * dist_to_edge_d);
                        new_pos_y = (float)edge_y_d;

                        center_pos_y++;

                        hit_from_udlr = 1;
                    }
                    else if (new_pos_y < edge_y_u)
                    {
                        dist_to_edge_u = ray_pos_y - edge_y_u;

                        new_pos_x = ray_pos_x + ((ray_dir_x / (-ray_dir_y)) * dist_to_edge_u);
                        new_pos_y = (float)edge_y_u;

                        center_pos_y--;

                        hit_from_udlr = 2;
                    }
                    else
                    {
                        new_pos_x = (float)edge_x_r;

                        center_pos_x++;

                        hit_from_udlr = 3;
                    }
                }
                else if (angle_ray >= (float)M_PI * 0.25F && angle_ray < (float)M_PI * 0.75F)
                {
                    // Down //

                    dist_to_edge_d = edge_y_d - ray_pos_y;

                    new_pos_x = ray_pos_x + ((ray_dir_x / ray_dir_y) * dist_to_edge_d);

                    if (new_pos_x >= edge_x_r)
                    {
                        dist_to_edge_r = edge_x_r - ray_pos_x;

                        new_pos_y = ray_pos_y + ((ray_dir_y / ray_dir_x) * dist_to_edge_r);
                        new_pos_x = (float)edge_x_r;

                        center_pos_x++;

                        hit_from_udlr = 3;
                    }
                    else if (new_pos_x < edge_x_l)
                    {
                        dist_to_edge_l = ray_pos_x - edge_x_l;

                        new_pos_y = ray_pos_y + ((ray_dir_y / (-ray_dir_x)) * dist_to_edge_l);
                        new_pos_x = (float)edge_x_l;

                        center_pos_x--;

                        hit_from_udlr = 4;
                    }
                    else
                    {
                        new_pos_y = (float)edge_y_d;

                        center_pos_y++;

                        hit_from_udlr = 1;
                    }
                }
                else
                {
                    // Left //

                    dist_to_edge_l = ray_pos_x - edge_x_l;

                    new_pos_y = ray_pos_y + ((ray_dir_y / (-ray_dir_x)) * dist_to_edge_l);

                    if (new_pos_y >= edge_y_d)
                    {
                        dist_to_edge_d = edge_y_d - ray_pos_y;

                        new_pos_x = ray_pos_x + ((ray_dir_x / ray_dir_y) * dist_to_edge_d);
                        new_pos_y = (float)edge_y_d;

                        center_pos_y++;

                        hit_from_udlr = 1;
                    }
                    else if (new_pos_y < edge_y_u)
                    {
                        dist_to_edge_u = ray_pos_y - edge_y_u;

                        new_pos_x = ray_pos_x + ((ray_dir_x / (-ray_dir_y)) * dist_to_edge_u);
                        new_pos_y = (float)edge_y_u;

                        center_pos_y--;

                        hit_from_udlr = 2;
                    }
                    else
                    {
                        new_pos_x = (float)edge_x_l;

                        center_pos_x--;

                        hit_from_udlr = 4;
                    }
                }

                ray_pos_x = new_pos_x;
                ray_pos_y = new_pos_y;
            }

            if (ray_pos_x < 0 || ray_pos_x >= level_size_x ||
                ray_pos_y < 0 || ray_pos_y >= level_size_y)
                break;
            else if (level_data[center_pos_y][center_pos_x] != 0)
                break;
        }

        float ray_from_to_x = ray_pos_x - player_x;
        float ray_from_to_y = ray_pos_y - player_y;

        float z_from_player = (ray_from_to_x * player_dir_x) + (ray_from_to_y * player_dir_y);

        z_list[x] = z_from_player;

        switch (hit_from_udlr)
        {
        case 1:
        case 2:
            texture_x_list[x] = fmodf(ray_pos_x, 1.0F);
            break;
        case 3:
        case 4:
            texture_x_list[x] = fmodf(ray_pos_y, 1.0F);
            break;
        }
    }

    // Rendering Routine //

    int wall_tex_width, wall_tex_height;
    int wall_tex_pitch;
    int wall_tex_channels;
    uint8_t *ptr_wall_tex_pixels = NULL;
    if (wall_texture != NULL)
    {
        wall_tex_width = wall_texture->w;
        wall_tex_height = wall_texture->h;

        wall_tex_pitch = wall_texture->pitch;

        wall_tex_channels = wall_tex_pitch / wall_tex_width;

        ptr_wall_tex_pixels = (uint8_t *)wall_texture->pixels;
    }

    uint8_t *pixel_buffer = NULL;
    int pitch;

    SDL_LockTexture(texture, NULL, (void **)&pixel_buffer, &pitch);

    // memset((void *)pixel_buffer, 0, screen_width * screen_height * screen_channels);

    float middle_y = screen_height / 2.0F;

    for (int x = 0; x < screen_width; x++)
    {
        int y = 0;

        // int pixel_buffer_offset = ((pixel_y_start * screen_width) + x) * screen_channels;
        int pixel_buffer_offset = x * screen_channels;
        int increment_per_row = (screen_width - 1) * screen_channels;

        uint8_t *ptr_pixel_buffer = pixel_buffer + pixel_buffer_offset;

        float current_z = z_list[x];

        if (current_z < z_cutoff)
            continue;

        float bar_height = 1.0F / current_z * height_z_one;
        float bar_height_half = bar_height / 2.0F;

        int start_y = (int)(middle_y - bar_height_half);
        int end_y = (int)(middle_y + bar_height_half);
        int range_y = end_y - start_y;

        float brightness = fmaxf(fade_distance - current_z, 0.0) / fade_distance;
        if (brightness <= 0.0F)
        {
            while (y < screen_height)
            {
                *ptr_pixel_buffer++ = 0;
                *ptr_pixel_buffer++ = 0;
                *ptr_pixel_buffer++ = 0;

                ptr_pixel_buffer += increment_per_row;

                y++;
            }

            continue;
        }

        brightness = fminf(fmaxf(0.0F, brightness), 1.0F);

        int pixel_y_start = start_y;
        if (pixel_y_start < 0)
            pixel_y_start = 0;

        int pixel_y_end = end_y;
        if (pixel_y_end >= screen_height)
            pixel_y_end = screen_height;

        while (y < pixel_y_start)
        {
            *ptr_pixel_buffer++ = 0;
            *ptr_pixel_buffer++ = 0;
            *ptr_pixel_buffer++ = 0;

            ptr_pixel_buffer += increment_per_row;

            y++;
        }

        if (wall_texture == NULL)
        {
            // Default White Wall Without Texture //

            uint8_t brightness_byte = (uint8_t)(brightness * 255.0F);

            while (y < pixel_y_end)
            {
                *ptr_pixel_buffer++ = brightness_byte;
                *ptr_pixel_buffer++ = brightness_byte;
                *ptr_pixel_buffer++ = brightness_byte;

                ptr_pixel_buffer += increment_per_row;

                y++;
            }
        }
        else
        {
            // Wall With Texture Loaded //

            int texture_x = (int)(texture_x_list[x] * (float)wall_tex_width);
            if (texture_x < 0)
                texture_x = 0;
            if (texture_x >= wall_tex_width)
                texture_x = wall_tex_width - 1;

            while (y < pixel_y_end)
            {
                int texture_y = (int)(((float)(y - start_y) / (float)range_y) * wall_tex_height);
                if (texture_y < 0)
                    texture_y = 0;
                if (texture_y >= wall_tex_height)
                    texture_y = wall_tex_height - 1;

                uint8_t *ptr_wall_tex = ptr_wall_tex_pixels + ((texture_y * wall_tex_pitch) + (texture_x * wall_tex_channels));

                *ptr_pixel_buffer++ = (uint8_t)((*ptr_wall_tex++) * brightness);
                *ptr_pixel_buffer++ = (uint8_t)((*ptr_wall_tex++) * brightness);
                *ptr_pixel_buffer++ = (uint8_t)((*ptr_wall_tex++) * brightness);

                ptr_pixel_buffer += increment_per_row;

                y++;
            }
        }

        while (y < screen_height)
        {
            *ptr_pixel_buffer++ = 0;
            *ptr_pixel_buffer++ = 0;
            *ptr_pixel_buffer++ = 0;

            ptr_pixel_buffer += increment_per_row;

            y++;
        }
    }

    SDL_UnlockTexture(texture);
}

static void RayCast_MouseMotion(SDL_Event *event)
{
    player_angle += event->motion.xrel * mouse_sensitivity;
    player_angle = RayCast_WrapAngle(player_angle);
}

static void RayCast_PlayerMovement(void)
{
    if (KeyStatesSDL_IsKeyDown(&key_states, SDL_SCANCODE_LEFT))
        player_angle -= player_turn_speed_per_tick;
    if (KeyStatesSDL_IsKeyDown(&key_states, SDL_SCANCODE_RIGHT))
        player_angle += player_turn_speed_per_tick;
    player_angle = RayCast_WrapAngle(player_angle);

    float player_angle_right = player_angle + (float)M_PI_2;

    float player_accel_forward_x = cosf(player_angle);
    float player_accel_forward_y = sinf(player_angle);

    float player_accel_right_x = cosf(player_angle_right);
    float player_accel_right_y = sinf(player_angle_right);

    float player_accel_x = 0.0F;
    float player_accel_y = 0.0F;

    if (KeyStatesSDL_IsKeyDown(&key_states, SDL_SCANCODE_A))
    {
        player_accel_x -= player_accel_right_x;
        player_accel_y -= player_accel_right_y;
    }
    if (KeyStatesSDL_IsKeyDown(&key_states, SDL_SCANCODE_D))
    {
        player_accel_x += player_accel_right_x;
        player_accel_y += player_accel_right_y;
    }
    
    if (KeyStatesSDL_IsKeyDown(&key_states, SDL_SCANCODE_S))
    {
        player_accel_x -= player_accel_forward_x;
        player_accel_y -= player_accel_forward_y;
    }
    if (KeyStatesSDL_IsKeyDown(&key_states, SDL_SCANCODE_W))
    {
        player_accel_x += player_accel_forward_x;
        player_accel_y += player_accel_forward_y;
    }

    float player_max_vel;
    if (KeyStatesSDL_IsKeyDown(&key_states, SDL_SCANCODE_LSHIFT))
        player_max_vel = player_vel_sprint_per_tick;
    else
        player_max_vel = player_vel_walk_per_tick;

    RayCast_VecClampLength(&player_accel_x, &player_accel_y, player_accel_per_tick);

    player_vel_x += player_accel_x;
    player_vel_y += player_accel_y;

    if (RayCast_Vec2Len(player_accel_x, player_accel_y) < 0.0001F)
    {
        player_vel_x *= player_fraction;
        player_vel_y *= player_fraction;

        if (RayCast_Vec2Len(player_vel_x, player_vel_y) < 0.0001F)
            player_vel_x = player_vel_y = 0.0F;
    }

    RayCast_VecClampLength(&player_vel_x, &player_vel_y, player_max_vel);

    player_x += player_vel_x;
    player_y += player_vel_y;
}

static void RayCast_DispatchEvents(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            quit = true;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            RayCast_MouseMotion(&event);
            break;
        case SDL_EVENT_KEY_DOWN:
            KeyStatesSDL_UpdateState(&key_states, event.key.scancode, true);
            break;
        case SDL_EVENT_KEY_UP:
            KeyStatesSDL_UpdateState(&key_states, event.key.scancode, false);
            break;
            /*
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            break;
            */
        }
    }
}

bool RayCast_Tick(void)
{
    RayCast_DispatchEvents();

    if (KeyStatesSDL_IsKeyDown(&key_states, SDL_SCANCODE_ESCAPE))
        quit = true;

    if (quit)
        return false;

    RayCast_PlayerMovement();

    RayCast_PlayerCollisionDetection();

    RayCast_DoRayCastAndRender();

    SDL_RenderTexture(renderer, texture, NULL, NULL);

    SDL_RenderPresent(renderer);

    return true;
}
