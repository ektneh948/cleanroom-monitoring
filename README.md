# 🏭클린룸 모니터링 / 제어 프로젝트  
*STM32 기반 센서 노드와 TCP/IP 서버를 활용한 실시간 환경 모니터링 & 제어 IoT 시스템*

## 🧭 프로젝트 개요

**미세먼지 / 온·습도 데이터**를 **TCP/IP 기반 서버로 전송**하고, 여러 클라이언트가 동시에 접속하여 **모니터링 및 제어를 수행하는 IoT 통합 시스템**입니다.  
IoT 시스템의 전체 흐름을 실제 하드웨어와 네트워크 환경에서 구현하는 것을 목표로 진행했습니다.  

<p align="center">
  <img src="https://github.com/user-attachments/assets/7baf4157-11ca-46fd-917e-5a4340834723" width="28%">
  <img src="https://github.com/user-attachments/assets/f687df31-1a78-472e-a042-af9b47c88749" width="28%">
</p>

---

## 🗺️ 전체 시스템 구성
<img width="820" height="494" alt="image" src="https://github.com/user-attachments/assets/b54a3360-6c6e-46d2-aa78-d60235f11dca" />

### 구성 요소

| 구성 요소 | 역할 | 주요 기능 |
|---------|------|----------|
| STM32 Wi-Fi 센서 노드 | 센서 데이터 수집 | 미세먼지 / 온·습도 측정, TCP 전송 |
| IoT TCP 서버 | 데이터 중계 | 다중 클라이언트 관리, 메시지 전달 |
| PC / DB 클라이언트 | 데이터 관리 | 센서 데이터 저장·조회 |
| Bluetooth LCD | 원격 제어 | 센서 표시, FAN 제어 |


---

## ⭐ 주요 기능

<img width="820" height="494" alt="image" src="https://github.com/user-attachments/assets/1be53671-e487-44af-ad4c-7dd7f1f31a7f" />

### 센서 데이터 측정 및 전송  

  ```
  [SENSOR:CMS_SQL]ptcl|temp|humi
  ```


| 기능 | 설명 |
|----|----|
| 센서 데이터 측정 | 미세먼지·온습도 센서 측정 |
| 네트워크 통신 | TCP/IP 기반 다중 클라이언트 |
| 원격 FAN 제어 | 자동·수동 제어 |


---
<img width="820" height="494" alt="image" src="https://github.com/user-attachments/assets/d386debe-d871-4a34-94dd-35054a352975" />


### TCP/IP 기반 네트워크 통신

| 항목 | 내용 |
|----|----|
| 연결 방식 | STM32 ↔ TCP Server |
| 통신 모듈 | ESP8266 (AT Command) |
| 안정성 처리 | 재접속 로직 |
| 수신 처리 | 제어 명령 파싱 |

---

### 원격 FAN 제어

| 모드 | 동작 방식 |
|----|----------|
|자동 | 임계치 초과 시 FAN ON |
|수동 | 원격 명령으로 속도 제어 |


---

## 👤 담당 역할 (미세먼지 · 온·습도 측정 / TCP/IP Network Programming)

| 구분 | 담당 내용 | 사용 기술 |
|----|---------|---------|
| 미세먼지 센서 | GP2Y 아날로그 데이터 측정 및 노이즈 감소 처리 | ADC, 다중 샘플링 |
| 온·습도 센서 | DHT11 타이밍 기반 데이터 수신 및 예외 처리 | GPIO, Timing Control |
| TCP/IP 통신 | STM32 ↔ 서버 간 TCP 통신 구현 | ESP8266, AT Command |
| 제어 명령 | 서버 제어 명령 수신 및 FAN 제어 | Command Parsing |

