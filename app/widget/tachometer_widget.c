#include "widgets.h"

#include "mf.h"

#include <assert.h>
#include <lvgl.h>
#include <stdlib.h>

static TachometerWidgetState_t state = {0};

typedef struct {
  lv_style_t items;
  lv_style_t indicator;
  lv_style_t main;
} section_styles_t;

static section_styles_t zone1_styles;
static section_styles_t zone2_styles;
static section_styles_t zone3_styles;
static section_styles_t zone4_styles;
static section_styles_t zone5_styles;

static lv_color_t get_hr_zone_color(int32_t hr, int32_t range1, int32_t range2,
                                    int32_t range3, int32_t range4) {
  if (hr < range1)
    return lv_palette_main(LV_PALETTE_GREY); /* Zone 1 */
  else if (hr < range2)
    return lv_palette_main(LV_PALETTE_BLUE); /* Zone 2 */
  else if (hr < range3)
    return lv_palette_main(LV_PALETTE_GREEN); /* Zone 3 */
  else if (hr < range4)
    return lv_palette_main(LV_PALETTE_BLUE); /* Zone 4 */
  else
    return lv_palette_main(LV_PALETTE_ORANGE); /* Zone 5 */
}

static void init_section_styles(section_styles_t *styles, lv_color_t color) {
  lv_style_init(&styles->items);
  lv_style_set_line_color(&styles->items, color);
  lv_style_set_line_width(&styles->items, 0);

  lv_style_init(&styles->indicator);
  lv_style_set_line_color(&styles->indicator, color);
  lv_style_set_line_width(&styles->indicator, 0);

  lv_style_init(&styles->main);
  lv_style_set_arc_color(&styles->main, color);
  lv_style_set_arc_width(&styles->main, 12);
}

static lv_scale_section_t *add_section(lv_obj_t *target_scale, int32_t from,
                                       int32_t to,
                                       const section_styles_t *styles) {
  lv_scale_section_t *sec = lv_scale_add_section(target_scale);
  lv_scale_set_section_range(target_scale, sec, from, to);
  lv_scale_set_section_style_items(target_scale, sec, &styles->items);
  lv_scale_set_section_style_indicator(target_scale, sec, &styles->indicator);
  lv_scale_set_section_style_main(target_scale, sec, &styles->main);
  return sec;
}

#define IS_VALUE_IN_RANGE(x, min, max) ((x) >= (min)) && ((x) <= (max))

static void get_section_range(const SledData_t *const sled,
                              const DataAnalysis_t *data_analysis,
                              int32_t *range1, int32_t *range2, int32_t *range3,
                              int32_t *range4) {
  const DataAnalysisMetaData_t *const below_90_lower =
      data_analysis->power_range.below_90_lower;
  const DataAnalysisMetaData_t *const below_97_lower =
      data_analysis->power_range.below_97_lower;
  const DataAnalysisMetaData_t *const below_90_upper =
      data_analysis->power_range.below_90_upper;
  const DataAnalysisMetaData_t *const below_97_upper =
      data_analysis->power_range.below_97_upper;
  *range1 = below_90_lower == NULL
                ? 0
                : below_90_lower->CurrentEngineRpm / sled->EngineMaxRpm * 100;
  *range2 = below_97_lower == NULL
                ? 0
                : below_97_lower->CurrentEngineRpm / sled->EngineMaxRpm * 100;

  *range3 = below_97_upper == NULL
                ? 100
                : below_97_upper->CurrentEngineRpm / sled->EngineMaxRpm * 100;
  *range4 = below_90_upper == NULL
                ? 100
                : below_90_upper->CurrentEngineRpm / sled->EngineMaxRpm * 100;
}

