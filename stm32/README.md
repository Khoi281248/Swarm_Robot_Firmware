# STM32 Swarm Robot – Firmware

Firmware cho vi điều khiển **STM32F103C8T6** trên mạch robot bầy đàn (swarm robot).
STM32 lo phần thời gian thực: điều khiển 2 động cơ, đọc encoder, đọc cảm biến IMU, đo pin.
Module **ESP32-C3** trên cùng mạch lo truyền thông không dây (Wi-Fi / ESP-NOW) và trao đổi lệnh, trạng thái với STM32 qua UART.

```
   Robot khác / máy chủ
          │  Wi-Fi, ESP-NOW
   ┌──────┴───────┐   UART 115200   ┌──────────────────┐   PWM   ┌──────────┐
   │  ESP32-C3    │ ◄─────────────► │  STM32F103C8T6   │ ──────► │ DRV8833  │ ──► 2 động cơ
   │(truyền thông)│ NRST, BOOT0 ──► │ (điều khiển)     │ ◄────── │ encoder  │
   └──────────────┘                 └────────┬─────────┘         └──────────┘
                                             │ I2C
                                       ICM-42670-P (IMU)
```

## Phần cứng

| Thành phần | Linh kiện | Ghi chú |
|---|---|---|
| Vi điều khiển | STM32F103C8T6 | 64 KB Flash, 20 KB RAM |
| Thạch anh | HSE 16 MHz | HSE/2 × PLL9 = **72 MHz** |
| Truyền thông | ESP32-C3-WROOM-02 | Điều khiển NRST (GPIO6) và BOOT0 (GPIO7) của STM32 |
| Cảm biến quán tính | ICM-42670-P | I2C, địa chỉ `0x68` (AD0 = GND), `WHO_AM_I` = `0x67` |
| Cầu H động cơ | DRV8833 | Chân nFAULT có điện trở kéo lên 10 kΩ |
| Đo pin | Cầu chia 100 kΩ / 100 kΩ | V(PA3) = VBAT / 2 |

## Sơ đồ chân

| Chân | Chức năng | Ngoại vi | Ghi chú |
|---|---|---|---|
| PA6 | AIN1 – động cơ A | TIM3_CH1 | PWM 20 kHz (PSC = 0, ARR = 3599) |
| PA7 | AIN2 – động cơ A | TIM3_CH2 | |
| PB1 | BIN1 – động cơ B | TIM3_CH4 | |
| PB0 | BIN2 – động cơ B | TIM3_CH3 | |
| PA0, PA1 | Encoder động cơ A (J1) | TIM2 encoder | TI1 và TI2, filter 8, pull-up |
| PB6, PB7 | Encoder động cơ B (J2) | TIM4 encoder | TI1 và TI2, filter 8, pull-up |
| PB10, PB11 | SCL, SDA của IMU | I2C2 | 400 kHz |
| PA2 | `IMU_INT` | EXTI2, cạnh lên | Pull-down; IMU cấu hình INT ở chế độ push-pull, tích cực mức cao |
| PA8 | `MOTOR_nFAULT` | EXTI8, cạnh xuống | DRV8833 báo lỗi khi chân này xuống mức thấp |
| PA3 | VBAT / 2 | ADC1_IN3 | Thời gian lấy mẫu 239,5 chu kỳ |
| PA9, PA10 | TX, RX nối ESP32-C3 | USART1 | 115200 8N1 |
| PC13 | `LED_STM` | GPIO | Tích cực mức thấp |
| PA13, PA14 | SWDIO, SWCLK | SWD | Cổng nạp J3 |

TIM1 được dùng làm **timebase của HAL** (SysTick để dành cho FreeRTOS), vì vậy không dùng TIM1 cho việc khác.

## Công cụ cần cài

| Công cụ | Phiên bản | Dùng để |
|---|---|---|
| STM32CubeMX | **6.18.1** | Sửa file `.ioc` và sinh lại code |
| Gói STM32Cube FW_F1 | **V1.8.7** | HAL cho dòng F1 (CubeMX tự tải) |
| STM32CubeIDE | 2.2.0 | Build, nạp, debug (cách 1) |
| VS Code + extension *STM32Cube for VS Code* | 3.x | Build, nạp, debug (cách 2) |
| GNU Tools for STM32 | 14.3.1 | Trình biên dịch (đi kèm CubeIDE / bundle của VS Code) |

> Mọi thành viên phải dùng **cùng phiên bản CubeMX và gói FW_F1**, nếu không code sinh ra sẽ khác nhau và gây conflict.

## Cấu trúc thư mục

```
stm32/
├── STM32_Swarm_Robot.ioc     Cấu hình CubeMX (chân, clock, ngoại vi, FreeRTOS)
├── CMakeLists.txt            File CMake chính (thêm file .c tự viết ở đây)
├── CMakePresets.json         Preset Debug / Release
├── cmake/                    Toolchain arm-none-eabi và CMake do CubeMX sinh
├── Core/
│   ├── Inc/                  main.h, FreeRTOSConfig.h, các file .h ngoại vi
│   └── Src/                  main.c, freertos.c, tim.c, i2c.c, usart.c, adc.c, ...
├── Drivers/                  HAL và CMSIS của ST (không sửa)
├── Middlewares/              FreeRTOS (không sửa)
├── startup_stm32f103xb.s     Vector ngắt, khởi động
└── STM32F103xx_FLASH.ld      Linker script
```

