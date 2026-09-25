#ifndef WIDGETS_H
#define WIDGETS_H

#include "mf.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIDGET_MEMORY_LOCATION

extern const mWidget_t MainWidget;

extern const mWidget_t LinkDownWidget;

extern const mWidget_t LinkUpWidget;

extern const mWidget_t UdpServerWidget;

extern const mWidget_t TachometerWidget;

extern const mWidget_t TachometerLedControllerWidget;

#include "common.h"

#include <lvgl.h>

typedef struct {
  lv_obj_t *screen;
  mContext_t *context;
  bool is_link_state_up;
  EventTask_t link_state_change_task;
  lv_anim_timeline_t *anim_timeline;
} MainWidgetState_t;

#include <lwip/udp.h>

typedef struct {
  void *context;
  void (*callback)(void *context, struct pbuf *p);
} DataRecvTask_t;

typedef struct {
  uint8_t buffer[332];
  size_t length;
} DataPacket_t;

typedef struct {
  DataPacket_t data_packets[256];
  uint16_t start;
  uint16_t length;
} CircularBuffer_t;

DataPacket_t *circular_buffer_push(CircularBuffer_t *buffer);

// @Return: nullable
const DataPacket_t *circular_buffer_get(const CircularBuffer_t *buffer,
                                        size_t index);

// @Return: nullable
const DataPacket_t *circular_buffer_get_last(const CircularBuffer_t *buffer);

void circular_buffer_clear(CircularBuffer_t *buffer);

typedef struct {
  float Power;
  float Torque;
  float CurrentEngineRpm;
} DataAnalysisMetaData_t;

typedef struct {
  const DataAnalysisMetaData_t *below_90_lower;
  const DataAnalysisMetaData_t *below_97_lower;
  const DataAnalysisMetaData_t *below_90_upper;
  const DataAnalysisMetaData_t *below_97_upper;
} PowerRange_t;

typedef struct {
  DataAnalysisMetaData_t max_power;
  DataAnalysisMetaData_t power_curve[1024];
  PowerRange_t power_range;
} DataAnalysis_t;

void data_analysis_clear(DataAnalysis_t *data_analysis);

typedef struct {
  lv_obj_t *div;
  mContext_t *context;
  CircularBuffer_t *circular_buffer;
  DataAnalysis_t *data_analysis;
  DataRecvTask_t recv_task;
  bool is_dirty;
} LinkUpWidgetState_t;

typedef struct {
  LinkUpWidgetState_t *link_up_widget_state;
  lv_obj_t *div; // root

  lv_obj_t *scale;
  lv_scale_section_t *sections[5];
  lv_obj_t *needle_line;
  lv_obj_t *hr_value_label;
  lv_obj_t *bpm_label;
  lv_obj_t *power_level_label;

  lv_obj_t *led_1;
  lv_obj_t *led_2;
  lv_obj_t *led_3;
  lv_obj_t *led_4;
  lv_obj_t *led_5;
  lv_timer_t *led_blink_timer;
  bool led_on;
} TachometerWidgetState_t;

#ifdef __cplusplus
}
#endif

#endif