static void set_data(TachometerWidgetState_t *state,
                     const DataPacket_t *data_packet_nullable, // nullable
                     const DataAnalysis_t *data_analysis) {
  if (data_packet_nullable == NULL) {
    return;
  }
  const DataPacket_t *const data_packet = data_packet_nullable;
  const SledData_t *const sled =
      get_sled_data(data_packet->buffer, data_packet->length);
  const DashData_t *const dash =
      get_dash_data(data_packet->buffer, data_packet->length);
  if (sled == NULL) {
    return;
  }
  float percent = sled->EngineMaxRpm == 0
                      ? 0
                      : (sled->CurrentEngineRpm / sled->EngineMaxRpm);

  int32_t hr_value = (int32_t)(percent * 100);

  /* Update needle */
  lv_scale_set_line_needle_value(state->scale, state->needle_line, 50,
                                 hr_value);

  /* Update HR text */
  if (dash != NULL) {
    if (dash->Gear == 0) {
      lv_label_set_text(state->hr_value_label, "R");
    } else {
      lv_label_set_text_fmt(state->hr_value_label, "%d", dash->Gear);
    }
    int32_t range1 = 0, range2 = 0, range3 = 0, range4 = 0;
    get_section_range(sled, data_analysis, &range1, &range2, &range3, &range4);
    lv_scale_set_section_range(state->scale, state->sections[0], 0, range1);
    lv_scale_set_section_range(state->scale, state->sections[1], range1,
                               range2);
    lv_scale_set_section_range(state->scale, state->sections[2], range2,
                               range3);
    lv_scale_set_section_range(state->scale, state->sections[3], range3,
                               range4);
    lv_scale_set_section_range(state->scale, state->sections[4], range4, 100);

    /* Update text color based on zone */
    lv_color_t zone_color =
        get_hr_zone_color(hr_value, range1, range2, range3, range4);
    lv_obj_set_style_text_color(state->hr_value_label, zone_color, 0);
    lv_obj_set_style_text_color(state->bpm_label, zone_color, 0);
    lv_obj_set_style_text_color(state->power_level_label, zone_color, 0);

    lv_label_set_text_fmt(
        state->power_level_label, "%ld%%",
        data_analysis->max_power.Power == 0
            ? 0
            : (int32_t)(dash->Power / data_analysis->max_power.Power * 100));
  }
  lv_label_set_text_fmt(state->bpm_label, "%ld",
                        (int32_t)sled->CurrentEngineRpm);
}

