/**
 * @file dom_engine.c
 * @brief In-Kernel Reactive Virtual DOM & Declarative JSX-Style Component UI Engine
 */

#include "dom_engine.h"
#include "../kstring.h"

#define MAX_VDOM_NODES 256
static VNode g_node_pool[MAX_VDOM_NODES];
static int g_node_index = 0;

void vdom_reset_pool(void) {
    g_node_index = 0;
}

static void str_cpy(char *dst, const char *src, size_t max) {
    size_t i = 0;
    while (i + 1 < max && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

VNode* vdom_create_node(VNodeType type, VNodeStyle style, const char *text) {
    if (g_node_index >= MAX_VDOM_NODES) {
        g_node_index = 0; /* Recycle pool */
    }
    VNode *node = &g_node_pool[g_node_index++];
    node->type = type;
    node->style = style;
    node->child_count = 0;
    node->x = 0;
    node->y = 0;
    node->width = 0;
    node->height = 0;
    node->progress_val = 0;
    node->progress_max = 100;
    node->on_click = NULL;
    node->user_data = NULL;
    node->icon_tag[0] = '\0';

    if (text) {
        str_cpy(node->text, text, sizeof(node->text));
    } else {
        node->text[0] = '\0';
    }

    return node;
}

void vdom_add_child(VNode *parent, VNode *child) {
    if (!parent || !child || parent->child_count >= 12) return;
    parent->children[parent->child_count++] = child;
}

VNode* jsx_div(uint32_t bg_color, uint16_t radius) {
    VNodeStyle s = {0};
    s.bg_color = bg_color;
    s.border_radius = radius;
    return vdom_create_node(VNODE_DIV, s, "");
}

VNode* jsx_flex_row(uint16_t gap, FlexAlign justify, FlexAlign align) {
    VNodeStyle s = {0};
    s.gap = gap;
    s.justify = justify;
    s.align = align;
    return vdom_create_node(VNODE_FLEX_ROW, s, "");
}

VNode* jsx_flex_col(uint16_t gap, FlexAlign justify, FlexAlign align) {
    VNodeStyle s = {0};
    s.gap = gap;
    s.justify = justify;
    s.align = align;
    return vdom_create_node(VNODE_FLEX_COL, s, "");
}

VNode* jsx_card(const char *title, uint32_t bg_color, uint32_t border_color) {
    VNodeStyle s = {0};
    s.bg_color = bg_color;
    s.border_color = border_color;
    s.border_radius = 8;
    s.padding = 10;
    s.text_color = GUI_TEXT_PRIMARY;
    s.text_scale = 1;
    return vdom_create_node(VNODE_CARD, s, title);
}

VNode* jsx_button(const char *label, const char *icon_tag, uint32_t btn_color, VNodeClickCallback on_click) {
    VNodeStyle s = {0};
    s.bg_color = btn_color;
    s.border_color = GUI_BORDER_COLOR;
    s.border_radius = 6;
    s.padding = 8;
    s.text_color = 0x00FFFFFF;
    s.text_scale = 1;
    VNode *n = vdom_create_node(VNODE_BUTTON, s, label);
    if (icon_tag) str_cpy(n->icon_tag, icon_tag, sizeof(n->icon_tag));
    n->on_click = on_click;
    return n;
}

VNode* jsx_badge(const char *label, uint32_t badge_color) {
    VNodeStyle s = {0};
    s.bg_color = badge_color;
    s.border_radius = 4;
    s.padding = 4;
    s.text_color = 0x00FFFFFF;
    s.text_scale = 1;
    return vdom_create_node(VNODE_BADGE, s, label);
}

VNode* jsx_progress(int current, int max, uint32_t fill_color) {
    VNodeStyle s = {0};
    s.bg_color = 0x0024344E;
    s.border_color = fill_color;
    s.border_radius = 4;
    VNode *n = vdom_create_node(VNODE_PROGRESS, s, "");
    n->progress_val = current;
    n->progress_max = max;
    return n;
}

VNode* jsx_text(const char *str, uint32_t color, int scale) {
    VNodeStyle s = {0};
    s.text_color = color;
    s.text_scale = scale;
    return vdom_create_node(VNODE_TEXT, s, str);
}

void vdom_render_tree(VNode *root, int base_x, int base_y, int container_w, int container_h) {
    if (!root) return;

    int cur_x = base_x + root->style.margin;
    int cur_y = base_y + root->style.margin;
    int w = (root->width > 0) ? root->width : (container_w - 2 * root->style.margin);
    int h = (root->height > 0) ? root->height : (container_h - 2 * root->style.margin);

    root->x = cur_x;
    root->y = cur_y;
    root->width = w;
    root->height = h;

    switch (root->type) {
        case VNODE_CARD:
            gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            if (root->style.border_color) {
                gui_draw_rect(cur_x, cur_y, w, h, root->style.border_color);
            }
            if (root->text[0] != '\0') {
                gui_draw_string(cur_x + root->style.padding, cur_y + root->style.padding, root->text, root->style.text_color, root->style.text_scale);
            }
            break;

        case VNODE_BUTTON:
            gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            if (root->style.border_color) {
                gui_draw_rect(cur_x, cur_y, w, h, root->style.border_color);
            }
            if (root->icon_tag[0] != '\0') {
                gui_draw_fluent_icon_by_tag(cur_x + 6, cur_y + (h - 24) / 2, root->icon_tag);
                gui_draw_string(cur_x + 36, cur_y + (h - 16) / 2, root->text, root->style.text_color, 1);
            } else {
                gui_draw_string(cur_x + root->style.padding, cur_y + (h - 16) / 2, root->text, root->style.text_color, 1);
            }
            break;

        case VNODE_BADGE:
            gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            gui_draw_string(cur_x + 6, cur_y + 4, root->text, root->style.text_color, 1);
            break;

        case VNODE_PROGRESS: {
            gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            int fill_w = (root->progress_max > 0) ? (w * root->progress_val) / root->progress_max : 0;
            if (fill_w > w) fill_w = w;
            if (fill_w > 0) {
                gui_fill_rounded_rect(cur_x, cur_y, fill_w, h, root->style.border_radius, root->style.border_color);
            }
            break;
        }

        case VNODE_TEXT:
            gui_draw_string(cur_x, cur_y, root->text, root->style.text_color, root->style.text_scale ? root->style.text_scale : 1);
            break;

        case VNODE_FLEX_ROW: {
            if (root->style.bg_color) {
                gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            }
            if (root->child_count > 0) {
                int child_w = (w - (root->child_count - 1) * root->style.gap) / root->child_count;
                int child_x = cur_x;
                for (int i = 0; i < root->child_count; i++) {
                    vdom_render_tree(root->children[i], child_x, cur_y, child_w, h);
                    child_x += child_w + root->style.gap;
                }
            }
            return;
        }

        case VNODE_FLEX_COL: {
            if (root->style.bg_color) {
                gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            }
            if (root->child_count > 0) {
                int child_h = (h - (root->child_count - 1) * root->style.gap) / root->child_count;
                int child_y = cur_y;
                for (int i = 0; i < root->child_count; i++) {
                    vdom_render_tree(root->children[i], cur_x, child_y, w, child_h);
                    child_y += child_h + root->style.gap;
                }
            }
            return;
        }

        default:
            if (root->style.bg_color) {
                gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            }
            break;
    }

    /* Render nested children vertically by default */
    int nested_y = cur_y + 36;
    for (int i = 0; i < root->child_count; i++) {
        vdom_render_tree(root->children[i], cur_x + 10, nested_y, w - 20, 36);
        nested_y += 42;
    }
}

uint8_t vdom_dispatch_click(VNode *root, int click_x, int click_y) {
    if (!root) return 0;

    if (click_x >= root->x && click_x <= root->x + root->width &&
        click_y >= root->y && click_y <= root->y + root->height) {
        
        /* Dispatch to children first */
        for (int i = 0; i < root->child_count; i++) {
            if (vdom_dispatch_click(root->children[i], click_x, click_y)) {
                return 1;
            }
        }

        if (root->on_click) {
            root->on_click(root, root->user_data);
            return 1;
        }
    }
    return 0;
}
