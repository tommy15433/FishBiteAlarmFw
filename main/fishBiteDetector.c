#include "fishBiteDetector.h"
#include "esp_log.h"

#define DATA_SIZE  10
#define BASE_SIZE   10

#define TAG "FishBiteDetector"

static double data_buffer[DATA_SIZE] = {0};
// static int data_buffer_index = DATA_SIZE - 1;   // last data inserted index
static int data_buffer_index = 0;


static double base_buffer[BASE_SIZE] = {0};
static int base_buffer_index = 0;
static double base_avg = 0;

static int runningCnt = 0;

static double threshold = 0.05;
static int isInitialized = 0;

static double baseValue = 0;

enum{
    STATE_INIT = 0,
    STATE_INIT_BASE = 1,
    STATE_RUNNING = 2,
    STATE_UPDATE_BASE = 3,
};

static int curState = STATE_INIT;

void get_buffer_min_max_avg(double* buffer, int size, double* min, double* max, double* avg)
{
    *min = buffer[0];
    *max = buffer[0];
    *avg = 0;

    double sum = 0;

    for (int i = 0; i < size; i++)
    {
        sum += buffer[i];
        if (buffer[i] > *max)
        {
            *max = buffer[i];
        }
        if (buffer[i] < *min)
        {
            *min = buffer[i];
        }
    }

    *avg = sum / size;
}

int get_prev_index(int offs)
{
    if (data_buffer_index - offs < 0){
        return DATA_SIZE - offs;
    }
    else{
        return data_buffer_index - offs;
    }
}


int fbd_check_fishbite(void)
{
    int preIdx = get_prev_index(1);
    int curIdx = data_buffer_index;

    double delta = data_buffer[curIdx] - data_buffer[preIdx]; 

    //ESP_LOGI("FBD", "delta: %f", delta);

    if (delta > threshold ||
        delta < -threshold)
    {
        if (isInitialized == 1)
        {
            return 1;
        }
        else
        {
            isInitialized = 1;
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

#define IIR_RATIO 20

static bool fishBiteDetectionFlag = false;
// when fish bite is detected
static void (*fish_bite_detect_event_task)() = NULL;
// when fish bite is released after detection
static void (*fish_bite_release_event_task)() = NULL;

void fbd_update(double val)
{
    double filteredVal = 0;
    double min, max, avg;

    switch (curState)
    {
        case STATE_INIT:
            ESP_LOGI(TAG, "STATE_INIT");
            
            base_buffer_index = 0;

            curState = STATE_INIT_BASE;
        break;

        case STATE_INIT_BASE:
            ESP_LOGI(TAG, "STATE_INIT_BASE");

            base_buffer[base_buffer_index++] = val;
            if (base_buffer_index >= BASE_SIZE){
                base_avg = 0;
                for (int i = 0; i < BASE_SIZE; i++)
                {
                    ESP_LOGI(TAG, "base_buffer[%d]: %f", i, base_buffer[i]);
                    base_avg += base_buffer[i]; 
                }
                base_avg = base_avg / BASE_SIZE;
                curState = STATE_RUNNING;
            }
        break;

        case STATE_RUNNING:
            filteredVal = (base_avg * (IIR_RATIO - 1) / IIR_RATIO) +  (val / IIR_RATIO);
            //ESP_LOGI(TAG, "STATE_RUNNING. base: %f inserted %f filtered %f", base_avg, val, filteredVal);

            //data_buffer[data_buffer_index] = filteredVal;
            data_buffer[data_buffer_index] = val;

            if (++data_buffer_index >= DATA_SIZE){
                data_buffer_index = 0;
            }

            get_buffer_min_max_avg(data_buffer, DATA_SIZE, &min, &max, &avg);
            if ( (min < avg - threshold) ||
                 (max > avg + threshold) )
            {
                runningCnt = 0;

                if (fishBiteDetectionFlag == false)
                {
                    fishBiteDetectionFlag = true;
                    if (fish_bite_detect_event_task != NULL)
                    {
                        (*fish_bite_detect_event_task)();
                    }
                }
            }
            else
            {
                if (fishBiteDetectionFlag == true)
                {
                    fishBiteDetectionFlag = false;
                    if (fish_bite_release_event_task != NULL)
                    {
                        (*fish_bite_release_event_task)();
                    }
                }

                runningCnt++;
                if (runningCnt > BASE_SIZE)
                {
                    runningCnt = 0;
                    curState = STATE_UPDATE_BASE;
                }
            }


            // if (val > threshold ||
            //     val < -threshold)
            // {
            //     runningCnt = 0;

            //     if (fish_bite_detect_event_task != NULL)
            //     {
            //         (*fish_bite_detect_event_task)();
            //     }
            // }
            // else
            // {
            //     runningCnt++;
            //     if (runningCnt > BASE_SIZE)
            //     {
            //         runningCnt = 0;
            //         curState = STATE_UPDATE_BASE;
            //     }
            // }
        break;

        case STATE_UPDATE_BASE:
            ESP_LOGI(TAG, "STATE_UPDATE_BASE");

            base_avg = 0;
            for (int i = 0; i < BASE_SIZE; i++)
            {
                base_avg += data_buffer[get_prev_index(i)];
            }
            base_avg = base_avg / BASE_SIZE;
            curState = STATE_RUNNING;
        break;
    }
}

void fbd_sensitivity_low(void)
{
    fbd_sensitivity(0.3);
}
void fbd_sensitivity_mid(void)
{
    fbd_sensitivity(0.2);
}
void fbd_sensitivity_high(void)
{
    fbd_sensitivity(0.1);
}
void fbd_sensitivity(double value)
{
    threshold = value;
    ESP_LOGI(TAG, "Threshold: %f", threshold);
}
void fbd_reset()
{
    curState = STATE_INIT;
}

void fbd_set_detect_handler(void (*fn_ptr))
{
    fish_bite_detect_event_task = fn_ptr;
}

void fbd_set_release_handler(void (*fn_ptr))
{
    fish_bite_release_event_task = fn_ptr;
}

void fbd_init(void)
{
    fbd_sensitivity_mid();
}