static void lv_tachometer(lv_obj_t *parent, TachometerWidgetState_t *state) {
  lv_obj_t *const div = lv_obj_create(parent);
  state->div = div;
  lv_obj_set_size(div, 128, 128);
  lv_obj_set_scrollable(div, false);
  lv_obj_t *scale = lv_scale_create(div);
  state->scale = scale;
  lv_obj_set_size(scale, 108, 108);
  lv_obj_center(scale);

  lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
  lv_scale_set_range(scale, 0, 100);
  lv_scale_set_total_tick_count(scale, 15);
  lv_scale_set_major_tick_every(scale, 3);
  lv_scale_set_angle_range(scale, 280);
  lv_scale_set_rotation(scale, 130);
  lv_scale_set_label_show(scale, false);

  lv_obj_set_style_length(scale, 6, LV_PART_ITEMS);
  lv_obj_set_style_length(scale, 10, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(scale, 0, LV_PART_MAIN);

  /* Zone 1: (Grey) */
  init_section_styles(&zone1_styles, lv_palette_main(LV_PALETTE_GREY));
  state->sections[0] = add_section(scale, 0, 20, &zone1_styles);

  /* Zone 2: (Blue) */
  init_section_styles(&zone2_styles, lv_palette_main(LV_PALETTE_BLUE));
  state->sections[1] = add_section(scale, 20, 40, &zone2_styles);

  /* Zone 3: (Green) */
  init_section_styles(&zone3_styles, lv_palette_main(LV_PALETTE_GREEN));
  state->sections[2] = add_section(scale, 40, 60, &zone3_styles);

  /* Zone 4: (Orange) */
  init_section_styles(&zone4_styles, lv_palette_main(LV_PALETTE_BLUE));
  state->sections[3] = add_section(scale, 60, 80, &zone4_styles);

  /* Zone 5: (Red) */
  init_section_styles(&zone5_styles, lv_palette_main(LV_PALETTE_ORANGE));
  state->sections[4] = add_section(scale, 80, 100, &zone5_styles);

  lv_obj_t *needle_line = lv_line_create(scale);
  state->needle_line = needle_line;

  /* Optional styling */
  lv_obj_set_style_line_color(needle_line,
                              lv_obj_get_style_line_color(div, LV_PART_MAIN),
                              LV_PART_MAIN);
  lv_obj_set_style_line_width(needle_line, 4, LV_PART_MAIN);
  lv_obj_set_style_length(needle_line, 20, LV_PART_MAIN);
  lv_obj_set_style_line_rounded(needle_line, true, LV_PART_MAIN);
  lv_obj_set_style_pad_right(needle_line, 50, LV_PART_MAIN);

  lv_scale_set_line_needle_value(scale, needle_line, 50, 0);

  lv_obj_t *circle = lv_obj_create(div);
  lv_obj_set_scrollable(circle, false);
  lv_obj_set_size(circle, 70, 70);
  lv_obj_center(circle);

  lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);

  lv_obj_set_style_bg_color(circle,
                            lv_obj_get_style_bg_color(div, LV_PART_MAIN), 0);
  lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(circle, 0, LV_PART_MAIN);

  lv_obj_t *hr_container = lv_obj_create(circle);
  lv_obj_center(hr_container);
  lv_obj_set_size(hr_container, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(hr_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(hr_container, 0, 0);
  lv_obj_set_layout(hr_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(hr_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(hr_container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(hr_container, 0, 0);
  lv_obj_set_flex_align(hr_container, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *hr_value_label = lv_label_create(hr_container);
  state->hr_value_label = hr_value_label;
  lv_label_set_text(hr_value_label, "R");
  lv_obj_set_style_text_font(hr_value_label, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_align(hr_value_label, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *bpm_label = lv_label_create(hr_container);
  state->bpm_label = bpm_label;
  lv_label_set_text(bpm_label, "Rpm");
  lv_obj_set_style_text_font(bpm_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_align(bpm_label, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *power_level_label = lv_label_create(hr_container);
  state->power_level_label = power_level_label;
  lv_label_set_text(power_level_label, "0\%");
  lv_obj_set_style_text_font(power_level_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_align(power_level_label, LV_TEXT_ALIGN_CENTER, 0);

  lv_color_t zone_color = get_hr_zone_color(0, 20, 40, 60, 80);
  lv_obj_set_style_text_color(hr_value_label, zone_color, 0);
  lv_obj_set_style_text_color(bpm_label, zone_color, 0);
  lv_obj_set_style_text_color(power_level_label, zone_color, 0);
}

static void lv_led(lv_obj_t *parent, TachometerWidgetState_t *state) {
  lv_obj_t *div = lv_obj_create(parent);
  lv_obj_set_height(div, 36);
  lv_obj_set_flex_flow(div, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(div, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scrollable(div, false);

  lv_obj_t *led_1 = lv_led_create(div);
  state->led_1 = led_1;
  lv_obj_set_size(led_1, 10, 10);
  lv_led_set_color(led_1, lv_palette_main(LV_PALETTE_BLUE));
  lv_led_set_brightness(led_1, 0);

  lv_obj_t *led_2 = lv_led_create(div);
  state->led_2 = led_2;
  lv_obj_set_size(led_2, 10, 10);
  lv_led_set_color(led_2, lv_palette_main(LV_PALETTE_BLUE));
  lv_led_set_brightness(led_2, 0);

  lv_obj_t *led_3 = lv_led_create(div);
  state->led_3 = led_3;
  lv_obj_set_size(led_3, 10, 10);
  lv_led_set_color(led_3, lv_palette_main(LV_PALETTE_GREEN));
  lv_led_set_brightness(led_3, 0);

  lv_obj_t *led_4 = lv_led_create(div);
  state->led_4 = led_4;
  lv_obj_set_size(led_4, 10, 10);
  lv_led_set_color(led_4, lv_palette_main(LV_PALETTE_BLUE));
  lv_led_set_brightness(led_4, 0);

  lv_obj_t *led_5 = lv_led_create(div);
  state->led_5 = led_5;
  lv_obj_set_size(led_5, 10, 10);
  lv_led_set_color(led_5, lv_palette_main(LV_PALETTE_BLUE));
  lv_led_set_brightness(led_5, 0);
}

static void *init_state(mContext_t *context) {
  LinkUpWidgetState_t *const link_up_widget_state =
      (LinkUpWidgetState_t *)m_get_widget_data(context);
  lv_led(link_up_widget_state->div, &state);
  lv_tachometer(link_up_widget_state->div, &state);
  const DataPacket_t *data_packet =
      circular_buffer_get_last(link_up_widget_state->circular_buffer);
  set_data(&state, data_packet, link_up_widget_state->data_analysis);
  state.link_up_widget_state = link_up_widget_state;
  return &state;
}

static void build(mContext_t *context, void *_state,
                  const mWidget_t *children[MF_MAX_CHILDREN],
                  const void *children_widget_data[MF_MAX_CHILDREN]) {
  TachometerWidgetState_t *state = (TachometerWidgetState_t *)_state;
  LinkUpWidgetState_t *const link_up_widget_state =
      (LinkUpWidgetState_t *)m_get_widget_data(context);
  const DataPacket_t *data_packet =
      circular_buffer_get_last(link_up_widget_state->circular_buffer);
  set_data(state, data_packet, link_up_widget_state->data_analysis);

  children[0] = &TachometerLedControllerWidget;
  children_widget_data[0] = state;
  return;
}

static void dispose(mContext_t *context, void *_state) {
  TachometerWidgetState_t *state = (TachometerWidgetState_t *)_state;
  lv_obj_del(state->div);
  state->div = NULL;
  state->link_up_widget_state = NULL;
  return;
};

const mWidget_t TachometerWidget = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
} WIDGET_MEMORY_LOCATION;
