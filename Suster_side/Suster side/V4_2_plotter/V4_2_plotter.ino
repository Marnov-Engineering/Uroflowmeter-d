#include <TFT_eSPI.h>
#include <lvgl.h>

#define TFT_HOR_RES   480
#define TFT_VER_RES   320

TFT_eSPI tft = TFT_eSPI(); 
int batt = 100;
int uro = 0;
bool bPage_plotter = false;
uint32_t LastTime;
static int simulated_value = 0;
const int potPin = 35;


#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

unsigned long lastTickMillis = 0;
lv_obj_t *scaling_rect;
lv_obj_t *batts;
lv_obj_t *screen;
lv_obj_t *screen_plotter;
lv_obj_t *chart1;
lv_obj_t *chart2;
lv_chart_series_t * ser1;
lv_chart_series_t * ser2;
static lv_subject_t fw_download_percent_subject;
static lv_subject_t fw_update_status_subject;

typedef enum {
    FW_UPDATE_STATE_IDLE,
    FW_UPDATE_STATE_CONNECTING,
    FW_UPDATE_STATE_START_CONNECTING,
    FW_UPDATE_STATE_S_CONNECTING,
    FW_UPDATE_STATE_CONNECTED,
    FW_UPDATE_STATE_DOWNLOADING,
    FW_UPDATE_STATE_CANCEL,
    FW_UPDATE_STATE_READY,
    FW_UPDATE_STATE_START_READY,
    FW_UPDATE_STATE_START_READY_D,
    FW_UPDATE_STATE_START_FINISH,
} fw_update_state_t;

void update_rectangle_width(int batt) {
    batt = constrain(batt, 0, 100);
    int new_width = map(batt, 0, 100, 0, 100);
    lv_obj_set_size(scaling_rect, new_width, 40);
}

