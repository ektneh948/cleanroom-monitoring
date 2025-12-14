#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "stm32f4xx_hal.h"  // HAL ����ü ����� ���� ����

//#define ADC_REGULAR_RANK_1             ((uint32_t)0x00000001U)
//���̽�ƽ ���� ������ ���¸� �����ϴ� ����ü
typedef struct {
	ADC_HandleTypeDef* hadc_x;         // X�� ���̽�ƽ�� ����� ADC �ڵ�
	uint32_t adc_channel_x;            // X�� ADC ä�� ��ȣ

	ADC_HandleTypeDef* hadc_y;         // Y�� ���̽�ƽ�� ����� ADC �ڵ�
	uint32_t adc_channel_y;            // Y�� ADC ä�� ��ȣ

	GPIO_TypeDef* button_gpio_port;    // ��ư�� ����� GPIO ��Ʈ (��: GPIOC)
	uint16_t button_gpio_pin;          // ��ư �� ��ȣ (��: GPIO_PIN_13)

	uint16_t x_value;                  // �ֱ� ���� X�� ADC �� (0~4095)
	uint16_t y_value;                  // �ֱ� ���� Y�� ADC �� (0~4095)
	GPIO_PinState button_state;        // �ֱ� ���� ��ư ���� (SET or RESET)

	uint32_t last_button_tick;         // ��ٿ���� ���� ������ ��ư ���� ��ȭ �ð� (HAL_GetTick ����)
}Joystick_HandleTypeDef;

//���̽�ƽ�� ���� ���� raw������ �����ϴ� ����ü
typedef struct {
	uint16_t x;                        // X�� ADC ��
	uint16_t y;                        // Y�� ADC ��
	GPIO_PinState button;             // ��ư ����
}Joystick_Data_t;

//���̽�ƽ�� �̵� �̷� �� ��ǥ�� �����ϴ� ����ü
typedef struct {
	// X�� �̵� �����丮 (bit ����Ʈ ���)
	uint8_t x_plus_history;
	uint8_t x_minus_history;
	// Y�� �̵� �����丮
	uint8_t y_plus_history;
	uint8_t y_minus_history;

	int8_t x_pos;                    // X�� ��ǥ (������ ���� ���)
	int8_t y_pos;                    // Y�� ��ǥ
}Joystick_Tracker_t;

// ���̽�ƽ �ʱ�ȭ �Լ�
void Joystick_Init(Joystick_HandleTypeDef* hjs,
                   ADC_HandleTypeDef* hadc_x, uint32_t ch_x,
                   ADC_HandleTypeDef* hadc_y, uint32_t ch_y,
                   GPIO_TypeDef* btn_port, uint16_t btn_pin);

// ���̽�ƽ���� ADC �� ��ư ���¸� polling ������� �о� ��ȯ
Joystick_Data_t Joystick_Read(Joystick_HandleTypeDef* hjs, uint16_t adc_timeout);

// ��ٿ���� ����Ͽ� ��ư ���¸� �д� �Լ�
GPIO_PinState Joystick_GetButton(Joystick_HandleTypeDef* hjs, uint16_t debounce_ms);

// X, Y �̵� �̷��� ������� ��ǥ�� �����ϴ� ���� �Լ�
void Joystick_Track(Joystick_Tracker_t* tracker, Joystick_Data_t* data,
                    uint16_t high_threshold, uint16_t low_threshold);


#endif
