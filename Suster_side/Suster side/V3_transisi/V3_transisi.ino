#include <TFT_eSPI.h>
#include <lvgl.h>
// #include <examples/lv_examples.h>

/*Set to your screen resolution*/
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
int batt = 100;
int uro = 0;
bool bPage_plotter = false;
uint32_t LastTime;
static int simulated_value = 10;

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

unsigned long lastTickMillis = 0;
lv_obj_t *scaling_rect;
lv_obj_t * batts;

lv_obj_t *screen;
lv_obj_t *screen_plotter;

void update_rectangle_width(int batt) {
    // Constrain the input to the range [0, 100]
    if (batt < 0) batt = 0;
    if (batt > 100) batt = 100;

    // Map the input range [0, 100] to the rectangle width [0, 200]
    int new_width = map(batt, 0, 100, 0, 100);

    // Set the new width while keeping height constant (80 px)
    lv_obj_set_size(scaling_rect, new_width, 40);
    // lv_obj_align_to(scaling_rect, batts, LV_ALIGN_LEFT_MID, 6, 0);
    // lv_obj_t *batt_text = lv_label_create(screen);
    // char buffer[20];
    // snprintf(buffer, sizeof(buffer), "%d %", batt);
    // const char* str = buffer;
    // lv_label_set_text(batt_text, buffer);
    // lv_obj_set_style_text_color(batt_text, lv_color_hex(0x000000), LV_PART_MAIN);
    // lv_obj_set_style_text_font(batt_text, &lv_font_montserrat_24, LV_PART_MAIN);
    // lv_obj_align_to(batt_text, batts, LV_ALIGN_OUT_TOP_MID, 0, -10);
    

}

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  uint16_t x, y;
  bool touched = tft.getTouch(&x, &y);

  if (touched) {
    // Map the raw touch values to the screen coordinates
    Serial.print("x : ");
    Serial.println(x);
    Serial.print("y : ");
    Serial.println(y);
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = map(x, 0, 480, 0, TFT_HOR_RES);  // Adjust for your screen's dimensions
    data->point.y = map(y, 0, 320, 0, TFT_VER_RES);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void *draw_buf;

static void anim_x_cb(void *var, int32_t v) {
    lv_obj_set_x((lv_obj_t *)var, v);
}

static void animate_pages(lv_obj_t *current_page, lv_obj_t *next_page) {
    lv_anim_t anim_out, anim_in;

    // Animate the current page out (to the left)
    lv_anim_init(&anim_out);
    lv_anim_set_var(&anim_out, current_page);
    lv_anim_set_exec_cb(&anim_out, anim_x_cb);
    lv_anim_set_values(&anim_out, 0, -lv_obj_get_width(lv_scr_act())); // Slide left
    lv_anim_set_duration(&anim_out, 300);
    lv_anim_set_path_cb(&anim_out, lv_anim_path_ease_in_out);
    lv_anim_set_deleted_cb(&anim_out, [](lv_anim_t *a) {
        lv_obj_add_flag((lv_obj_t *)a->var, LV_OBJ_FLAG_HIDDEN);
    });
    lv_anim_start(&anim_out);

    // Prepare the next page off-screen (to the right)
    lv_obj_set_x(next_page, lv_obj_get_width(lv_scr_act())); // Position off-screen
    lv_obj_clear_flag(next_page, LV_OBJ_FLAG_HIDDEN); // Make it visible

    // Animate the next page in (from the right)
    lv_anim_init(&anim_in);
    lv_anim_set_var(&anim_in, next_page);
    lv_anim_set_exec_cb(&anim_in, anim_x_cb);
    lv_anim_set_values(&anim_in, lv_obj_get_width(lv_scr_act()), 0); // Slide in
    lv_anim_set_duration(&anim_in, 300);
    lv_anim_set_path_cb(&anim_in, lv_anim_path_ease_in_out);
    lv_anim_start(&anim_in);
}


void setup()
{
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.begin( 115200 );
    Serial.println( LVGL_Arduino );

    uint16_t calData[5] = { 332, 3607, 385, 3365, 1 };
    tft.init();
    tft.setRotation(3); // Set the display orientation (adjust as needed)
    tft.setTouch(calData);

    lv_init();
    

    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    lv_display_t * disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);

    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    lv_indev_set_read_cb(indev, touchscreen_read);

    // lv_obj_t *label = lv_label_create( lv_scr_act() );
    // lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
    // lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

    Serial.println( "Setup done" );
    ui_home();
    ui_plotter(); // Initialize plotter UI
    
}

void loop()
{
  unsigned int tickPeriod = millis() - lastTickMillis;
  lv_tick_inc(tickPeriod);
  lastTickMillis = millis();
  lv_timer_handler(); /* let the UI do its work */
  // ui_batt();
  // ui_home();
  delay( 5 );
    if (millis() - LastTime >= 1000) {
    LastTime = millis();
    batt--;
    update_rectangle_width(batt);
  }
  
}

