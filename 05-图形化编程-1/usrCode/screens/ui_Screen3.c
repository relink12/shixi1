// ui_Screen3.c
#include "../ui.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

// 全局变量（LVGL统一管理）
lv_obj_t * ui_Screen3 = NULL;
lv_obj_t * ui_Button12 = NULL;
lv_obj_t * ui_Label11 = NULL;
lv_obj_t * ui_slide_img = NULL;  // LVGL图片控件（显示幻灯片）
static lv_timer_t *slide_timer = NULL;
static int slide_paused = 0;
static int current_pic_idx = 0;
// 图片路径（保持和你的一致）
static char const *pic_paths[] = {
    "/mydata/1.bmp",
    "/mydata/2.bmp",
    "/mydata/3.bmp"
};
#define PIC_COUNT 3
#define LCD_W 800
#define LCD_H 480

// ------------ 关键修改：BMP图片读取→转LVGL颜色数据（不操作fb0）------------
int pic_show_lvgl(char const *pic_path) {
    // 1. 打开BMP图片
    int pic_fd = open(pic_path, O_RDONLY);
    if (pic_fd == -1) {
        printf("图片打开失败：%s\n", pic_path);
        return -1;
    }

    // 2. 读取BMP头（54字节）
    char bmp_head[54];
    read(pic_fd, bmp_head, 54);
    // 验证是否是24位BMP（避免格式错误）
    if ((*(uint16_t *)&bmp_head[0]) != 0x4D42) { // BMP标识：0x4D42
        printf("不是BMP图片：%s\n", pic_path);
        close(pic_fd);
        return -1;
    }
    if ((*(uint16_t *)&bmp_head[28]) != 24) { // 24位真彩色
        printf("不是24位BMP：%s\n", pic_path);
        close(pic_fd);
        return -1;
    }

    // 3. 读取BMP像素数据（BGR格式）
    char bgr_buf[LCD_W * LCD_H * 3];
    lseek(pic_fd, 54, SEEK_SET); // 跳过头部，直接读像素
    int read_len = read(pic_fd, bgr_buf, sizeof(bgr_buf));
    if (read_len != sizeof(bgr_buf)) {
        printf("图片数据不完整：%s\n", pic_path);
        close(pic_fd);
        return -1;
    }
    close(pic_fd);

    // 4. BGR→RGB（LVGL默认RGB格式）+ 上下颠倒（适配显示）
    static lv_color_t lv_rgb_buf[LCD_W * LCD_H]; // LVGL颜色数组
    for (int y = 0; y < LCD_H; y++) {
        for (int x = 0; x < LCD_W; x++) {
            // BMP是BGR+上下颠倒，转换为LVGL的RGB
            int bmp_idx = (LCD_H - 1 - y) * LCD_W * 3 + x * 3; // 颠倒行
            uint8_t b = bgr_buf[bmp_idx];
            uint8_t g = bgr_buf[bmp_idx + 1];
            uint8_t r = bgr_buf[bmp_idx + 2];
            // 赋值给LVGL颜色（RGB888）
            lv_rgb_buf[y * LCD_W + x] = lv_color_make(r, g, b);
        }
    }

    // 5. 用LVGL图片控件显示（核心：LVGL管理图层，按钮不被遮挡）
    lv_img_dsc_t img_dsc = {
        .header.always_zero = 0,
        .header.w = LCD_W,
        .header.h = LCD_H,
        .data_size = LCD_W * LCD_H * sizeof(lv_color_t),
        .header.cf = LV_IMG_CF_TRUE_COLOR, // 真彩色格式
        .data = lv_rgb_buf, // 转换后的LVGL颜色数据
    };
    lv_img_set_src(ui_slide_img, &img_dsc); // 显示到图片控件

    return 0;
}

// ------------ 定时器回调：1秒切换图片 ------------
static void slide_timer_cb(lv_timer_t *timer) {
    if (slide_paused) return;
    current_pic_idx = (current_pic_idx + 1) % PIC_COUNT;
    pic_show_lvgl(pic_paths[current_pic_idx]);
}

// ------------ 触摸暂停/继续 ------------
static void ui_event_screen3_touch(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        slide_paused = !slide_paused;
    }
}

// ------------ 启动幻灯片 ------------
void start_slide_show(void) {
    slide_paused = 0;
    current_pic_idx = 0;
    pic_show_lvgl(pic_paths[0]); // 显示第一张图
    // 启动定时器（1秒切换）
        slide_timer = lv_timer_create(slide_timer_cb, 1000, NULL);

}

// ------------ 停止幻灯片（释放资源）------------
void stop_slide_show(void) {
    if (slide_timer) {
        lv_timer_del(slide_timer);
        slide_timer = NULL;
    }
}

// ------------ 菜单按钮：返回主菜单（修复死机）------------
void ui_event_Button12(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        stop_slide_show(); // 停止播放
        // 正确切换页面：先加载主菜单，再销毁当前页面（避免LVGL对象树错误）
        _ui_screen_change(&ui_screen2, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_screen2_screen_init);
        ui_Screen3_screen_destroy(); // 后销毁，避免active screen被删除
    }
}

// ------------ 页面初始化：创建图片控件+按钮（靠创建顺序控制图层，后创建的在上层）------------
void ui_Screen3_screen_init(void) {
    // 1. 创建页面容器
    ui_Screen3 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ui_Screen3, LCD_W, LCD_H); // 适配屏幕尺寸

    // 2. 先创建图片控件（底层，因为先创建）
    ui_slide_img = lv_img_create(ui_Screen3);
    lv_obj_set_size(ui_slide_img, LCD_W, LCD_H);
    lv_obj_set_align(ui_slide_img, LV_ALIGN_CENTER); // 居中显示
    // 删掉这行：lv_obj_set_z_index(ui_slide_img, 0);

    // 3. 后创建菜单按钮（上层，因为后创建，自动覆盖在图片上面）
    ui_Button12 = lv_btn_create(ui_Screen3);
    lv_obj_set_size(ui_Button12, 100, 50);
    lv_obj_set_pos(ui_Button12, LCD_W - 110, 10); // 右上角（x=690，y=10，更易点击）
    // 删掉这行：lv_obj_set_z_index(ui_Button12, 10);
    lv_obj_add_flag(ui_Button12, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_Button12, LV_OBJ_FLAG_SCROLLABLE);

    // 按钮文字（不变）
    ui_Label11 = lv_label_create(ui_Button12);
    lv_obj_set_align(ui_Label11, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label11, "菜单");
    lv_obj_set_style_text_font(ui_Label11, &ui_font_Chinese1, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 4. 页面触摸事件（暂停/继续）
    lv_obj_add_event_cb(ui_Screen3, ui_event_screen3_touch, LV_EVENT_CLICKED, NULL);
    // 5. 按钮事件绑定（不变）
    lv_obj_add_event_cb(ui_Button12, ui_event_Button12, LV_EVENT_ALL, NULL);
}

// ------------ 页面销毁：正确释放所有LVGL对象 ------------
void ui_Screen3_screen_destroy(void) {
    stop_slide_show();
    // 按顺序销毁（子对象→父对象）
    if (ui_Label11) lv_obj_del(ui_Label11);
    if (ui_Button12) lv_obj_del(ui_Button12);
    if (ui_slide_img) lv_obj_del(ui_slide_img);
    if (ui_Screen3) lv_obj_del(ui_Screen3);
    // 置空指针，避免野指针
    ui_Screen3 = NULL;
    ui_Button12 = NULL;
    ui_Label11 = NULL;
    ui_slide_img = NULL;
}