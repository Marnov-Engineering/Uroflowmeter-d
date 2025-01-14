#include <TFT_eSPI.h>
#include <lvgl.h>
#include <examples/lv_examples.h>

/*Set to your screen resolution*/
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
int batt = 95;

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

unsigned long lastTickMillis = 0;

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
}

void ui_home(){
  static lv_style_t style_base;
  lv_style_init(&style_base);
  lv_style_set_border_width(&style_base, 0);

  lv_obj_t *screen = lv_obj_create(lv_screen_active());
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
  lv_obj_t * batts = lv_image_create(screen);
  lv_image_set_src(batts, &bat0v2);
  lv_obj_align(batts, LV_ALIGN_BOTTOM_MID, 0, 0);

  lv_obj_t *batt_text = lv_label_create(screen);
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%d %", batt);
  const char* str = buffer;
  lv_label_set_text(batt_text, buffer);
  lv_obj_set_style_text_color(batt_text, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(batt_text, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_align_to(batt_text, batts, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
  

  // button
  lv_obj_t * btn = lv_button_create(screen);
  lv_obj_set_size(btn, 100, 50);
  lv_obj_add_style(btn, &style_btn, 0);
  lv_obj_center(btn);
  

  lv_obj_t * label = lv_label_create(btn);
  lv_label_set_text(label, "Start");
  lv_obj_center(label);
  lv_obj_t * pik = lv_label_create(lv_screen_active());
  lv_label_set_text(pik, "100");
  lv_obj_align_to(pik, batts, LV_ALIGN_OUT_TOP_MID, 0, -5);

  lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, pik);




  // if(batt > 85 && batt <= 100){
  //   LV_IMAGE_DECLARE(Kondisi_100);
  //   lv_obj_t * batts = lv_image_create(screen);
  //   lv_image_set_src(batts, &Kondisi_100);
  //   lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  // }
  // if(batt > 70 && batt <= 85){
  //   LV_IMAGE_DECLARE(Kondisi_85);
  //   lv_obj_t * batts = lv_image_create(screen);
  //   lv_image_set_src(batts, &Kondisi_85);
  //   lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  // }
  // if(batt > 55 && batt <= 70){
  //   LV_IMAGE_DECLARE(Kondisi_70);
  //   lv_obj_t * batts = lv_image_create(screen);
  //   lv_image_set_src(batts, &Kondisi_70);
  //   lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  // }
  // if(batt > 40 && batt <= 55){
  //   LV_IMAGE_DECLARE(Kondisi_55);
  //   lv_obj_t * batts = lv_image_create(screen);
  //   lv_image_set_src(batts, &Kondisi_55);
  //   lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  // }
  // if(batt > 25 && batt <= 40){
  //   LV_IMAGE_DECLARE(Kondisi_40);
  //   lv_obj_t * batts = lv_image_create(screen);
  //   lv_image_set_src(batts, &Kondisi_40);
  //   lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  // }
  // if(batt > 0 && batt <= 25){
  //   LV_IMAGE_DECLARE(Kondisi_25);
  //   lv_obj_t * batts = lv_image_create(screen);
  //   lv_image_set_src(batts, &Kondisi_25);
  //   lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  // }


  

}

void ui_batt(){
  lv_obj_t *screen = lv_obj_create(lv_screen_active());
  lv_obj_set_size(screen, TFT_HOR_RES, TFT_VER_RES);
  if(batt > 85 && batt <= 100){
    LV_IMAGE_DECLARE(Kondisi_100);
    lv_obj_t * batts = lv_image_create(screen);
    lv_image_set_src(batts, &Kondisi_100);
    lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  }
  if(batt > 70 && batt <= 85){
    LV_IMAGE_DECLARE(Kondisi_85);
    lv_obj_t * batts = lv_image_create(screen);
    lv_image_set_src(batts, &Kondisi_85);
    lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  }
  if(batt > 55 && batt <= 70){
    LV_IMAGE_DECLARE(Kondisi_70);
    lv_obj_t * batts = lv_image_create(screen);
    lv_image_set_src(batts, &Kondisi_70);
    lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  }
  if(batt > 40 && batt <= 55){
    LV_IMAGE_DECLARE(Kondisi_55);
    lv_obj_t * batts = lv_image_create(screen);
    lv_image_set_src(batts, &Kondisi_55);
    lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  }
  if(batt > 25 && batt <= 40){
    LV_IMAGE_DECLARE(Kondisi_40);
    lv_obj_t * batts = lv_image_create(screen);
    lv_image_set_src(batts, &Kondisi_40);
    lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  }
  if(batt > 0 && batt <= 25){
    LV_IMAGE_DECLARE(Kondisi_25);
    lv_obj_t * batts = lv_image_create(screen);
    lv_image_set_src(batts, &Kondisi_25);
    lv_obj_align(batts, LV_ALIGN_LEFT_MID, 0, 0);
  }
}

static void event_cb(lv_event_t *e)
{
    LV_LOG_USER("Clicked");

    static uint32_t cnt = 1;
    lv_obj_t * btn = (lv_obj_t*) lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    lv_obj_t * pik = (lv_obj_t*) lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_label_set_text_fmt(label, "%"LV_PRIu32, cnt);
    cnt++;
    batt--;
    char buffer[20];
  snprintf(buffer, sizeof(buffer), "%d %", batt);
  const char* str = buffer;
  lv_label_set_text(pik, buffer);
  lv_obj_set_style_text_color(pik, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_font(pik, &lv_font_montserrat_24, LV_PART_MAIN);
  // lv_obj_align_to(pik, batts, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    
}
