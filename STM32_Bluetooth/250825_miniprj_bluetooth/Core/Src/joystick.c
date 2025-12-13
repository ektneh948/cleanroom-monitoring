#include "joystick.h"

#ifndef __STM32F4xx_ADC_H
#include "stm32f4xx_hal_adc.h"  // Rank ���� ����
#endif


// ���ο����� ����ϴ� ADC polling �Լ�
static uint16_t read_adc_polling(ADC_HandleTypeDef* hadc, uint32_t channel, uint16_t timeout) {
	ADC_ChannelConfTypeDef sConfig = {0};                 // ä�� ���� ����ü �ʱ�ȭ
	sConfig.Channel = channel;                            // ���� ADC ä�� ����
	sConfig.Rank = 1;                    // ���� ��ȯ���� ���
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;        // ���ø� �ð� (ª��)

	HAL_ADC_ConfigChannel(hadc, &sConfig);                // ���� ����
	HAL_ADC_Start(hadc);                                  // ��ȯ ����
	HAL_ADC_PollForConversion(hadc, timeout);             // ��ȯ �Ϸ� ���
	uint16_t val = HAL_ADC_GetValue(hadc);                // �� �б�
	HAL_ADC_Stop(hadc);                                   // ��ȯ ����
	return val;
}

//���̽�ƽ �ڵ� �ʱ�ȭ
void Joystick_Init(Joystick_HandleTypeDef* hjs,
									 ADC_HandleTypeDef* hadc_x, uint32_t ch_x,
									 ADC_HandleTypeDef* hadc_y, uint32_t ch_y,
									 GPIO_TypeDef* btn_port, uint16_t btn_pin) {

	hjs->hadc_x = hadc_x;
	hjs->adc_channel_x = ch_x;
	hjs->hadc_y = hadc_y;
	hjs->adc_channel_y = ch_y;
	hjs->button_gpio_port = btn_port;
	hjs->button_gpio_pin = btn_pin;

	hjs->x_value = 0;
	hjs->y_value = 0;
	hjs->button_state = GPIO_PIN_RESET;
	hjs->last_button_tick = 0;
}

//���̽�ƽ�� raw ADC���� ��ư ���¸� polling ������� �а� ����ü�� ��ȯ
Joystick_Data_t Joystick_Read(Joystick_HandleTypeDef* hjs, uint16_t timeout) {
	Joystick_Data_t data;
	data.x = read_adc_polling(hjs->hadc_x, hjs->adc_channel_x, timeout);
	data.y = read_adc_polling(hjs->hadc_y, hjs->adc_channel_y, timeout);
	//data.button = HAL_GPIO_ReadPin(hjs -> button_gpio_port, hjs -> button_gpio_pin);
	data.button = Joystick_GetButton(hjs, 100);

	// ���� ���尪 ������Ʈ
	hjs->x_value = data.x;
	hjs->y_value = data.y;
	hjs->button_state = data.button;

	return data;
}

