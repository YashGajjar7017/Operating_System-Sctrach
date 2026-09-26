/**
 * @file audiosrv.h
 */
#ifndef _SERVICES_AUDIOSRV_H_
#define _SERVICES_AUDIOSRV_H_
#include <stdint.h>
typedef struct AudioSession AudioSession;
AudioSession *audiosrv_open_session(const char *app_tag);
void          audiosrv_close_session(AudioSession *s);
void          audiosrv_set_master_volume(uint32_t vol);
void          audiosrv_set_master_mute(uint8_t muted);
uint32_t      audiosrv_get_master_volume(void);
void          audiosrv_service_thread(void);
#endif
