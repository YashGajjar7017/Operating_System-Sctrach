/**
 * @file vlc_app.c
 * @brief Windows 11 VLC Media Player Application with Animated Video Playback
 */

#include "vlc_app.h"
#include "../kstring.h"

typedef struct {
    const char *title;
    const char *duration;
    const char *codec;
    uint32_t total_sec;
} PlaylistItem;

#define PLAYLIST_COUNT 4
static PlaylistItem g_playlist[PLAYLIST_COUNT] = {
    {"matrix_intro_64.mp4",      "03:45", "1080p60 H.264", 225},
    {"cyber_sentinel_trailer.mp4","02:18", "4K HDR HEVC",   138},
    {"bloom_visualizer_demo.mp4", "04:12", "1080p60 AV1",   252},
    {"xenithra_security_doc.mp4", "05:30", "1080p30 H.264", 330}
};

static uint8_t g_is_playing = 1;
static int g_current_track = 0;
static uint32_t g_current_sec = 42;
static int g_volume = 85;
static uint64_t g_video_frame = 0;
static uint8_t g_show_playlist = 1;

void vlc_app_tick(void) {
    if (g_is_playing) {
        g_video_frame++;
        if ((g_video_frame % 20) == 0) {
            g_current_sec++;
            if (g_current_sec >= g_playlist[g_current_track].total_sec) {
                g_current_sec = 0;
                g_current_track = (g_current_track + 1) % PLAYLIST_COUNT;
            }
        }
    }
}

static void on_vlc_mouse(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click) {
    (void)win;
    (void)right_click;
    if (!left_click) return;

    /* Bottom Control Bar Area (relative Y: 340 - 410) */
    if (rel_y >= 350 && rel_y <= 410) {
        /* Play / Pause button (X: 60 - 92) */
        if (rel_x >= 60 && rel_x <= 96) {
            g_is_playing = !g_is_playing;
            return;
        }
        /* Stop button (X: 104 - 136) */
        if (rel_x >= 104 && rel_x <= 136) {
            g_is_playing = 0;
            g_current_sec = 0;
            return;
        }
        /* Prev button (X: 20 - 52) */
        if (rel_x >= 20 && rel_x <= 52) {
            if (g_current_track > 0) g_current_track--;
            else g_current_track = PLAYLIST_COUNT - 1;
            g_current_sec = 0;
            return;
        }
        /* Next button (X: 144 - 176) */
        if (rel_x >= 144 && rel_x <= 176) {
            g_current_track = (g_current_track + 1) % PLAYLIST_COUNT;
            g_current_sec = 0;
            return;
        }
        /* Playlist toggle button (X: 184 - 230) */
        if (rel_x >= 184 && rel_x <= 240) {
            g_show_playlist = !g_show_playlist;
            return;
        }
        /* Timeline scrubber click (Y: 350 - 362, X: 20 - 580) */
        if (rel_y >= 350 && rel_y <= 366 && rel_x >= 20 && rel_x <= 560) {
            int track_w = 540;
            int offset = rel_x - 20;
            if (offset < 0) offset = 0;
            if (offset > track_w) offset = track_w;
            uint32_t total = g_playlist[g_current_track].total_sec;
            g_current_sec = (total * offset) / track_w;
            return;
        }
        /* Volume Slider (X: 600 - 700) */
        if (rel_x >= 600 && rel_x <= 710) {
            int vol_offset = rel_x - 600;
            g_volume = (vol_offset * 100) / 100;
            if (g_volume < 0) g_volume = 0;
            if (g_volume > 100) g_volume = 100;
            return;
        }
    }

    /* Playlist sidebar clicks (if open) */
    if (g_show_playlist && rel_x >= 540 && rel_y >= 50 && rel_y <= 330) {
        int row = (rel_y - 80) / 44;
        if (row >= 0 && row < PLAYLIST_COUNT) {
            g_current_track = row;
            g_current_sec = 0;
            g_is_playing = 1;
            return;
        }
    }
}

/* Fast procedural sin/cos table approximation for 3D video synthesis */
static int fast_sin(int deg) {
    deg = deg % 360;
    if (deg < 0) deg += 360;
    if (deg < 90)  return (deg * 1000) / 90;
    if (deg < 180) return ((180 - deg) * 1000) / 90;
    if (deg < 270) return -((deg - 180) * 1000) / 90;
    return -((360 - deg) * 1000) / 90;
}

static int fast_cos(int deg) {
    return fast_sin(deg + 90);
}

