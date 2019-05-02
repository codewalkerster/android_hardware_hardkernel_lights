/* Copyright (C) 2011 The Android Open Source Project
 *
 * Original code licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this software except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This implements a lights hardware library for the Android emulator.
 * the following code should be built as a shared library that will be
 * placed into /system/lib/hw/lights.goldfish.so
 *
 * It will be loaded by the code in hardware/libhardware/hardware.c
 * which is itself called from
 * ./frameworks/base/services/jni/com_android_server_HardwareService.cpp
 *
 * Modified by J. Wolff
 * Support of backlight control on Odroid board via pwm on pin 33.
 */

#include <android/log.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdbool.h>
#include <hardware/lights.h>
#include <hardware/hardware.h>

#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <dirent.h>

#define  LOG_TAG    "lights.odroid"
/* Set to 1 to enable debug messages to the log */
#define DEBUG 1
#if DEBUG
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#else
# define LOGD(...) do{}while(0)
#endif
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define init_module(mod, len, opts) syscall(__NR_init_module, mod, len, opts)
#define delete_module(name, flags) syscall(__NR_delete_module, name, flags)

#define BACKLIGHT_PWM          "backlight_pwm"
#define BACKLIGHT_PWM_YES          "yes"
#define BACKLIGHT_PWM_NO           "no"
#define BACKLIGHT_PWM_INV          "invert"

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

char backlight_control[64] = "/sys/class/gpio/export";
char backlight_gpio491[64] = "/sys/class/gpio/gpio491/direction";
char backlight_export[64] = "/sys/class/pwm/pwmchip4/export";
char backlight_unexport[64] = "/sys/class/pwm/pwmchip4/unexport";
char backlight_en[64] = "/sys/class/pwm/pwmchip4/pwm0/enable";
char backlight_period[64] = "/sys/class/pwm/pwmchip4/pwm0/period";
char backlight_duty_cycle[64] = "/sys/class/pwm/pwmchip4/pwm0/duty_cycle";


char * env_backlight;
bool invert, enable;

void init_globals(void)
{
    LOGD( "in: %s", __FUNCTION__ );

    // init the mutex
    pthread_mutex_init(&g_lock, NULL);

    char value[20];
    int nwr, ret, fd;

    if (invert) {
        // VU8C
        fd = open(backlight_control, O_RDWR);
        if (fd > 0) {
            ret = write(fd, "491", 3);
            close(fd);
        }
        sleep(1);
        fd = open(backlight_gpio491, O_RDWR);
        if (fd > 0) {
            ret = write(fd, "out", 3);
            close(fd);
        }
        LOGD("enable backlight for ODROID-VU8");
    }

    if (enable) {
        fd = open(backlight_export, O_WRONLY);
        if (fd > 0) {
            ret = write(fd, "0", 1);
            close(fd);
        }
        fd = open(backlight_period, O_RDWR);
        if (fd > 0) {
            nwr = sprintf(value, "%d\n", 1000000);
            ret = write(fd, value, nwr);
            close(fd);
        }

        fd = open(backlight_en, O_RDWR);
        if (fd > 0) {
            nwr = sprintf(value, "%d\n", 1);
            ret = write(fd, value, nwr);
            close(fd);
        }
    }
    LOGD( "leaving  %s", __FUNCTION__ );
}

/* set backlight brightness by LIGHTS_SERVICE_NAME service. */
static int
set_light_backlight( struct light_device_t* dev, struct light_state_t const* state )

{
    int nwr, ret = -1, fd;
    char value[20];
    int light_level;

    pthread_mutex_lock(&g_lock);
    light_level = state->color&0xff;
    //light_level range 10 ~ 255
    if (invert) {
        //ODROID-VU8: Avaiable PWM range 1000000 ~ 100000
        light_level = (int)(light_level * 3674);
        light_level = 1000000 - light_level;
    } else {
        //ODROID-VU other: Avaiable PWM range 100000 ~ 1000000
        light_level = (int)(light_level * 3674);
    }
    LOGD("light_level = %d", light_level);

    fd = open(backlight_duty_cycle, O_RDWR);
    if (fd > 0) {
        nwr = sprintf(value, "%d\n", light_level);
        ret = write(fd, value, nwr);
        close(fd);
    }
    pthread_mutex_unlock(&g_lock);
    return ret;
}

static int
set_light_buttons( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    LOGD( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}

static int
set_light_battery( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    LOGD( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}

static int
set_light_keyboard( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    LOGD( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}
static int
set_light_notifications( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    LOGD( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}

static int
set_light_attention( struct light_device_t* dev, struct light_state_t const* state )
{
    /* @Waiting for later implementation. */
    LOGD( "%s: Not implemented.", __FUNCTION__ );

    return 0;
}

/** Close the lights device */
static int
close_lights( struct light_device_t *dev )
{
    free( dev );

    int ret, fd;
    fd = open(backlight_unexport, O_RDWR);
    if (fd > 0) {
        ret = write(fd, "0", 1);
        close(fd);
    }

    return 0;
}
/**
 * module methods
 */
/** Open a new instance of a lights device using name */
static int
open_lights( const struct hw_module_t* module, char const *name,
        struct hw_device_t **device )
{
    void* set_light;

    if (0 == strcmp( LIGHT_ID_BACKLIGHT, name )) {
        set_light = set_light_backlight;

        //check the bootargs for backlight_pwm
        FILE * fp;
        char * line = NULL;
        char * list;
        size_t len = 0;

        fp = fopen("/proc/cmdline", "r");
        if (fp != NULL) {
            getline(&line, &len, fp);
            list = strtok (line, " ");
            while (list != NULL)
            {
                LOGD ("%s\n",list);
                if (0 == strncmp(BACKLIGHT_PWM, list, 13)) {
                    env_backlight = strtok(list, "=");
                    env_backlight = strtok(NULL, "=");
                    break;
                }
                list = strtok (NULL, " ");
            }
        }
        enable = false;
        invert = false;
        if (env_backlight != NULL) {
            LOGD("backlight_pwm : %s", env_backlight);

            if (0 == strncmp( BACKLIGHT_PWM_YES, env_backlight, 3 )) {
                enable = true;
            } else if (0 == strncmp( BACKLIGHT_PWM_NO, env_backlight, 2 )) {
                //enable = false;
                //return -EINVAL;
            } else if (0 == strncmp( BACKLIGHT_PWM_INV, env_backlight, 6 )) {
                enable = true;
                invert = true;
            } else {
                enable = false;
            }
        }
    } else if (0 == strcmp( LIGHT_ID_KEYBOARD, name )) {
        set_light = set_light_keyboard;
    } else if (0 == strcmp( LIGHT_ID_BUTTONS, name )) {
        set_light = set_light_buttons;
    } else if (0 == strcmp( LIGHT_ID_BATTERY, name )) {
        set_light = set_light_battery;
    } else if (0 == strcmp( LIGHT_ID_NOTIFICATIONS, name )) {
        set_light = set_light_notifications;
    } else if (0 == strcmp( LIGHT_ID_ATTENTION, name )) {
        set_light = set_light_attention;
    } else {
        LOGD( "%s: %s light isn't supported yet.", __FUNCTION__, name );
        return -EINVAL;
    }

    if (enable)
        pthread_once(&g_init, init_globals);

    struct light_device_t *dev = malloc( sizeof(struct light_device_t) );
    if (dev == NULL) {
        return -EINVAL;
    }
    memset( dev, 0, sizeof(*dev) );

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The emulator lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Odroid lights Module",
    .author = "HARDKERNEL",
    .methods = &lights_module_methods,
};