## Build

### Cách 1: STM32CubeIDE 2.2
1. **File → Import → General → Existing Projects into Workspace**, chọn thư mục `stm32/`.
2. Tick **STM32_Swarm_Robot**, **bỏ tick** *Copy projects into workspace*, bấm **Finish**.
3. Chọn project, bấm **Build** (Ctrl+B). Nếu hiện hộp thoại *Existing build directory found*,
   chọn **Delete and Reconfigure**.

### Cách 2: VS Code
1. Mở thư mục này bằng VS Code. Cài bundle `gnu-tools-for-stm32` khi extension hỏi.
2. `Ctrl+Shift+P` → **CMake: Select Configure Preset** → `Debug`.
3. Bấm **F7** để build.

### Cách 3: dòng lệnh
```bash
cmake --preset Debug
cmake --build --preset Debug
```

Kết quả: `build/Debug/STM32_Swarm_Robot.elf`.

## Nạp và debug

Cổng **J3** (SWD) nối với ST-Link:

| J3 | Tín hiệu |
|---|---|
| 1 | 3,3 V (không nối nếu board đã có nguồn) |
| 2 | SWDIO |
| 3 | SWCLK |
| 4 | GND |

- J3 **không có chân NRST**. Trong cấu hình debug, đặt *Reset behaviour* = **Software system reset**.
- **Nếu không kết nối được** (firmware lỡ tắt SWD hoặc chip đang ngủ): gạt **SW3 (BOOT0) = 1**, nhấn **SW1 (reset)**, nạp lại, rồi gạt SW3 về 0.
- Có thể nạp qua ESP32-C3: ESP32 đưa BOOT0 lên 1, reset STM32 vào bootloader ROM, rồi gửi chương trình qua USART1 (8E1). Đang phát triển.

## FreeRTOS

| Thông số | Giá trị |
|---|---|
| Interface | CMSIS-RTOS v2 |
| `configTOTAL_HEAP_SIZE` | 10240 byte |
| `configCHECK_FOR_STACK_OVERFLOW` | 2 |
| `configUSE_NEWLIB_REENTRANT` | 1 (cần cho `snprintf` trong task) |
| Timebase HAL | TIM1 |

Kiến trúc task dự kiến:

| Task | Chu kỳ | Ưu tiên | Việc |
|---|---|---|---|
| `controlTask` | 10 ms | Cao nhất | Đọc encoder, PID, PWM, odometry |
| `imuTask` | Theo ngắt IMU (100–200 Hz) | Cao | Đọc IMU, lọc Mahony, tính góc yaw |
| `commTask` | Theo gói UART | Trung bình | Nhận lệnh, gửi trạng thái cho ESP32-C3 |
| `monitorTask` | 500 ms | Thấp | Đo VBAT, kiểm tra nFAULT, nháy LED |

Quy tắc:
- Mỗi ngoại vi chỉ do một task dùng (I2C2 → `imuTask`, UART1 → `commTask`).
- Dữ liệu dùng chung phải bảo vệ bằng mutex hoặc vùng găng (critical section).
- Ngắt nào gọi hàm FreeRTOS thì phải có priority ≥ 5 và chỉ dùng các hàm `...FromISR`.

## Làm việc nhóm

1. **Chỉ một người phụ trách file `.ioc`** và bấm GENERATE. Người khác cần đổi chân hay ngoại vi thì tạo Issue.
2. Code tự viết **chỉ đặt trong vùng `/* USER CODE BEGIN … */ … /* USER CODE END … */`** hoặc trong file riêng. Code nằm ngoài các vùng này sẽ bị xoá khi GENERATE.
3. Mỗi việc làm trên một nhánh riêng, rồi gộp vào `main` qua Pull Request:
   ```bash
   git switch main && git pull
   git switch -c ten-nhanh
   # ... sửa code, build thử ...
   git add . && git commit -m "Mo ta thay doi"
   git push -u origin ten-nhanh
   gh pr create --fill
   ```
4. Luôn commit trước khi bấm GENERATE trong CubeMX, để nếu cần thì lấy lại được code cũ.

## Tiến độ

- [x] Cấu hình CubeMX: clock, PWM, encoder, I2C, UART, ADC, EXTI, FreeRTOS
- [x] Build được bằng CubeIDE và VS Code
- [ ] Test phần cứng: LED, IMU `WHO_AM_I`, VBAT, encoder, động cơ
- [ ] `controlTask`: PID tốc độ hai bánh
- [ ] `imuTask`: đọc IMU, lọc Mahony
- [ ] `commTask`: giao thức UART với ESP32-C3
- [ ] Nạp firmware STM32 qua ESP32-C3

## Giấy phép

Code của nhóm phát hành theo giấy phép [MIT](../LICENSE).
Thư viện STM32 HAL/CMSIS (`Drivers/`) và FreeRTOS (`Middlewares/`) giữ nguyên giấy phép gốc của từng thư viện.
