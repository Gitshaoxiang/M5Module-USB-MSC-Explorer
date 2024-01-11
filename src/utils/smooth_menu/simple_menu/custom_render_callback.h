#include "simple_menu.h"
#include <M5GFX.h>
#include "device_def.h"

class CustomMenuCallback : public SMOOTH_MENU::SimpleMenuCallback_t {
   private:
    M5Canvas* _canvas;

   public:
    CustomMenuCallback(M5Canvas* canvas) : _canvas(canvas){};

    LVGL::Anim_Path menuAnim;
    LVGL::Anim_Path pickAnim;

    /* Override render callback */
    void renderCallback(const std::vector<SMOOTH_MENU::Item_t*>& menuItemList,
                        const SMOOTH_MENU::RenderAttribute_t& selector,
                        const SMOOTH_MENU::RenderAttribute_t& camera) override;
};