void ui_home(){
  static lv_style_t style_base;
  lv_style_init(&style_base);
  lv_style_set_border_width(&style_base, 0);

  screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(screen, TFT_HOR_RES, TFT_VER_RES);
  lv_obj_center(screen);
  lv_obj_add_style(screen, &style_base, LV_PART_MAIN);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *user_label = lv_label_create(screen);
  // char buffer[20];
  // snprintf(buffer, sizeof(buffer), "%d", batt);
  // const char* str = buffer;
  lv_label_set_text(user_label, "Uroflow");
  lv_obj_set_style_text_color(user_label, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(user_label, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_align(user_label, LV_ALIGN_TOP_MID, 0, 20);

  //stlye button home
  static lv_style_t style_btn;
  lv_style_init(&style_btn);
  lv_style_set_radius(&style_btn, 3);
  lv_style_set_bg_color(&style_btn, lv_palette_main(LV_PALETTE_GREEN));

    //baterai

  

  LV_IMAGE_DECLARE(bat0v2);
  batts = lv_image_create(screen);
  lv_image_set_src(batts, &bat0v2);
  lv_obj_align(batts, LV_ALIGN_BOTTOM_MID, 0, 0);

  // lv_obj_t *batt_text = lv_label_create(screen);
  // char buffer[20];
  // snprintf(buffer, sizeof(buffer), "%d %", batt);
  // const char* str = buffer;
  // lv_label_set_text(batt_text, buffer);
  // lv_obj_set_style_text_color(batt_text, lv_color_hex(0x000000), LV_PART_MAIN);
  // lv_obj_set_style_text_font(batt_text, &lv_font_montserrat_24, LV_PART_MAIN);
  // lv_obj_align_to(batt_text, batts, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

  scaling_rect = lv_obj_create(screen); // Create a rectangle
  lv_obj_set_size(scaling_rect, 100, 40); // Initial size: 200x80 px
  lv_obj_set_style_bg_color(scaling_rect, lv_color_hex(0x000000), LV_PART_MAIN); // Set background color
  lv_obj_align_to(scaling_rect, batts, LV_ALIGN_LEFT_MID, 6, 0);
  lv_obj_clear_flag(scaling_rect, LV_OBJ_FLAG_SCROLLABLE);
  

  // button
  lv_obj_t * btn = lv_button_create(screen);
  lv_obj_set_size(btn, 100, 50);
  lv_obj_add_style(btn, &style_btn, 0);
  lv_obj_center(btn);
  

  lv_obj_t * label = lv_label_create(btn);
  lv_label_set_text(label, "Start");
  lv_obj_center(label);
  // lv_obj_t * pik = lv_label_create(lv_screen_active());
  // lv_label_set_text(pik, "100");
  // lv_obj_align_to(pik, batts, LV_ALIGN_OUT_TOP_MID, 0, -10);

  lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

}

// Create and configure the chart
void ui_plotter() {
    // Create the chart object
    screen_plotter = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen_plotter, TFT_HOR_RES, TFT_VER_RES);
    lv_obj_clear_flag(screen_plotter, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(screen_plotter, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    lv_obj_t *user_label = lv_label_create(screen_plotter);

  lv_label_set_text(user_label, "Plotter Uroflow");
  lv_obj_set_style_text_color(user_label, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(user_label, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_align(user_label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t * chart = lv_chart_create(screen_plotter);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR); // Circular data update
    // lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_size(chart, 200, 150);
    lv_obj_align(chart, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t * chart2 = lv_chart_create(screen_plotter);
    lv_chart_set_update_mode(chart2, LV_CHART_UPDATE_MODE_CIRCULAR); // Circular data update
    lv_obj_set_style_size(chart2, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_size(chart2, 200, 150);
    lv_obj_align(chart2, LV_ALIGN_RIGHT_MID, 0, 0);

    // Set up the chart with a single series
    lv_chart_set_point_count(chart, 150);
    lv_chart_series_t * ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    // Prefill the chart with initial data
      // for (uint32_t i = 0; i < 80; i++) {
      //   uro++;
      //   lv_chart_set_next_value(chart, ser, uro); // Random initial values
      //   // delay(100);
      // }
    lv_timer_create(add_data, 300, chart);

    lv_chart_set_point_count(chart2, 150);
    lv_chart_series_t * ser2 = lv_chart_add_series(chart2, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    // Prefill the chart with initial data
      // for (uint32_t i = 0; i < 80; i++) {
      //   uro++;
      //   lv_chart_set_next_value(chart, ser, uro); // Random initial values
      //   // delay(100);
      // }
    lv_timer_create(add_data, 300, chart2);
}

static void add_data(lv_timer_t * timer) {
    lv_obj_t * chart = (lv_obj_t *) timer->user_data;
    lv_chart_series_t * ser = lv_chart_get_series_next(chart, NULL);

    // Push new data into the chart
    int value = get_simulated_data(); // Replace with actual data source
    if(bPage_plotter){
      lv_chart_set_next_value(chart, ser, value);
    }
    // lv_chart_set_next_value(chart, ser, value);
}


static int get_simulated_data() {
    simulated_value = (simulated_value + 15) % 100; // Example dynamic data
    return simulated_value;
}


static void event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        animate_pages(screen, screen_plotter);
        bPage_plotter = true;
    }

}
