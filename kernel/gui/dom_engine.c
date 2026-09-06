/**
 * @file dom_engine.c
 * @brief Declarative DOM-style UI Engine implementation for Xenithra OS
 */

#include "dom_engine.h"

#define MAX_DOM_NODES 128
static DOMElement g_node_pool[MAX_DOM_NODES];
static int g_node_index = 0;

static void str_cpy(char *dst, const char *src, size_t max) {
    size_t i = 0;
    while (i + 1 < max && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

DOMElement* dom_create_element(DOMElementType type, DOMStyle style, const char *text) {
    if (g_node_index >= MAX_DOM_NODES) {
        g_node_index = 0; /* Recycle pool */
    }
    DOMElement *elem = &g_node_pool[g_node_index++];
    elem->type = type;
    elem->style = style;
    elem->child_count = 0;
    elem->rel_x = 0;
    elem->rel_y = 0;
    elem->width = 0;
    elem->height = 0;
    elem->progress_val = 0;
    elem->progress_max = 100;

    if (text) {
        str_cpy(elem->text, text, sizeof(elem->text));
    } else {
        elem->text[0] = '\0';
    }

    return elem;
}

void dom_add_child(DOMElement *parent, DOMElement *child) {
    if (!parent || !child || parent->child_count >= 8) return;
    parent->children[parent->child_count++] = child;
}

DOMElement* dom_create_card(const char *title, uint32_t bg_color) {
    DOMStyle s = {0};
    s.bg_color = bg_color;
    s.border_color = GUI_BORDER_COLOR;
    s.border_radius = 8;
    s.padding = 12;
    s.text_color = GUI_TEXT_WHITE;
    s.text_scale = 1;
    return dom_create_element(DOM_CARD, s, title);
}

DOMElement* dom_create_badge(const char *label, uint32_t badge_color) {
    DOMStyle s = {0};
    s.bg_color = badge_color;
    s.border_color = 0;
    s.border_radius = 4;
    s.padding = 6;
    s.text_color = 0x00FFFFFF;
    s.text_scale = 1;
    return dom_create_element(DOM_BADGE, s, label);
}

DOMElement* dom_create_button(const char *label, uint32_t btn_color) {
    DOMStyle s = {0};
    s.bg_color = btn_color;
    s.border_color = GUI_BORDER_COLOR;
    s.border_radius = 6;
    s.padding = 8;
    s.text_color = 0x00FFFFFF;
    s.text_scale = 1;
    return dom_create_element(DOM_BUTTON, s, label);
}

DOMElement* dom_create_progress(int current, int max, uint32_t fill_color) {
    DOMStyle s = {0};
    s.bg_color = 0x00334155;
    s.border_color = fill_color;
    s.border_radius = 4;
    s.text_scale = 1;
    DOMElement *el = dom_create_element(DOM_PROGRESS_BAR, s, "");
    el->progress_val = current;
    el->progress_max = max;
    return el;
}

void dom_render_tree(DOMElement *root, int base_x, int base_y, int container_w, int container_h) {
    if (!root) return;

    int cur_x = base_x + root->style.margin;
    int cur_y = base_y + root->style.margin;
    int w = (root->width > 0) ? root->width : (container_w - 2 * root->style.margin);
    int h = (root->height > 0) ? root->height : (container_h - 2 * root->style.margin);

    switch (root->type) {
        case DOM_CARD:
            gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            gui_draw_rect(cur_x, cur_y, w, h, root->style.border_color);
            if (root->text[0] != '\0') {
                gui_draw_string(cur_x + root->style.padding, cur_y + root->style.padding, root->text, root->style.text_color, root->style.text_scale);
            }
            break;

        case DOM_BUTTON:
            gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            gui_draw_rect(cur_x, cur_y, w, h, root->style.border_color);
            gui_draw_string(cur_x + root->style.padding, cur_y + (h - 16) / 2, root->text, root->style.text_color, 1);
            break;

        case DOM_BADGE:
            gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            gui_draw_string(cur_x + 6, cur_y + 4, root->text, root->style.text_color, 1);
            break;

        case DOM_PROGRESS_BAR: {
            gui_fill_rounded_rect(cur_x, cur_y, w, h, root->style.border_radius, root->style.bg_color);
            int fill_w = (root->progress_max > 0) ? (w * root->progress_val) / root->progress_max : 0;
            if (fill_w > w) fill_w = w;
            if (fill_w > 0) {
                gui_fill_rounded_rect(cur_x, cur_y, fill_w, h, root->style.border_radius, root->style.border_color);
            }
            break;
        }

        case DOM_TEXT:
            gui_draw_string(cur_x, cur_y, root->text, root->style.text_color, root->style.text_scale);
            break;

        default:
            if (root->style.bg_color) {
                gui_fill_rect(cur_x, cur_y, w, h, root->style.bg_color);
            }
            break;
    }

    /* Render Children */
    int child_y = cur_y + 36;
    for (int i = 0; i < root->child_count; i++) {
        dom_render_tree(root->children[i], cur_x + 12, child_y, w - 24, 40);
        child_y += 48;
    }
}
