#include "health_controls.h"
#include "health_data.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "FreeRTOS.h"
#include "task.h"

/* Sample pin assignment; verify against the whole team's schematic.
   PA0 SET, PA1 UP, PA2 DOWN: active low buttons tied to GND.
   PB8 alert LED (through resistor), PB9 logic output to ACTIVE buzzer module.
   DO NOT power a high-current buzzer directly from PB9. */
#define BTN_SET GPIO_Pin_0
#define BTN_UP  GPIO_Pin_1
#define BTN_DN  GPIO_Pin_2
#define WARN_LED GPIO_Pin_8
#define WARN_BUZZ GPIO_Pin_9

static volatile uint8_t g_mode=0;
uint8_t HealthControls_GetMode(void) {return g_mode;}
static void buttons_init(void)
{
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    GPIO_StructInit(&g);
    g.GPIO_Pin=BTN_SET|BTN_UP|BTN_DN;
    g.GPIO_Mode=GPIO_Mode_IPU;
    GPIO_Init(GPIOA,&g);
}
static void adjust(int8_t dir)
{
    HealthLimits l=HealthLimits_Get();
    switch(g_mode) {
        case 1:
            if(dir>0 && l.bpm_max<220) l.bpm_max++;
            if(dir<0 && l.bpm_max>40) l.bpm_max--;
            break;
        case 2:
            if(dir>0 && l.spo2_min<100) l.spo2_min++;
            if(dir<0 && l.spo2_min>70) l.spo2_min--;
            break;
        case 3:
            if(dir>0 && l.temp_max_x10<450) l.temp_max_x10++;
            if(dir<0 && l.temp_max_x10>300) l.temp_max_x10--;
            break;
        default: return;
    }
    HealthLimits_Set(l);
}
void Health_Buttons_Task(void *argument)
{
    // raw: trạng thái 3 nút vừa đọc được
    // prev: trạng thái 3 nút trước đó
    // stable: trạng thái 3 nút nhấn khi đã được xác nhận ổn định
    // count: đếm số lần đọc trạng thái giống nhau liên tiếp
    uint8_t prev=0,stable=0,count=0,raw;
    (void)argument;
    buttons_init();
    // Đọc trạng thái 3 nút
    for(;;) {
        raw=0;
        if(GPIO_ReadInputDataBit(GPIOA,BTN_SET)==Bit_RESET) raw|=1;
        if(GPIO_ReadInputDataBit(GPIOA,BTN_UP)==Bit_RESET) raw|=2;
        if(GPIO_ReadInputDataBit(GPIOA,BTN_DN)==Bit_RESET) raw|=4;
        if(raw==prev) {if(count<3) count++;} else {prev=raw;count=0;}
        if(count==3 && raw!=stable) {
            uint8_t rising=raw & (uint8_t)~stable;
            stable=raw;
            // thưc hiện chức năng của nút 
            if(rising&1) g_mode=(uint8_t)((g_mode+1)%4);
            else if(rising&2) adjust(+1);
            else if(rising&4) adjust(-1);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
static void warning_init(void)
{
    GPIO_InitTypeDef g;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    GPIO_StructInit(&g);
    g.GPIO_Pin=WARN_LED|WARN_BUZZ;
    g.GPIO_Mode=GPIO_Mode_Out_PP;
    g.GPIO_Speed=GPIO_Speed_2MHz;
    GPIO_Init(GPIOB,&g);
    GPIO_ResetBits(GPIOB,WARN_LED|WARN_BUZZ);
}
void Health_Warning_Task(void *argument)
{
    uint8_t on=0;
    (void)argument;
    warning_init();
    for(;;) {
        HealthReading r=HealthData_Get();
        HealthLimits l=HealthLimits_Get();
        uint8_t alert=r.valid && (r.bpm>l.bpm_max || r.spo2<l.spo2_min || r.temp_x10>l.temp_max_x10);
        if(alert) {
            on=!on; // Đảo trạng thái, nhấp nháy led và buzzer
            if(on) GPIO_SetBits(GPIOB,WARN_LED|WARN_BUZZ);
            else GPIO_ResetBits(GPIOB,WARN_LED|WARN_BUZZ);
        } else {
            on=0; // Tắt led và buzzer khi ko có cảnh báo
            GPIO_ResetBits(GPIOB,WARN_LED|WARN_BUZZ);
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
