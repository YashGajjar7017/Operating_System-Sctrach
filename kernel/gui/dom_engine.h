/**
 * @file dom_engine.h
 * @brief Declarative HTML/CSS DOM-like UI Engine for Xenithra OS Applications
 */

#ifndef _KERNEL_GUI_DOM_ENGINE_H_
#define _KERNEL_GUI_DOM_ENGINE_H_

#include <stdint.h>
#include <stddef.h>
#include "compositor.h"

typedef enum {
    DOM_DIV = 0,
    DOM_CARD,
    DOM_FLEX_ROW,
    DOM_FLEX_COL,
    DOM_TEXT,
    DOM_BUTTON,
    DOM_PROGRESS_BAR,
    DOM_BADGE
} DOMElementType;

typedef struct {
    uint32_t bg_color;
    uint32_t border_color;
    uint32_t text_color;
    uint32_t hover_color;
    uint16_t border_radius;
    uint16_t padding;
    uint16_t margin;
    uint8_t  text_scale;
    uint8_t  has_gradient;
    uint32_t gradient_bot;
} DOMStyle;

typedef struct DOMElement DOMElement;

struct DOMElement {
    DOMElementType type;
    DOMStyle style;
    char text[128];
    int progress_val;
    int progress_max;
    
    int rel_x, rel_y;
    int width, height;
    
    DOMElement *children[8];
    int child_count;
};

/* DOM Engine API */
DOMElement* dom_create_element(DOMElementType type, DOMStyle style, const char *text);
void dom_add_child(DOMElement *parent, DOMElement *child);
void dom_render_tree(DOMElement *root, int base_x, int base_y, int container_w, int container_h);

/* Pre-styled Quick Component Helpers */
DOMElement* dom_create_card(const char *title, uint32_t bg_color);
DOMElement* dom_create_badge(const char *label, uint32_t badge_color);
DOMElement* dom_create_button(const char *label, uint32_t btn_color);
DOMElement* dom_create_progress(int current, int max, uint32_t fill_color);

#endif /* _KERNEL_GUI_DOM_ENGINE_H_ */