//��ư ���� ��ȯ �Լ�
GPIO_PinState Joystick_GetButton(Joystick_HandleTypeDef* hjs, uint16_t debounce_ms) {

	static uint8_t button_press = 0;

	// ���� ��ư ���� �Է� ���¸� ���� Ǯ�� �����̱� ������ ������ LOW
	GPIO_PinState current = HAL_GPIO_ReadPin(hjs->button_gpio_port, hjs->button_gpio_pin);
//
//	if (current == GPIO_PIN_RESET) return GPIO_PIN_SET;
//	else return GPIO_PIN_RESET;

//	if (current == GPIO_PIN_RESET)
//		HAL_Delay(debounce_ms);
//
//	current = HAL_GPIO_ReadPin(hjs->button_gpio_port, hjs->button_gpio_pin);

//	return !current;

	//��ư �Է��� LOW�� ��
	if(current == GPIO_PIN_RESET){
		button_press |= 1;
	}

	button_press <<= 1;

	return ((button_press == 0x80) ? GPIO_PIN_SET : GPIO_PIN_RESET);

	//button_press &= 0x3F;
//	if(button_press == 0b10000000){
	//if(button_press == 0b00100000){
	//if ((button_press & 0x0C) == 0x08){
//		if((button_press & 0xF0) != 0){
//			return GPIO_PIN_RESET;
//		}
//		else{
//			return GPIO_PIN_SET;
//		}
//	}
//	else
//		return GPIO_PIN_RESET;


	/*
	// ���� �ð� (ms ����, �ý��� ���� ���� ��� �ð�)�� �о�´�
	uint32_t now = HAL_GetTick();

	// ���� ���� ���°� ���� ���¿� �ٸ� ��� (���� ��ȭ ����)
	if (current != hjs->button_state) {

		// ������ ���� ��ȭ ���� debounce_ms��ŭ �ð��� �����ٸ� ��ȿ�� ��ȭ�� �Ǵ�
		if ((now - hjs->last_button_tick) >= debounce_ms) {

			// ��ư ���¸� ���� ������ ������Ʈ (SET �Ǵ� RESET)
			hjs->button_state = current;

			// ������ ���� ��ȭ �ð��� ���� �ð����� ����
			hjs->last_button_tick = now;
		}
	}
	else {
		// ���� ��ȭ�� ���� ��쿡�� ���� �ð����� ���� (ª�� �ð� �� ��ȭ ��ȿȭ��)
		hjs->last_button_tick = now;
	}

	// ��ٿ���� ����� ���� ��ư ���¸� ��ȯ
	return hjs->button_state;
	*/
}


void Joystick_Track(Joystick_Tracker_t* tracker, Joystick_Data_t* data,
                    uint16_t high_th, uint16_t low_th) {
	// X�� �����丮 ������Ʈ
	tracker->x_plus_history <<= 1;
	tracker->x_minus_history <<= 1;

	if (data->x > high_th)
		tracker->x_plus_history |= 0x01; //������ �̵�
	else if (data->x < low_th)
		tracker->x_minus_history |= 0x01; //���� �̵�
//	else{
//		//�߾ӿ� ���� ����Ʈ
//		tracker -> x_plus_history <<= 1;
//		tracker -> x_minus_history <<= 1;
//	}

	// Y�� �����丮 ������Ʈ
	tracker->y_plus_history <<= 1;
	tracker->y_minus_history <<= 1;

	if (data->y > high_th)
		tracker->y_plus_history |= 0x01; // ���� �̵�
	else if (data->y < low_th)
		tracker->y_minus_history |= 0x01; //�Ʒ��� �̵�
//	else{
//		//�߾ӿ� ���� ����Ʈ
//		tracker -> y_plus_history <<= 1;
//		tracker -> y_minus_history <<= 1;
//	}

	tracker->x_pos = 0;
	tracker->y_pos = 0;

	// �������ٰ� ���ƿ� ��� ��ǥ ����
	//if ((tracker->x_plus_history & 0b00000011) == 0b00000010){
	//if ((tracker->x_plus_history & 0b00000011) == 0b00000001){
//	if ((tracker->x_plus_history & 0b00000001) == 0b00000001){
//		tracker->x_pos = 1;
//		tracker -> x_plus_history <<= 1;
//	}
//
//	if ((tracker->x_minus_history & 0b00000001) == 0b00000001){
//		tracker->x_pos = -1;
//		tracker -> x_minus_history <<= 1;
//	}
//
//	if ((tracker->y_plus_history & 0b00000001) == 0b00000001){
//		tracker->y_pos = 1;
//		tracker -> y_plus_history <<= 1;
//	}
//
//	if ((tracker->y_minus_history & 0b00000001) == 0b00000001){
//		tracker->y_pos = -1;
//		tracker -> y_minus_history <<= 1;
//	}
}