static void render_animated_video_frame(int vx, int vy, int vw, int vh) {
    /* 1. Cinematic Background with ambient flow */
    gui_fill_rect(vx, vy, vw, vh, 0x00060911);

    /* Ambient dynamic spot */
    int spot_cx = vx + vw / 2 + (fast_sin(g_video_frame * 3) * 60) / 1000;
    int spot_cy = vy + vh / 2 + (fast_cos(g_video_frame * 2) * 30) / 1000;
    for (int r = 90; r > 0; r -= 15) {
        uint32_t glow = (r > 60) ? 0x000D1D3A : ((r > 30) ? 0x0017325F : 0x001E4280);
        gui_fill_rounded_rect(spot_cx - r * 2, spot_cy - r, r * 4, r * 2, r, glow);
    }

    /* 2. 3D Rotating Geometric Hologram Wireframe / Nodes */
    int center_x = vx + vw / 2;
    int center_y = vy + vh / 2 - 10;
    int angle = (int)(g_video_frame * 4) % 360;

    /* 8 Cube Vertices in 3D */
    int size = 50;
    int pts_x[8], pts_y[8];
    int raw_x[8] = {-size, size, size, -size, -size, size, size, -size};
    int raw_y[8] = {-size, -size, size, size, -size, -size, size, size};
    int raw_z[8] = {-size, -size, -size, -size, size, size, size, size};

    for (int i = 0; i < 8; i++) {
        /* Rotate around Y and X axis */
        int rx = (raw_x[i] * fast_cos(angle) - raw_z[i] * fast_sin(angle)) / 1000;
        int rz = (raw_x[i] * fast_sin(angle) + raw_z[i] * fast_cos(angle)) / 1000;
        int ry = (raw_y[i] * fast_cos(angle / 2) - rz * fast_sin(angle / 2)) / 1000;

        /* Perspective projection */
        int fov = 200;
        int proj_scale = (fov * 1000) / (fov + rz + 100);
        pts_x[i] = center_x + (rx * proj_scale) / 1000;
        pts_y[i] = center_y + (ry * proj_scale) / 1000;
    }

    /* Draw Cube Edges */
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    for (int e = 0; e < 12; e++) {
        int x1 = pts_x[edges[e][0]];
        int y1 = pts_y[edges[e][0]];
        int x2 = pts_x[edges[e][1]];
        int y2 = pts_y[edges[e][1]];

        /* Draw line step */
        int steps = 20;
        for (int s = 0; s <= steps; s++) {
            int lx = x1 + ((x2 - x1) * s) / steps;
            int ly = y1 + ((y2 - y1) * s) / steps;
            if (lx >= vx && lx < vx + vw && ly >= vy && ly < vy + vh) {
                gui_put_pixel(lx, ly, (e < 4) ? 0x0038BDF8 : ((e < 8) ? 0x00818CF8 : 0x00F43F5E));
            }
        }
    }

    /* Draw Vertex Glowing Nodes */
    for (int i = 0; i < 8; i++) {
        gui_fill_rounded_rect(pts_x[i] - 3, pts_y[i] - 3, 6, 6, 3, 0x00FFFFFF);
    }

    /* 3. Dynamic Audio Spectrum Equalizer (Bottom of video canvas) */
    int num_bars = 20;
    int bar_w = (vw - 40) / num_bars;
    for (int b = 0; b < num_bars; b++) {
        int beat = (fast_sin((b * 28 + (int)g_video_frame * 8) % 360) + 1000) / 20;
        int bar_h = (g_is_playing) ? (12 + (beat % 46)) : 6;
        int bx = vx + 20 + b * bar_w;
        int by = vy + vh - 20 - bar_h;

        uint32_t bcolor = (b < 6) ? GUI_ACCENT_ORANGE : ((b < 14) ? GUI_ACCENT_CYAN : GUI_ACCENT_PURPLE);
        gui_fill_rounded_rect(bx, by, bar_w - 3, bar_h, 2, bcolor);
    }

    /* 4. On-Screen Video Overlays (HUD) */
    gui_fill_rounded_rect(vx + 12, vy + 12, 180, 24, 4, 0x00101726);
    gui_draw_rect(vx + 12, vy + 12, 180, 24, GUI_BORDER_COLOR);
    gui_draw_string(vx + 18, vy + 16, g_playlist[g_current_track].codec, 0x0038BDF8, 1);

    if (g_is_playing) {
        gui_fill_rounded_rect(vx + vw - 70, vy + 12, 58, 24, 4, 0x0016A34A);
        gui_draw_string(vx + vw - 60, vy + 16, "PLAY", 0x00FFFFFF, 1);
    } else {
        gui_fill_rounded_rect(vx + vw - 80, vy + 12, 68, 24, 4, 0x00EA580C);
        gui_draw_string(vx + vw - 72, vy + 16, "PAUSE", 0x00FFFFFF, 1);
    }
}

