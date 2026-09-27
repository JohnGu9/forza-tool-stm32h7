#ifndef WIDGETS_H
#define WIDGETS_H

#include "mf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const mWidgetClass_t MainWidgetClass;

extern const mWidgetClass_t LinkDownWidgetClass;

extern const mWidgetClass_t LinkUpWidgetClass;

extern const mWidgetClass_t UdpServerWidgetClass;

extern const mWidgetClass_t TachometerWidgetClass;

extern const mWidgetClass_t TachometerLedControllerWidgetClass;

extern const mWidgetClass_t ControlInfoWidgetClass;

extern const mWidgetClass_t GForceWidgetClass;

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

#include "animated_transition_widget.h"

typedef struct {
  lv_obj_t *div;
  mContext_t *context;
  CircularBuffer_t *circular_buffer;
  DataAnalysis_t *data_analysis;
  DataRecvTask_t recv_task;
  EventTaskSet_t data_recv_listeners;
  EventTask_t k1_listener;
  EventTask_t k2_listener;
  AnimatedTransitionWidgetData_t animated_transition_widget_data;
  uint16_t refresh_key;
  uint16_t child_type_key;
  bool is_dirty;
} LinkUpWidgetState_t;

#ifdef __cplusplus
}
#endif

#endif
