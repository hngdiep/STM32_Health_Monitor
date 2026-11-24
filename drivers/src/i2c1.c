
#include "i2c1.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_i2c.h"

#include "FreeRTOS.h"
#include "task.h"

static int SensorI2C1_WaitEvent(uint32_t event)
{
    uint32_t timeout = 100000;

    while (I2C_CheckEvent(I2C1, event) != SUCCESS)
    {
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) == SET)
            return 0;

        if (--timeout == 0)
            return 0;
    }

    return 1;
}

void SensorI2C1_Init(void)
{
    GPIO_InitTypeDef gpio;
    I2C_InitTypeDef i2c;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    I2C_DeInit(I2C1);
    I2C_StructInit(&i2c);

    i2c.I2C_ClockSpeed = 100000;
    i2c.I2C_Mode = I2C_Mode_I2C;
    i2c.I2C_DutyCycle = I2C_DutyCycle_2;
    i2c.I2C_OwnAddress1 = 0;
    i2c.I2C_Ack = I2C_Ack_Enable;
    i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    I2C_Init(I2C1, &i2c);
    I2C_Cmd(I2C1, ENABLE);
}

int SensorI2C1_Probe(uint8_t address7)
{
    uint32_t timeout = 100000;

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET)
    {
        if (--timeout == 0)
            return 0;
    }

    I2C_GenerateSTART(I2C1, ENABLE);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
        goto error;

    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1),
                        I2C_Direction_Transmitter);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        goto error;

    I2C_GenerateSTOP(I2C1, ENABLE);
    return 1;

error:
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_ClearFlag(I2C1, I2C_FLAG_AF);
    return 0;
}


int SensorI2C1_ReadRegister8(uint8_t address7, uint8_t reg, uint8_t *value)
{
    uint32_t timeout = 100000U;

    if (value == 0)
        return 0;

    I2C_AcknowledgeConfig(I2C1, ENABLE);

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET)
    {
        if (--timeout == 0)
            return 0;
    }

    I2C_GenerateSTART(I2C1, ENABLE);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
        goto error;

    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1), I2C_Direction_Transmitter);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        goto error;

    I2C_SendData(I2C1, reg);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        goto error;

    I2C_GenerateSTART(I2C1, ENABLE);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
        goto error;

    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1), I2C_Direction_Receiver);

    timeout = 100000U;

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET)
    {
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) == SET || --timeout == 0)
            goto error;
    }

    taskENTER_CRITICAL();

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    I2C_GenerateSTOP(I2C1, ENABLE);

    taskEXIT_CRITICAL();

    timeout = 100000U;

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET)
    {
        if (--timeout == 0)
            goto error;
    }

    *value = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, ENABLE);

    return 1;

error:
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_ClearFlag(I2C1, I2C_FLAG_AF);
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    return 0;
}


int SensorI2C1_WriteRegister8(uint8_t address7, uint8_t reg, uint8_t value)
{
    uint32_t timeout = 100000U;

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET)
    {
        if (--timeout == 0)
            return 0;
    }

    I2C_GenerateSTART(I2C1, ENABLE);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
        goto error;

    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1),
                        I2C_Direction_Transmitter);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        goto error;

    I2C_SendData(I2C1, reg);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        goto error;

    I2C_SendData(I2C1, value);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        goto error;

    I2C_GenerateSTOP(I2C1, ENABLE);

    return 1;

error:
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_ClearFlag(I2C1, I2C_FLAG_AF);

    return 0;
}


int SensorI2C1_ReadBurst(uint8_t address7, uint8_t reg, uint8_t *buffer, uint8_t length)
{
    uint32_t timeout;
    uint8_t remaining;
    uint8_t *ptr = buffer;

    if (buffer == 0 || length < 3U)
        return 0;

    I2C_AcknowledgeConfig(I2C1, ENABLE);
    I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);

    timeout = 100000U;

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET)
    {
        if (--timeout == 0U)
            return 0;
    }

    I2C_GenerateSTART(I2C1, ENABLE);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
        goto error;

    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1),
                        I2C_Direction_Transmitter);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        goto error;

    I2C_SendData(I2C1, reg);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
        goto error;

    I2C_GenerateSTART(I2C1, ENABLE);

    if (!SensorI2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
        goto error;

    I2C_Send7bitAddress(I2C1, (uint8_t)(address7 << 1),
                        I2C_Direction_Receiver);

    timeout = 100000U;

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET)
    {
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) == SET || --timeout == 0U)
            goto error;
    }

    (void)I2C1->SR1;
    (void)I2C1->SR2;

    remaining = length;

    while (remaining > 3U)
    {
        timeout = 100000U;

        while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET)
        {
            if (--timeout == 0U)
                goto error;
        }

        *ptr++ = I2C_ReceiveData(I2C1);
        --remaining;
    }

    timeout = 100000U;

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET)
    {
        if (--timeout == 0U)
            goto error;
    }

    taskENTER_CRITICAL();

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    *ptr++ = I2C_ReceiveData(I2C1);
    I2C_GenerateSTOP(I2C1, ENABLE);
    *ptr++ = I2C_ReceiveData(I2C1);

    taskEXIT_CRITICAL();

    timeout = 100000U;

    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET)
    {
        if (--timeout == 0U)
            goto error;
    }

    *ptr = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, ENABLE);

    return 1;

error:
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_ClearFlag(I2C1, I2C_FLAG_AF);
    I2C_AcknowledgeConfig(I2C1, ENABLE);

    return 0;
}