static void on_vlc_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* 1. Main Background */
    gui_fill_rect(cx, cy, cw, ch, GUI_BG_WINDOW);

    /* 2. Top Menu Ribbon */
    int menu_h = 28;
    gui_fill_rect(cx, cy, cw, menu_h, 0x00141C2E);
    gui_draw_rect(cx, cy, cw, menu_h, GUI_BORDER_COLOR);

    gui_draw_fluent_icon_vlc(cx + 8, cy + 4);
    gui_draw_string(cx + 34, cy + 7, "VLC Media Player - 64-bit HD Engine", GUI_TEXT_PRIMARY, 1);

    gui_draw_string(cx + 300, cy + 7, "Media", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 350, cy + 7, "Playback", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 420, cy + 7, "Audio", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 470, cy + 7, "Video", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 520, cy + 7, "Subtitle", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 590, cy + 7, "Tools", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 640, cy + 7, "Help", GUI_TEXT_SECONDARY, 1);

    /* 3. Main Video Display Frame */
    int playlist_w = g_show_playlist ? 230 : 0;
    int video_x = cx + 8;
    int video_y = cy + menu_h + 6;
    int video_w = cw - 16 - playlist_w;
    int video_h = ch - menu_h - 100;

    render_animated_video_frame(video_x, video_y, video_w, video_h);
    gui_draw_rect(video_x, video_y, video_w, video_h, GUI_BORDER_COLOR);

    /* 4. Playlist Sidebar (if toggled) */
    if (g_show_playlist) {
        int pl_x = cx + cw - playlist_w - 4;
        int pl_y = video_y;
        int pl_h = video_h;

        gui_fill_rounded_rect(pl_x, pl_y, playlist_w, pl_h, 6, 0x000E1422);
        gui_draw_rect(pl_x, pl_y, playlist_w, pl_h, GUI_BORDER_COLOR);

        gui_draw_string(pl_x + 12, pl_y + 10, "Media Playlist", GUI_ACCENT_CYAN, 1);
        gui_draw_rect(pl_x + 8, pl_y + 28, playlist_w - 16, 1, GUI_BORDER_COLOR);

        for (int i = 0; i < PLAYLIST_COUNT; i++) {
            int row_y = pl_y + 36 + i * 44;
            uint32_t row_bg = (g_current_track == i) ? GUI_BG_CARD_HOVER : GUI_BG_CARD;
            gui_fill_rounded_rect(pl_x + 8, row_y, playlist_w - 16, 38, 4, row_bg);
            if (g_current_track == i) {
                gui_draw_rect(pl_x + 8, row_y, playlist_w - 16, 38, GUI_ACCENT_ORANGE);
            }

            /* Icon */
            gui_fill_rounded_rect(pl_x + 14, row_y + 8, 22, 22, 3, (g_current_track == i) ? GUI_ACCENT_ORANGE : GUI_ACCENT_BLUE);
            gui_draw_string(pl_x + 19, row_y + 11, (g_current_track == i && g_is_playing) ? ">" : "#", 0x00FFFFFF, 1);

            /* Title & duration */
            gui_draw_string(pl_x + 42, row_y + 6, g_playlist[i].title, (g_current_track == i) ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1);
            gui_draw_string(pl_x + 42, row_y + 22, g_playlist[i].duration, GUI_TEXT_MUTED, 1);
        }
    }

    /* 5. Bottom Playback Control Deck */
    int ctrl_y = cy + ch - 82;
    int ctrl_w = cw - 16;
    gui_fill_rounded_rect(cx + 8, ctrl_y, ctrl_w, 74, 8, 0x00131B2C);
    gui_draw_rect(cx + 8, ctrl_y, ctrl_w, 74, GUI_BORDER_COLOR);

    /* 5.1 Timeline Scrubber Bar */
    int time_bar_x = cx + 20;
    int time_bar_y = ctrl_y + 12;
    int time_bar_w = ctrl_w - 180;
    int time_bar_h = 6;

    gui_fill_rounded_rect(time_bar_x, time_bar_y, time_bar_w, time_bar_h, 3, 0x0024344E);

    uint32_t total = g_playlist[g_current_track].total_sec;
    int fill_w = (total > 0) ? (time_bar_w * (int)g_current_sec) / (int)total : 0;
    if (fill_w > time_bar_w) fill_w = time_bar_w;

    gui_fill_rounded_rect(time_bar_x, time_bar_y, fill_w, time_bar_h, 3, GUI_ACCENT_ORANGE);
    gui_fill_rounded_rect(time_bar_x + fill_w - 4, time_bar_y - 3, 12, 12, 6, 0x00FFFFFF);

    /* Timecode Readout */
    char time_str[32];
    char cur_m[8], cur_s[8], tot_m[8], tot_s[8];
    uint_to_str(g_current_sec / 60, cur_m);
    uint_to_str(g_current_sec % 60, cur_s);
    uint_to_str(total / 60, tot_m);
    uint_to_str(total % 60, tot_s);

    strcpy(time_str, cur_m);
    strcat(time_str, ":");
    if (g_current_sec % 60 < 10) strcat(time_str, "0");
    strcat(time_str, cur_s);
    strcat(time_str, " / ");
    strcat(time_str, tot_m);
    strcat(time_str, ":");
    if (total % 60 < 10) strcat(time_str, "0");
    strcat(time_str, tot_s);

    gui_draw_string(time_bar_x + time_bar_w + 14, time_bar_y - 2, time_str, GUI_TEXT_SECONDARY, 1);

    /* 5.2 Media Control Buttons */
    int btn_row_y = ctrl_y + 30;

    /* Previous Track [|<] */
    gui_fill_rounded_rect(cx + 20, btn_row_y, 34, 32, 6, GUI_BG_CARD);
    gui_draw_rect(cx + 20, btn_row_y, 34, 32, GUI_BORDER_COLOR);
    gui_draw_string(cx + 28, btn_row_y + 8, "|<", GUI_TEXT_PRIMARY, 1);

    /* Play / Pause [ > ] or [ || ] */
    gui_fill_rounded_rect(cx + 62, btn_row_y, 36, 32, 6, GUI_ACCENT_ORANGE);
    if (g_is_playing) {
        gui_draw_string(cx + 72, btn_row_y + 8, "||", 0x00FFFFFF, 1);
    } else {
        gui_draw_string(cx + 74, btn_row_y + 8, ">", 0x00FFFFFF, 1);
    }

    /* Stop [[]] */
    gui_fill_rounded_rect(cx + 106, btn_row_y, 34, 32, 6, GUI_BG_CARD);
    gui_draw_rect(cx + 106, btn_row_y, 34, 32, GUI_BORDER_COLOR);
    gui_draw_string(cx + 116, btn_row_y + 8, "[]", GUI_TEXT_PRIMARY, 1);

    /* Next Track [>|] */
    gui_fill_rounded_rect(cx + 148, btn_row_y, 34, 32, 6, GUI_BG_CARD);
    gui_draw_rect(cx + 148, btn_row_y, 34, 32, GUI_BORDER_COLOR);
    gui_draw_string(cx + 156, btn_row_y + 8, ">|", GUI_TEXT_PRIMARY, 1);

    /* Playlist toggle */
    gui_fill_rounded_rect(cx + 190, btn_row_y, 76, 32, 6, g_show_playlist ? GUI_BG_CARD_HOVER : GUI_BG_CARD);
    gui_draw_rect(cx + 190, btn_row_y, 76, 32, g_show_playlist ? GUI_ACCENT_ORANGE : GUI_BORDER_COLOR);
    gui_draw_string(cx + 200, btn_row_y + 8, "Playlist", GUI_TEXT_PRIMARY, 1);

    /* Speed indicator */
    gui_fill_rounded_rect(cx + 274, btn_row_y, 50, 32, 6, GUI_BG_CARD);
    gui_draw_rect(cx + 274, btn_row_y, 50, 32, GUI_BORDER_COLOR);
    gui_draw_string(cx + 284, btn_row_y + 8, "1.0x", GUI_ACCENT_CYAN, 1);

    /* Volume Slider */
    int vol_x = cx + ctrl_w - 170;
    gui_draw_string(vol_x - 36, btn_row_y + 8, "VOL", GUI_TEXT_MUTED, 1);
    gui_fill_rounded_rect(vol_x, btn_row_y + 12, 100, 8, 4, 0x0024344E);
    gui_fill_rounded_rect(vol_x, btn_row_y + 12, (100 * g_volume) / 100, 8, 4, GUI_ACCENT_CYAN);
    gui_fill_rounded_rect(vol_x + (100 * g_volume) / 100 - 4, btn_row_y + 8, 12, 16, 4, 0x00FFFFFF);

    char vol_str[8];
    uint_to_str(g_volume, vol_str);
    strcat(vol_str, "%");
    gui_draw_string(vol_x + 110, btn_row_y + 8, vol_str, GUI_TEXT_SECONDARY, 1);
}

Window* vlc_app_launch(void) {
    Window *win = window_create(
        "VLC Media Player - 64-bit",
        "vlc",
        80,
        50,
        780,
        500,
        on_vlc_paint,
        NULL
    );
    if (win) {
        window_set_callbacks(win, NULL, on_vlc_mouse);
    }
    return win;
}
