SET(SOURCES screens/ui_screen1.c
    screens/ui_screen2.c
    screens/ui_Screen3.c
    screens/ui_Screen6.c
    screens/ui_Screen4.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_ksm_png.c
    images/ui_img_ako_png.c
    images/ui_img_1_png.c
    images/ui_img_2_png.c
    images/ui_img_3_png.c
    images/ui_img_4_png.c
    fonts/ui_font_Chinese1.c)

add_library(ui ${SOURCES})
