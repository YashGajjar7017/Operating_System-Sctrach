/**
 * @file dom_engine.h
 * @brief In-Kernel Reactive Virtual DOM & Declarative JSX-Style Component UI Engine
 */

#ifndef _KERNEL_GUI_DOM_ENGINE_H_
#define _KERNEL_GUI_DOM_ENGINE_H_

#include <stdint.h>
#include <stddef.h>
#include "compositor.h"

typedef enum {
    VNODE_DIV = 0,
    VNODE_FLEX_ROW,
    VNODE_FLEX_COL,
    VNODE_CARD,
    VNODE_BUTTON,
    VNODE_BADGE,
    VNODE_INPUT,
    VNODE_PROGRESS,
    VNODE_ICON,
    VNODE_TEXT
} VNodeType;

typedef enum {
    FLEX_START = 0,
    FLEX_CENTER,
    FLEX_BETWEEN,
    FLEX_END
} FlexAlign;

typedef struct {
    uint32_t bg_color;
    uint32_t border_color;
    uint32_t text_color;
    uint32_t hover_color;
    uint32_t gradient_bot;
    uint16_t border_radius;
    uint16_t padding;
    uint16_t margin;
    uint16_t gap;
    uint8_t  text_scale;
    uint8_t  has_gradient;
    uint8_t  has_shadow;
    FlexAlign justify;
    FlexAlign align;
} VNodeStyle;

typedef struct VNode VNode;
typedef void (*VNodeClickCallback)(VNode *node, void *user_data);

struct VNode {
    VNodeType type;
    VNodeStyle style;
    char text[128];
    char icon_tag[16];
    int progress_val;
    int progress_max;
    
    int x, y, width, height;
    
    VNodeClickCallback on_click;
    void *user_data;
    
    VNode *children[12];
    int child_count;
};

/* Core Virtual DOM Engine API */
VNode* vdom_create_node(VNodeType type, VNodeStyle style, const char *text);
void   vdom_add_child(VNode *parent, VNode *child);
void   vdom_render_tree(VNode *root, int base_x, int base_y, int container_w, int container_h);
uint8_t vdom_dispatch_click(VNode *root, int click_x, int click_y);
void   vdom_reset_pool(void);

/* JSX-Style Declarative Component DSL */
VNode* jsx_div(uint32_t bg_color, uint16_t radius);
VNode* jsx_flex_row(uint16_t gap, FlexAlign justify, FlexAlign align);
VNode* jsx_flex_col(uint16_t gap, FlexAlign justify, FlexAlign align);
VNode* jsx_card(const char *title, uint32_t bg_color, uint32_t border_color);
VNode* jsx_button(const char *label, const char *icon_tag, uint32_t btn_color, VNodeClickCallback on_click);
VNode* jsx_badge(const char *label, uint32_t badge_color);
VNode* jsx_progress(int current, int max, uint32_t fill_color);
VNode* jsx_text(const char *str, uint32_t color, int scale);

#endif /* _KERNEL_GUI_DOM_ENGINE_H_ */
