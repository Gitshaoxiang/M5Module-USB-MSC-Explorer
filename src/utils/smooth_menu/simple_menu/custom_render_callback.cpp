#include "custom_render_callback.h"

/* Override render callback */
void CustomMenuCallback::renderCallback(
    const std::vector<SMOOTH_MENU::Item_t*>& menuItemList,
    const SMOOTH_MENU::RenderAttribute_t& selector,
    const SMOOTH_MENU::RenderAttribute_t& camera) {
    /* Render menu */
    _canvas->fillScreen(BLACK);

    int anim_menu_value   = menuAnim.getValue(millis());
    float anim_pick_value = pickAnim.getValue(millis());

    int menu_start_y_offset = 0;
    for (const auto& item : menuItemList) {
        int y_offset = item->y - selector.y + menu_start_y_offset;

        file_info_t* userData = (file_info_t*)item->userData;
        if (item->id == selector.targetItem) {
            _canvas->setTextSize(1);
            _canvas->setTextColor(WHITE);
            _canvas->drawString(item->tag.c_str(), item->x, y_offset);
            _canvas->fillRect(item->x + 220, y_offset, 100, 35, BLACK);
            _canvas->drawString(String(userData->size / 1024.0) + "KB",
                                item->x + 230, y_offset);
            _canvas->fillRect(0, y_offset + 20, anim_pick_value, 4, 0x45f);

        } else {
            _canvas->setTextSize(1);
            _canvas->setTextColor(DARKGREY);
            _canvas->drawString(item->tag.c_str(), item->x, y_offset);
            _canvas->fillRect(item->x + 220, y_offset, 100, 35, BLACK);
            _canvas->drawString(String(userData->size / 1024.0) + "KB",
                                item->x + 230, y_offset);
        }
    }

    _canvas->pushSprite(0, 55);
}