void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t x, y;
    bool touched = tft.getTouch(&x, &y);

    if (touched) {
        Serial.printf("x: %d, y: %d\n", x, y);
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = map(x, 0, 480, 0, TFT_HOR_RES);
        data->point.y = map(y, 0, 320, 0, TFT_VER_RES);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void *draw_buf;

static void anim_x_cb(void *var, int32_t v) {
    lv_obj_set_x((lv_obj_t *)var, v);
}

// static void animate_pages(lv_obj_t *current_page, lv_obj_t *next_page) {
//     lv_anim_t anim_out, anim_in;

//     lv_anim_init(&anim_out);
//     lv_anim_set_var(&anim_out, current_page);
//     lv_anim_set_exec_cb(&anim_out, anim_x_cb);
//     lv_anim_set_values(&anim_out, 0, -lv_obj_get_width(lv_scr_act()));
//     lv_anim_set_duration(&anim_out, 100);
//     lv_anim_set_path_cb(&anim_out, lv_anim_path_ease_in_out);
//     lv_anim_set_deleted_cb(&anim_out, [](lv_anim_t *a) {
//         lv_obj_add_flag((lv_obj_t *)a->var, LV_OBJ_FLAG_HIDDEN);
//     });
//     lv_anim_start(&anim_out);

//     lv_obj_set_x(next_page, lv_obj_get_width(lv_scr_act()));
//     lv_obj_clear_flag(next_page, LV_OBJ_FLAG_HIDDEN);

//     lv_anim_init(&anim_in);
//     lv_anim_set_var(&anim_in, next_page);
//     lv_anim_set_exec_cb(&anim_in, anim_x_cb);
//     lv_anim_set_values(&anim_in, lv_obj_get_width(lv_scr_act()), 0);
//     lv_anim_set_duration(&anim_in, 100);
//     lv_anim_set_path_cb(&anim_in, lv_anim_path_ease_in_out);
//     lv_anim_start(&anim_in);
// }

static void animate_pages(lv_obj_t *current_page, lv_obj_t *next_page) {
    // Hide the current page
    lv_obj_add_flag(current_page, LV_OBJ_FLAG_HIDDEN);

    // Set the next page to be visible
    lv_obj_clear_flag(next_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_x(next_page, 0);  // Position it in the default view (no animation)
}


void setup() {
    Serial.begin(115200);
    Serial.printf("Hello Arduino! LVGL %d.%d.%d\n", lv_version_major(), lv_version_minor(), lv_version_patch());
    pinMode(potPin, INPUT);
    uint16_t calData[5] = {332, 3607, 385, 3365, 1};
    tft.init();
    tft.setRotation(3);
    tft.setTouch(calData);

    lv_init();

    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    lv_display_t *disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchscreen_read);

    ui_home();
    ui_plotter();
    Serial.println("Setup done");
}

void loop() {
    unsigned int tickPeriod = millis() - lastTickMillis;
    lv_tick_inc(tickPeriod);
    lastTickMillis = millis();
    lv_timer_handler();
    delay(5);

    if (millis() - LastTime >= 1000) {
        LastTime = millis();
        batt--;
        update_rectangle_width(batt);
    }
}

void ui_home() {
    static lv_style_t style_base;
    lv_style_init(&style_base);
    lv_style_set_border_width(&style_base, 0);

    screen = lv_obj_create(lv_screen_active());
    lv_obj_set_size(screen, TFT_HOR_RES, TFT_VER_RES);
    lv_obj_center(screen);
    lv_obj_add_style(screen, &style_base, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *user_label = lv_label_create(screen);
    lv_label_set_text(user_label, "Uroflow");
    lv_obj_set_style_text_color(user_label, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_font(user_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_align(user_label, LV_ALIGN_TOP_MID, 0, 20);

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 3);
    lv_style_set_bg_color(&style_btn, lv_palette_main(LV_PALETTE_GREEN));

    LV_IMAGE_DECLARE(bat0v2);
    batts = lv_image_create(screen);
    lv_image_set_src(batts, &bat0v2);
    lv_obj_align(batts, LV_ALIGN_BOTTOM_MID, 0, 0);

    scaling_rect = lv_obj_create(screen);
    lv_obj_set_size(scaling_rect, 100, 40);
    lv_obj_set_style_bg_color(scaling_rect, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_align_to(scaling_rect, batts, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_clear_flag(scaling_rect, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *btn = lv_button_create(screen);
    lv_obj_set_size(btn, 100, 50);
    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_center(btn);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Start");
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
}

void ui_plotter() {
  screen_plotter = lv_obj_create(lv_scr_act());
  lv_obj_set_size(screen_plotter, TFT_HOR_RES, TFT_VER_RES);
  lv_obj_clear_flag(screen_plotter, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(screen_plotter, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *user_label = lv_label_create(screen_plotter);
  lv_label_set_text(user_label, "Plotter Uroflow");
  lv_obj_set_style_text_color(user_label, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(user_label, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_align(user_label, LV_ALIGN_TOP_MID, 0, -10);
  

  chart1 = lv_chart_create(screen_plotter);
  lv_obj_set_size(chart1, 200, 150);
  lv_obj_align(chart1, LV_ALIGN_LEFT_MID, 0, 0);
  lv_chart_set_update_mode(chart1, LV_CHART_UPDATE_MODE_CIRCULAR);
  lv_obj_set_style_size(chart1, 0, 0, LV_PART_INDICATOR);
  lv_chart_set_point_count(chart1, 150);
   

  lv_obj_t *chart1_label = lv_label_create(screen_plotter);
  lv_label_set_text(chart1_label, "Flow Rate");
  lv_obj_set_style_text_color(chart1_label, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(chart1_label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align_to(chart1_label, chart1, LV_ALIGN_OUT_TOP_MID, 0, 0);

  chart2 = lv_chart_create(screen_plotter);
  lv_obj_set_size(chart2, 200, 150);
  lv_obj_align(chart2, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_chart_set_update_mode(chart2, LV_CHART_UPDATE_MODE_CIRCULAR);
  lv_obj_set_style_size(chart2, 0, 0, LV_PART_INDICATOR);
  lv_chart_set_point_count(chart2, 150);
   ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

  lv_obj_t *chart2_label = lv_label_create(screen_plotter);
  lv_label_set_text(chart2_label, "Volume Rate");
  lv_obj_set_style_text_color(chart2_label, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(chart2_label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_align_to(chart2_label, chart2, LV_ALIGN_OUT_TOP_MID, 0, 0);

  lv_timer_create(add_data, 300, chart1);
  lv_timer_create(add_data, 300, chart2);

  static lv_style_t style_btn;
  lv_style_init(&style_btn);
  lv_style_set_radius(&style_btn, 3);
  lv_style_set_bg_color(&style_btn, lv_palette_main(LV_PALETTE_GREEN));

  lv_obj_t * btn1 = lv_button_create(screen_plotter);
  lv_obj_set_size(btn1, 100, 50);
  lv_obj_add_style(btn1, &style_btn, 0);
  lv_obj_align(btn1, LV_ALIGN_BOTTOM_MID, -150, 0);

  lv_obj_t * label_btn1 = lv_label_create(btn1);
  lv_label_set_text(label_btn1, "Print");
  lv_obj_center(label_btn1);

  lv_obj_t * btn2 = lv_button_create(screen_plotter);
  lv_obj_set_size(btn2, 100, 50);
  lv_obj_add_style(btn2, &style_btn, 0);
  lv_obj_align(btn2, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_event_cb(btn2, event_cb_btn, LV_EVENT_CLICKED, NULL);

  lv_obj_t * label_btn2 = lv_label_create(btn2);
  lv_label_set_text(label_btn2, "Finish");
  lv_obj_center(label_btn2);
  
  lv_subject_init_int(&fw_update_status_subject, FW_UPDATE_STATE_IDLE);
  lv_subject_add_observer(&fw_update_status_subject, fw_upload_manager_observer_cb, NULL);

  lv_obj_t * btn3 = lv_button_create(screen_plotter);
  lv_obj_set_size(btn3, 100, 50);
  lv_obj_add_style(btn3, &style_btn, 0);
  lv_obj_align(btn3, LV_ALIGN_BOTTOM_MID, 150, 0);
  lv_obj_add_event_cb(btn3, event_cb_btn2, LV_EVENT_CLICKED, NULL);


  lv_obj_t * label_btn3 = lv_label_create(btn3);
  lv_label_set_text(label_btn3, "Flush");
  lv_obj_center(label_btn3);
}

static void add_data(lv_timer_t *timer) {
    lv_obj_t *chart = (lv_obj_t *)timer->user_data;
    lv_chart_series_t *ser = lv_chart_get_series_next(chart, NULL);
    int value = get_simulated_data();
    if (bPage_plotter) {
        lv_chart_set_next_value(chart, ser, value);
    }

}

static int get_simulated_data() {
    simulated_value = analogRead(potPin);
    // simulated_value = simulated_value*0,0244200244200244;
    Serial.println(simulated_value);
    return simulated_value;
}

static void event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // LV_UNUSED(e);
        lv_obj_t * win = (lv_obj_t *)lv_win_create(lv_screen_active());
        lv_obj_set_size(win, lv_pct(90), lv_pct(90));
        lv_obj_set_height(lv_win_get_header(win), 40);
        lv_obj_set_style_radius(win, 8, 0);
        lv_obj_set_style_shadow_width(win, 24, 0);
        lv_obj_set_style_shadow_offset_x(win, 2, 0);
        lv_obj_set_style_shadow_offset_y(win, 3, 0);
        lv_obj_set_style_shadow_color(win, lv_color_hex3(0x888), 0);
        lv_win_add_title(win, " ");
        lv_obj_t * btn_close = lv_win_add_button(win, LV_SYMBOL_CLOSE, 40);
        lv_obj_add_event_cb(btn_close, fw_update_close_event_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_center(win);

        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_START_CONNECTING);
        lv_subject_add_observer_obj(&fw_update_status_subject, fw_update_win_observer_cb, win, NULL);

        // animate_pages(screen, screen_plotter);
        // bPage_plotter = true;
    }
}

// static void event_cb(lv_event_t *e) {
//   if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
//     animate_pages(screen, screen_plotter);
//     bPage_plotter = true;
//   }
// }


static void event_cb_btn2(lv_event_t *e) {
    LV_UNUSED(e);
    lv_obj_t * win = (lv_obj_t *)lv_win_create(lv_screen_active());
    lv_obj_set_size(win, lv_pct(90), lv_pct(90));
    lv_obj_set_height(lv_win_get_header(win), 40);
    lv_obj_set_style_radius(win, 8, 0);
    lv_obj_set_style_shadow_width(win, 24, 0);
    lv_obj_set_style_shadow_offset_x(win, 2, 0);
    lv_obj_set_style_shadow_offset_y(win, 3, 0);
    lv_obj_set_style_shadow_color(win, lv_color_hex3(0x888), 0);
    lv_win_add_title(win, "Flush system");
    lv_obj_t * btn = lv_win_add_button(win, LV_SYMBOL_CLOSE, 40);
    lv_obj_add_event_cb(btn, fw_update_close_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_center(win);

    lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_IDLE);
    lv_subject_add_observer_obj(&fw_update_status_subject, fw_update_win_observer_cb, win, NULL);
}

static void fw_update_close_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_CANCEL);
}

// static void restart_btn_click_event_cb(lv_event_t * e)
// {
//     lv_obj_t * win = (lv_obj_t *)lv_event_get_user_data(e);
//     lv_obj_delete(win);
//     lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_IDLE);
// }

static void fw_update_win_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    lv_obj_t * win = (lv_obj_t *)lv_observer_get_target(observer);
    lv_obj_t * cont = lv_win_get_content(win);
    fw_update_state_t status = static_cast<fw_update_state_t>(lv_subject_get_int(&fw_update_status_subject));
    if(status == FW_UPDATE_STATE_IDLE) {
        lv_obj_clean(cont);
        lv_obj_t * spinner = lv_spinner_create(cont);
        lv_obj_center(spinner);
        lv_obj_set_size(spinner, 130, 130);

        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Flushing");
        lv_obj_center(label);

        lv_subject_set_int(subject, FW_UPDATE_STATE_CONNECTING);
    }
    else if(status == FW_UPDATE_STATE_START_CONNECTING) {
        lv_obj_clean(cont);
        lv_obj_t * spinner = lv_spinner_create(cont);
        lv_obj_center(spinner);
        lv_obj_set_size(spinner, 130, 130);

        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Connecting");
        ser1 = lv_chart_add_series(chart1, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
        ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
        lv_obj_center(label);

        lv_subject_set_int(subject, FW_UPDATE_STATE_S_CONNECTING);
    }
    else if(status == FW_UPDATE_STATE_READY) {
        lv_obj_clean(cont);
        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Finish Flushing");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

        // animate_pages(screen_plotter, screen);
        // bPage_plotter = false;
        // lv_obj_delete(win);
    }

    else if(status == FW_UPDATE_STATE_START_READY) {
        lv_obj_clean(cont);
        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Device Ready");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

        lv_subject_set_int(subject, FW_UPDATE_STATE_START_READY_D);

        // animate_pages(screen, screen_plotter);
        // bPage_plotter = true;
        // lv_obj_delete(win);
    }
    
    else if(status == FW_UPDATE_STATE_START_FINISH) {
        lv_obj_clean(cont);
        lv_obj_t * label = lv_label_create(cont);
        lv_label_set_text(label, "Device Ready");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
        
        animate_pages(screen, screen_plotter);
        bPage_plotter = true;
        lv_obj_delete(win);
    }

    else if(status == FW_UPDATE_STATE_CANCEL) {
        lv_obj_delete(win);
    }
}

static void connect_timer_cb(lv_timer_t * t)
{
    if(lv_subject_get_int(&fw_update_status_subject) != FW_UPDATE_STATE_CANCEL) {
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_READY);
    }
    lv_timer_delete(t);
}

static void connect_timer_cb2(lv_timer_t * t)
{
    if(lv_subject_get_int(&fw_update_status_subject) != FW_UPDATE_STATE_CANCEL) {
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_START_FINISH);
    }
    lv_timer_delete(t);
}

static void connect_timer_s_connected2(lv_timer_t * t)
{
    if(lv_subject_get_int(&fw_update_status_subject) != FW_UPDATE_STATE_CANCEL) {
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_START_READY);
    }
    lv_timer_delete(t);
}

static void download_timer_cb(lv_timer_t * t)
{
    if(lv_subject_get_int(&fw_update_status_subject) == FW_UPDATE_STATE_CANCEL) {
        lv_timer_delete(t);
        return;
    }

    int32_t v = lv_subject_get_int(&fw_download_percent_subject);
    if(v < 100) {
        lv_subject_set_int(&fw_download_percent_subject, v + 1);
    }
    else {
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_READY);
        lv_timer_delete(t);
    }
}

static void fw_upload_manager_observer_cb(lv_observer_t * observer, lv_subject_t * subject)
{
    LV_UNUSED(subject);
    LV_UNUSED(observer);

    fw_update_state_t state = static_cast<fw_update_state_t>(lv_subject_get_int(&fw_update_status_subject));
    if(state == FW_UPDATE_STATE_CONNECTING) {
        lv_timer_create(connect_timer_cb, 5000, NULL);
    }
    else if(state == FW_UPDATE_STATE_CONNECTED) {
        lv_subject_set_int(&fw_download_percent_subject, 0);
        lv_subject_set_int(&fw_update_status_subject, FW_UPDATE_STATE_DOWNLOADING);
        lv_timer_create(download_timer_cb, 50, NULL);
    }
    else if(state == FW_UPDATE_STATE_S_CONNECTING) {
        lv_timer_create(connect_timer_s_connected2, 5000, NULL);
    }
    else if(state == FW_UPDATE_STATE_START_READY_D) {
        lv_timer_create(connect_timer_cb2, 2000, NULL);
    }
}

static void event_cb_btn(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    // Reset series in chart1
    reset_chart_series(chart1); // Remove all existing series
    ser1 = lv_chart_add_series(chart1, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y); // Re-add series
    
    // Reset series in chart2
    reset_chart_series(chart2); // Remove all existing series
    ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y); // Re-add series

    // Optionally log reset
    Serial.println("Charts reset.");

    // Handle any other logic like page animations or status changes
    animate_pages(screen_plotter, screen); // Example animation back to home screen
    bPage_plotter = false;
  }
}

// Function to reset the chart series
static void reset_chart_series(lv_obj_t *chart) {
    lv_chart_series_t *ser = lv_chart_get_series_next(chart, NULL); // Get the first series
    while (ser) {
        lv_chart_remove_series(chart, ser); // Remove the current series
        ser = lv_chart_get_series_next(chart, NULL); // Get the next series
    }
}
