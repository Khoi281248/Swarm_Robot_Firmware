# Nhật ký cấu hình firmware STM32 – Swarm Robot

Tài liệu này ghi lại **mọi thiết lập** của project `STM32_Swarm_Robot`: đã đặt **cái gì**, **tại sao** đặt như vậy, và **nếu đổi thì hỏng gì**.
Đọc file này trước khi sửa `.ioc`, sửa cấu hình build hay viết code dùng ngoại vi.

- Ảnh chụp cấu hình đầy đủ (hình chip, cây clock, thông số ngoại vi) do CubeMX xuất: [`STM32_Swarm_Robot.pdf`](STM32_Swarm_Robot.pdf), bản chữ: [`STM32_Swarm_Robot.txt`](STM32_Swarm_Robot.txt).
- Hướng dẫn build, nạp nhanh: [`README.md`](../README.md).

> **Quy tắc:** ai thay đổi cấu hình (file `.ioc`, `CMakeLists.txt`, `FreeRTOSConfig`, …) thì **cập nhật file này trong cùng commit**, và thêm một dòng vào mục [Lịch sử thay đổi](#16-lịch-sử-thay-đổi).

---

## Mục lục
1. [Trạng thái hiện tại](#1-trạng-thái-hiện-tại)
2. [Phần cứng liên quan đến firmware](#2-phần-cứng-liên-quan-đến-firmware)
3. [Clock](#3-clock)
4. [SYS, debug và timebase của HAL](#4-sys-debug-và-timebase-của-hal)
5. [GPIO](#5-gpio)
6. [NVIC – ưu tiên ngắt](#6-nvic--ưu-tiên-ngắt)
7. [TIM3 – PWM cho động cơ](#7-tim3--pwm-cho-động-cơ)
8. [TIM2, TIM4 – đọc encoder](#8-tim2-tim4--đọc-encoder)
9. [I2C2 – cảm biến IMU ICM-42670-P](#9-i2c2--cảm-biến-imu-icm-42670-p)
10. [USART1 – giao tiếp với ESP32-C3](#10-usart1--giao-tiếp-với-esp32-c3)
11. [ADC1 – đo điện áp pin](#11-adc1--đo-điện-áp-pin)
12. [FreeRTOS](#12-freertos)
13. [Bộ nhớ RAM và Flash](#13-bộ-nhớ-ram-và-flash)
14. [Sinh code và hệ thống build](#14-sinh-code-và-hệ-thống-build)
15. [Các sự cố đã gặp và cách sửa](#15-các-sự-cố-đã-gặp-và-cách-sửa)
16. [Lịch sử thay đổi](#16-lịch-sử-thay-đổi)
17. [Việc còn lại](#17-việc-còn-lại)

---

## 1. Trạng thái hiện tại

*Cập nhật: 27/10/2026*

| Hạng mục | Trạng thái |
|---|---|
| Cấu hình CubeMX (clock, chân, ngoại vi, FreeRTOS) | ✅ Xong |
| Build bằng VS Code và STM32CubeIDE 2.2 | ✅ Được, cùng compiler GNU Tools for STM32 14.3.1 |
| Code ứng dụng | ⏳ Chưa có. `main.c` và `freertos.c` vẫn là code CubeMX sinh, chưa viết gì trong vùng USER CODE |
| Test trên mạch thật | ⏳ Chưa. Chưa cắm ST-Link |
| Stack của `defaultTask` | ⚠️ Đang là 128 words, **cần tăng lên 512** trước khi viết code có `snprintf` (xem [mục 12](#12-freertos)) |

Phiên bản công cụ: **STM32CubeMX 6.18.1**, gói **STM32Cube FW_F1 V1.8.7**, toolchain **CMake**.

---

## 2. Phần cứng liên quan đến firmware

Các thông tin dưới đây lấy từ netlist của mạch KiCad. Firmware phụ thuộc trực tiếp vào chúng.

### 2.1. Đầu nối

| Đầu nối | Chân 1 | Chân 2 | Chân 3 | Chân 4 | Chân 5 | Chân 6 |
|---|---|---|---|---|---|---|
| **J1** (động cơ A) | AOUT1 | +3,3 V | Encoder A → PA0 | Encoder B → PA1 | GND | AOUT2 |
| **J2** (động cơ B) | BOUT1 | +3,3 V | Encoder A → PB6 | Encoder B → PB7 | GND | BOUT2 |
| **J3** (SWD) | +3,3 V | SWDIO (PA13) | SWCLK (PA14) | GND | – | – |

- Encoder được cấp nguồn **3,3 V**, nên mức logic của encoder khớp với STM32.
- **J3 không có chân NRST.** Debug phải dùng *Software system reset* (xem [mục 4](#4-sys-debug-và-timebase-của-hal)).

### 2.2. Nút nhấn, công tắc, LED

| Linh kiện | Nối với | Tác dụng |
|---|---|---|
| **SW1** | NRST ↔ GND | Reset STM32 |
| **SW3** | BOOT0: chọn GND hoặc 3,3 V | BOOT0 = 1: khởi động vào bootloader ROM (dùng để cứu chip) |
| **R27** | PB2 (BOOT1) → GND | BOOT1 luôn = 0 |
| **D4** | PC13, cathode nối vào chân | LED sáng khi PC13 = 0 (tích cực mức thấp) |

### 2.3. Linh kiện ảnh hưởng đến cấu hình

| Linh kiện | Ảnh hưởng |
|---|---|
| Thạch anh HSE **16 MHz** | Phải chia 2 trước PLL ([mục 3](#3-clock)); `HSE_VALUE = 16000000` trong `stm32f1xx_hal_conf.h` |
| **R25 = R26 = 100 kΩ**, **C29 = 100 nF** | Cầu chia VBAT/2 vào PA3; ADC phải lấy mẫu lâu ([mục 11](#11-adc1--đo-điện-áp-pin)) |
| **R28 = 10 kΩ** kéo lên nFAULT | PA8 không cần bật pull nội |
| IMU: **AD0 = GND**, chân INT **không có** điện trở kéo lên | Địa chỉ I2C 0x68; phải cấu hình INT của IMU sang push-pull ([mục 9](#9-i2c2--cảm-biến-imu-icm-42670-p)) |
| Encoder **không có** điện trở kéo lên trên mạch | Bật pull-up nội cho PA0, PA1, PB6, PB7 |

### 2.4. Nối với ESP32-C3

| STM32 | Hướng | ESP32-C3 | Ghi chú |
|---|---|---|---|
| PA9 (USART1_TX) | → | GPIO5 (RX) | Net `RX_ESP` |
| PA10 (USART1_RX) | ← | GPIO4 (TX) | Net `TX_ESP` |
| NRST | ← | GPIO6 | ESP kéo xuống để reset STM32 (open-drain) |
| BOOT0 | ← | GPIO7 | ESP đưa lên 1 để STM32 vào bootloader, nạp firmware qua UART |

---

## 3. Clock

```
HSE 16 MHz ──► ÷2 ──► 8 MHz ──► PLL ×9 ──► SYSCLK 72 MHz
                                             │
                                             ├─► AHB  ÷1 ──► HCLK 72 MHz (CPU, DMA, bộ nhớ)
                                             ├─► APB1 ÷2 ──► PCLK1 36 MHz ──► timer APB1 ×2 = 72 MHz (TIM2, TIM3, TIM4)
                                             ├─► APB2 ÷1 ──► PCLK2 72 MHz (TIM1, USART1, GPIO)
                                             └─► ADC  ÷6 ──► 12 MHz
```

| Thông số | Giá trị | Tại sao |
|---|---|---|
| Nguồn clock | HSE (thạch anh 16 MHz) | Chính xác hơn HSI nội 8 MHz (±1 %), nên baud UART và chu kỳ PID ổn định |
| `HSEDivPLL` | **÷2** | PLL của F103 chỉ nhân tối đa ×16. Với 16 MHz: 16 × 4,5 không đặt được, nên phải chia 2 thành 8 MHz rồi ×9 = 72 MHz |
| `PLLMUL` | ×9 | 8 × 9 = 72 MHz, tốc độ tối đa của STM32F103 |
| APB1 | **÷2** | APB1 tối đa 36 MHz. Khi APB1 có chia, clock timer trên APB1 được nhân 2, nên **TIM2, TIM3, TIM4 vẫn chạy 72 MHz** |
| APB2 | ÷1 | APB2 cho phép 72 MHz |
| ADC | **÷6** = 12 MHz | ADC của F1 tối đa **14 MHz**. Chia 4 thì ra 18 MHz, quá giới hạn |
| USB | 72 MHz (không hợp lệ) | USB cần 48 MHz. Board **không dùng USB của STM32** nên bỏ qua |

> **Nếu đổi:** đổi clock là thay đổi tần số PWM, baud UART, tốc độ I2C, chu kỳ tick. CubeMX tự tính lại phần lớn, nhưng các hằng số trong code (ví dụ `PWM_MAX`) phải sửa tay.

---

## 4. SYS, debug và timebase của HAL

| Thông số | Giá trị | Tại sao |
|---|---|---|
| Debug | **Serial Wire** (PA13, PA14) | Chỉ cần 2 dây, khớp với J3. Chọn *No Debug* là **mất SWD** sau lần nạp đầu tiên |
| Timebase Source | **TIM1** | FreeRTOS chiếm SysTick làm tick của hệ điều hành. Nếu HAL cũng dùng SysTick thì hai bên tranh nhau, `HAL_Delay` và timeout của HAL chạy sai |

- **Không dùng TIM1 cho việc khác** (PWM, input capture…), vì nó đang là đồng hồ của HAL. Hàm `HAL_TIM_PeriodElapsedCallback` trong `main.c` gọi `HAL_IncTick()` mỗi khi TIM1 tràn.
- Ngắt `TIM1_UP` có priority **15** (`TICK_INT_PRIORITY = 15`), thấp nhất, giống SysTick của FreeRTOS.

**Cấu hình debug** (CubeIDE: *Run → Debug Configurations*, VS Code: `launch.json`):

| Thông số | Giá trị | Lý do |
|---|---|---|
| Interface | SWD | |
| **Reset behaviour** | **Software system reset** | J3 không có chân NRST, nên ST-Link không reset cứng chip được |
| Live expressions | Bật | Xem biến toàn cục khi chip đang chạy |

**Cứu chip khi không kết nối được** (lỡ tắt SWD, chip ngủ sâu, firmware lỗi treo ngay khi khởi động):
1. Gạt **SW3 (BOOT0) = 1**.
2. Nhấn **SW1** (reset). Chip chạy bootloader ROM, không chạy firmware lỗi.
3. Nạp lại firmware qua ST-Link.
4. Gạt **SW3 = 0**, nhấn SW1.

---

## 5. GPIO

| Chân | Nhãn (User Label) | Chế độ | Pull | Mức ban đầu | Tại sao |
|---|---|---|---|---|---|
| PC13 | `LED_STM` | Output push-pull | – | **High** | LED tích cực mức thấp. High = LED tắt khi khởi động |
| PA2 | `IMU_INT` | EXTI, **cạnh lên** | **Pull-down** | – | Sau khi cấu hình, IMU xuất INT push-pull, tích cực cao. Pull-down giữ chân ở mức 0 trong lúc IMU chưa được cấu hình |
| PA8 | `MOTOR_nFAULT` | EXTI, **cạnh xuống** | Không | – | nFAULT của DRV8833 là open-drain, đã có R28 kéo lên. Lỗi (quá dòng, quá nhiệt, thấp áp) kéo chân xuống, tạo cạnh xuống |

Nhãn User Label sinh ra các macro trong `main.h`: `LED_STM_Pin`, `LED_STM_GPIO_Port`, `IMU_INT_Pin`, `MOTOR_nFAULT_Pin`… Code **luôn dùng tên macro**, không viết `GPIO_PIN_13`. Như vậy nếu đổi chân thì chỉ cần sửa trong CubeMX.

Ngắt EXTI được xử lý tập trung trong callback:
```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == MOTOR_nFAULT_Pin) { /* DRV8833 bao loi */ }
    if (GPIO_Pin == IMU_INT_Pin)      { /* IMU co du lieu moi */ }
}
```

---

## 6. NVIC – ưu tiên ngắt

**Priority Group 4**: 4 bit dành hết cho preemption priority (0–15), không có sub-priority. FreeRTOS yêu cầu như vậy.
**Số càng nhỏ, ưu tiên càng cao.**

| Ngắt | Priority | Gọi được hàm FreeRTOS | Ghi chú |
|---|---|---|---|
| EXTI2 (`IMU_INT`) | **5** | ✅ | Sau này dùng để đánh thức `imuTask` |
| EXTI9_5 (`MOTOR_nFAULT`) | **5** | ✅ | |
| USART1 | **5** | ✅ | Sau này dùng để báo `commTask` có gói mới |
| TIM1_UP (timebase HAL) | 15 | – | |
| SysTick (tick FreeRTOS) | 15 | – | |
| PendSV (đổi task) | 15 | – | |

**Quy tắc bắt buộc** (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`):
- Ngắt có **priority 0–4**: **không được** gọi bất kỳ hàm FreeRTOS nào. Gọi vào là treo ngẫu nhiên, rất khó tìm lỗi.
- Ngắt có **priority 5–15**: được gọi, nhưng **chỉ các hàm có đuôi `FromISR`** (`xQueueSendFromISR`, `vTaskNotifyGiveFromISR`, …). Nếu dùng CMSIS-RTOS v2 thì `osThreadFlagsSet`, `osMessageQueuePut` (timeout = 0) cũng gọi được trong ngắt.
- Khi thêm ngắt mới trong CubeMX, tích cột **Uses FreeRTOS functions** để CubeMX tự kiểm tra priority.

---

## 7. TIM3 – PWM cho động cơ

| Thông số | Giá trị |
|---|---|
| Clock Source | Internal Clock (72 MHz) |
| Prescaler (PSC) | 0 |
| Counter Period (ARR) | **3599** |
| Counter Mode | Up |
| auto-reload preload | Enable |
| Kênh 1–4 | PWM mode 1, Pulse = 0, preload Enable, Polarity High |

**Tần số PWM** = 72 MHz / (PSC + 1) / (ARR + 1) = 72 000 000 / 1 / 3600 = **20 kHz**.
- 20 kHz nằm ngoài vùng tai người nghe, nên động cơ không kêu rít. DRV8833 hỗ trợ PWM tới 250 kHz.
- Độ phân giải: 3600 mức. Duty (%) = Pulse / 3600 × 100.
- Trong code: `#define PWM_MAX 3599`. Đổi ARR thì **phải sửa hằng số này**.

**Ghép kênh với DRV8833:**

| Kênh | Chân | Chân DRV8833 |
|---|---|---|
| CH1 | PA6 | AIN1 |
| CH2 | PA7 | AIN2 |
| CH4 | PB1 | **BIN1** |
| CH3 | PB0 | **BIN2** |

> Chú ý: động cơ B **bị đảo thứ tự**: CH4 là BIN1, CH3 là BIN2 (do đi dây trên mạch). Viết code điều khiển động cơ B phải theo đúng bảng này.

**Bảng điều khiển DRV8833** (một động cơ, xIN1 / xIN2):

| xIN1 | xIN2 | Kết quả |
|---|---|---|
| PWM | 0 | Quay tiến, **fast decay** (lúc tắt xung thì động cơ chạy trớn) |
| 0 | PWM | Quay lùi, fast decay |
| 1 | PWM | Quay tiến, **slow decay** (lúc tắt xung thì phanh). Duty thực = 1 − duty đặt |
| PWM | 1 | Quay lùi, slow decay |
| 0 | 0 | Thả trôi (coast) |
| 1 | 1 | Phanh (brake) |

Code test hiện dùng **fast decay**: một chân PWM, chân kia bằng 0. Slow decay cho tốc độ tuyến tính hơn theo duty, nên cân nhắc khi chỉnh PID.

Code phải gọi `HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_x)` cho cả 4 kênh thì mới có xung. CubeMX chỉ khởi tạo, không tự start.

---

## 8. TIM2, TIM4 – đọc encoder

| Thông số | TIM2 (động cơ A) | TIM4 (động cơ B) |
|---|---|---|
| Chân | PA0 (CH1), PA1 (CH2) | PB6 (CH1), PB7 (CH2) |
| Combined Channels | Encoder Mode | Encoder Mode |
| Encoder Mode | **TI1 and TI2** | **TI1 and TI2** |
| Prescaler | 0 | 0 |
| Counter Period | 65535 | 65535 |
| Polarity (CH1, CH2) | Rising | Rising |
| **Input Filter** (CH1, CH2) | **8** | **8** |
| GPIO Pull | **Pull-up** | **Pull-up** |

**Tại sao:**
- **TI1 and TI2:** đếm cả cạnh lên và cạnh xuống của cả hai kênh, nên **số đếm mỗi vòng = 4 × PPR** (PPR là số xung mỗi vòng của encoder). Độ phân giải gấp 4 lần so với chỉ đếm một kênh.
- **Filter = 8:** lấy mẫu ở f_DTS/8 = 9 MHz, cần 6 mẫu liên tiếp giống nhau mới nhận một cạnh. Xung nhiễu ngắn hơn khoảng **0,67 µs** bị bỏ qua. Nhiễu này hay sinh ra do chổi than động cơ và PWM. Encoder động cơ nhỏ chỉ ra vài chục kHz, nên bộ lọc không làm mất xung thật.
- **Pull-up:** mạch không có điện trở kéo lên. Encoder loại open-collector bắt buộc phải có pull-up. Encoder loại push-pull thì pull-up nội cũng không gây hại.
- **Lưu ý cũ:** lúc đầu cấu hình nhầm *TI1* với filter 0 (xem [mục 15](#15-các-sự-cố-đã-gặp-và-cách-sửa)).

**Cách đọc trong code:**
```c
HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);    /* goi 1 lan luc khoi dong */

int16_t cnt = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
```
- Bộ đếm là 16 bit (0–65535), quay lùi qua 0 thì nhảy lên 65535. Ép sang `int16_t` để đi lùi thành số âm.
- Để tính tốc độ, lấy **hiệu** giữa hai lần đọc, ép kiểu `int16_t`:
  ```c
  int16_t delta = (int16_t)(cnt_now - cnt_prev);
  ```
  Hiệu này đúng cả khi bộ đếm tràn, miễn là mỗi chu kỳ đọc quay chưa tới 32768 xung. Với chu kỳ 10 ms thì luôn thỏa.
- Nếu động cơ quay tiến mà số đếm giảm: đổi Polarity của **một** kênh sang Falling, hoặc đổi dấu trong code.

---

## 9. I2C2 – cảm biến IMU ICM-42670-P

| Thông số | Giá trị |
|---|---|
| Chân | PB10 = SCL, PB11 = SDA |
| Speed Mode | **Fast Mode, 400 kHz** |
| Fast Mode Duty Cycle | Tlow/Thigh = 2 |
| Địa chỉ 7 bit | **0x68** (AD0 = GND) |
| Địa chỉ dùng trong HAL | `0x68 << 1` = 0xD0 (HAL cần địa chỉ đã dịch trái 1 bit) |

**Tại sao I2C2, không phải I2C1:** I2C1 dùng PB6, PB7, mà hai chân này đã dành cho encoder TIM4. Nếu bật nhầm I2C1 thì PB6, PB7 chuyển sang màu cam (xung đột).

**Thanh ghi IMU dùng khi khởi động:**

| Thanh ghi | Địa chỉ | Giá trị | Ý nghĩa |
|---|---|---|---|
| `WHO_AM_I` | 0x75 | đọc ra **0x67** | Kiểm tra IMU đúng loại và I2C hoạt động. Đọc ra 0x00 hoặc lỗi thì kiểm tra hàn, nguồn, dây |
| `INT_CONFIG` | 0x06 | ghi **0x03** | bit1 = 1: INT1 **push-pull**; bit0 = 1: **tích cực mức cao**; bit2 = 0: dạng xung |

**Vì sao phải ghi `INT_CONFIG`:** mặc định IMU xuất INT kiểu open-drain, tích cực thấp. Mạch không có điện trở kéo lên cho đường INT, nên ở chế độ mặc định chân này "lơ lửng". Sau khi ghi 0x03, IMU tự đẩy chân lên 1 khi có sự kiện, khớp với cấu hình PA2: EXTI cạnh lên, pull-down.

**Lỗi đã biết của I2C trên STM32F1:** sau khi reset giữa chừng một giao dịch, IMU có thể giữ SDA ở mức thấp, làm I2C kẹt ở trạng thái BUSY. Mọi lệnh `HAL_I2C_*` sẽ trả về lỗi. Cách xử lý: trước `MX_I2C2_Init`, chuyển PB10 thành GPIO và tạo khoảng 9 xung trên SCL để giải phóng bus. Chỉ cần làm khi gặp lỗi này.

---

## 10. USART1 – giao tiếp với ESP32-C3

| Thông số | Giá trị |
|---|---|
| Chân | PA9 = TX, PA10 = RX |
| Mode | Asynchronous, không flow control |
| Baud | **115200** |
| Khung | **8N1** (8 bit, không parity, 1 stop) |
| Over Sampling | 16 |
| Ngắt | USART1 global interrupt, priority 5 |

**Tại sao USART1:**
- PA9, PA10 cũng là chân của **bootloader ROM** trong STM32. ESP32-C3 có thể nạp firmware cho STM32 qua chính đường này, sau khi đưa BOOT0 lên 1 và reset.

> **Chú ý:** bootloader ROM dùng khung **8E1** (parity chẵn), còn ứng dụng dùng **8N1**. Firmware ESP32 phải đổi cấu hình UART khi chuyển giữa chế độ nạp và chế độ chạy bình thường.

**Dự kiến:** nhận dữ liệu bằng DMA kết hợp ngắt IDLE (`HAL_UARTEx_ReceiveToIdle_DMA`), rồi đẩy gói vào queue cho `commTask`. Khi làm phần này cần bật DMA cho USART1_RX trong CubeMX.

---

## 11. ADC1 – đo điện áp pin

| Thông số | Giá trị |
|---|---|
| Kênh | IN3 (PA3) |
| Clock ADC | 12 MHz |
| Data Alignment | Right |
| Scan / Continuous / Discontinuous | Tắt / Tắt / Tắt |
| Trigger | Phần mềm |
| **Sampling Time** | **239,5 cycles** |

**Tại sao lấy mẫu 239,5 chu kỳ:**
- Trở kháng nguồn của cầu chia là 100 kΩ // 100 kΩ = **50 kΩ**, rất lớn so với mức ADC khuyến nghị (khoảng 10 kΩ).
- Lấy mẫu ngắn thì tụ lấy mẫu trong ADC chưa nạp đủ, kết quả đo bị **thấp hơn** thực tế.
- C29 = 100 nF ở chân PA3 giúp cấp dòng tức thời cho lần lấy mẫu.
- Thời gian một lần chuyển đổi = (239,5 + 12,5) / 12 MHz ≈ **21 µs**. Đo pin vài lần mỗi giây nên không ảnh hưởng gì.

**Công thức:**
```
V_PA3  = raw / 4095 × 3,3 V
VBAT   = V_PA3 × 2                 (cầu chia 100k/100k)
VBAT_mV = raw × 3300 × 2 / 4095
```

| VBAT | V(PA3) | raw (xấp xỉ) |
|---|---|---|
| 4,20 V (đầy) | 2,10 V | 2606 |
| 3,70 V (danh định) | 1,85 V | 2296 |
| **3,40 V (ngưỡng dừng động cơ)** | 1,70 V | **2110** |
| 3,00 V (cạn) | 1,50 V | 1861 |

- Gọi `HAL_ADCEx_Calibration_Start(&hadc1)` **một lần** sau khi khởi tạo, trước lần đo đầu tiên.
- Công thức giả sử VDDA = 3,3 V chính xác. Nếu cần chính xác hơn, đo thêm kênh VREFINT (khoảng 1,2 V) để bù.

---

## 12. FreeRTOS

| Thông số | Giá trị | Tại sao |
|---|---|---|
| Interface | **CMSIS_V2** | API chuẩn của ARM (`osThreadNew`, `osDelay`, `osMessageQueueNew`…). Vẫn gọi được hàm FreeRTOS gốc (`xTaskCreate`, …) khi cần |
| `configTICK_RATE_HZ` | 1000 | Tick 1 ms, `osDelay(10)` = 10 ms |
| `configTOTAL_HEAP_SIZE` | **10240** byte | Mặc định 3072 chỉ đủ 1–2 task. Mọi task, queue, mutex tạo ra lúc chạy đều lấy bộ nhớ từ đây |
| Quản lý heap | `heap_4.c` | Cho phép giải phóng và gộp các khối trống |
| `configCHECK_FOR_STACK_OVERFLOW` | **2** | Phát hiện tràn stack, gọi `vApplicationStackOverflowHook` trong `freertos.c` |
| `configUSE_NEWLIB_REENTRANT` | **1** | Mỗi task có vùng dữ liệu newlib riêng, nên gọi `snprintf` trong nhiều task không làm hỏng dữ liệu của nhau |
| `configMAX_PRIORITIES` | 56 | Bắt buộc với CMSIS_V2 (có 56 mức `osPriority`) |
| `configMINIMAL_STACK_SIZE` | 128 words | Stack của task Idle |
| `configUSE_TIMERS` | 1, stack 256 words | Phục vụ software timer (`osTimerNew`) |
| `configUSE_MUTEXES` | 1 | |
| `configUSE_TRACE_FACILITY` | 1 | Cho phép xem danh sách task khi debug |
| `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` | 5 | Xem [mục 6](#6-nvic--ưu-tiên-ngắt) |

> **Đơn vị stack:** CubeMX tính stack bằng **words** (1 word = 4 byte). 128 words = 512 byte.

**Task hiện có:**

| Task | Entry | Priority | Stack | Ghi chú |
|---|---|---|---|---|
| `defaultTask` | `StartDefaultTask` (`Core/Src/freertos.c`) | Normal | **128 words** ⚠️ | **Phải tăng lên 512**: `snprintf` cần khoảng 1 KB stack |

**Cách sửa stack:** CubeMX → FREERTOS → Tasks and Queues → bấm đúp `defaultTask` → Stack Size = 512 → GENERATE.

**Điểm khác biệt quan trọng khi dùng FreeRTOS:**
- `main()` gọi `osKernelStart()` và **không bao giờ chạy tới `while(1)`**. Code lặp lại phải đặt **trong task**.
- Code khởi tạo một lần (start PWM, start encoder, hiệu chuẩn ADC, cấu hình IMU) đặt ở `USER CODE BEGIN 2` trong `main.c`, trước khi scheduler chạy.
- Trong task dùng **`osDelay`**, không dùng `HAL_Delay`. `HAL_Delay` chiếm CPU suốt thời gian chờ; `osDelay` cho task ngủ để task khác chạy.
- Task chạy theo chu kỳ cố định (PID) dùng `osDelayUntil` để chu kỳ không bị trôi.

**Kiến trúc task dự kiến:**

| Task | Chạy khi nào | Priority | Stack (dự kiến) | Việc |
|---|---|---|---|---|
| `controlTask` | Mỗi 10 ms (`osDelayUntil`) | Realtime / High | 256 words | Đọc encoder → PID → PWM → odometry |
| `imuTask` | Khi có ngắt IMU (thread flag) | AboveNormal | 256 words | Đọc I2C → lọc Mahony → góc yaw |
| `commTask` | Khi có gói UART (queue) | Normal | 384 words | Giải mã lệnh, gửi trạng thái |
| `monitorTask` (thay `defaultTask`) | Mỗi 500 ms | BelowNormal | 512 words | VBAT, nFAULT, LED |

**Quy tắc chia sẻ:**
1. Mỗi ngoại vi chỉ do **một task** dùng: I2C2 → `imuTask`, UART1 TX → `commTask`. Task khác cần dùng thì gửi yêu cầu qua queue.
2. Dữ liệu dùng chung (vận tốc đặt, góc yaw, vị trí) gom vào struct, bảo vệ bằng mutex hoặc `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`.
3. Biến được ngắt ghi vào phải khai báo `volatile`.

---

## 13. Bộ nhớ RAM và Flash

Số liệu đo từ bản build Debug ngày 27/10/2026 (có FreeRTOS, chưa có code ứng dụng):

| Vùng | Đã dùng | Tổng | % |
|---|---|---|---|
| FLASH | 27,6 KB | 64 KB | 42 % |
| **RAM** | **16,7 KB** | 20 KB | **82 %** |

**RAM dùng vào đâu:**

| Thành phần | Kích thước | Ghi chú |
|---|---|---|
| Heap FreeRTOS (`ucHeap`) | 10 240 B | Cấp phát tĩnh từ lúc build. Task, queue tạo sau sẽ lấy từ đây, **không làm tăng con số RAM lúc build** |
| Danh sách task sẵn sàng (56 mức priority) | 1 120 B | Hệ quả của `configMAX_PRIORITIES = 56` |
| Stack + TCB của task Timer | 1 024 + 384 B | |
| Stack + TCB của task Idle | 512 + 384 B | |
| Heap C + stack của `main` (linker) | 512 + 1 024 B | `_Min_Heap_Size = 0x200`, `_Min_Stack_Size = 0x400`. Stack này cũng là stack của các hàm ngắt |
| Biến HAL, newlib, khác | khoảng 1,5 KB | |

**Còn trống:** khoảng **3,7 KB** ngoài heap FreeRTOS, và khoảng **9 KB** trong heap FreeRTOS để tạo task mới.
Mỗi task tốn **stack + khoảng 400 B** (TCB có kèm vùng newlib). Bốn task theo kiến trúc dự kiến tốn khoảng 7,2 KB, vẫn vừa trong heap 10 KB.

> **Nếu báo `region RAM overflowed`:** giảm `configTOTAL_HEAP_SIZE`, giảm stack các task, hoặc tắt `configUSE_TIMERS` nếu không dùng software timer (tiết kiệm khoảng 1,5 KB).
> **Kiểm tra stack thực tế:** gọi `uxTaskGetStackHighWaterMark(NULL)` trong task. Hàm trả về số word chưa bao giờ dùng tới; nên giữ ít nhất 50 words.

---

## 14. Sinh code và hệ thống build

### 14.1. Thiết lập Project Manager trong CubeMX

| Thông số | Giá trị | Tại sao |
|---|---|---|
| Toolchain / IDE | **CMake** | Dùng được cả CubeIDE 2.x lẫn VS Code |
| Copy library files | Only necessary | Repo gọn hơn (chỉ khoảng 6,5 MB HAL và FreeRTOS) |
| Generate peripheral initialization as a pair of .c/.h | ✅ | Mỗi ngoại vi một file (`tim.c`, `i2c.c`, …), `main.c` gọn |
| **Keep User Code when re-generating** | ✅ | Giữ code trong các vùng `USER CODE` |
| Delete previously generated files | ✅ | Tắt ngoại vi nào thì file của nó bị xoá theo |
| Minimum Heap / Stack | 0x200 / 0x400 | Mặc định. Stack 1 KB cho `main` và các hàm ngắt |

**Quy tắc vàng:** CubeMX **xoá mọi code nằm ngoài** cặp `/* USER CODE BEGIN xxx */ … /* USER CODE END xxx */` mỗi khi GENERATE.
- Luôn **commit trước khi GENERATE**, để còn lấy lại code nếu cần.
- Code ứng dụng lớn nên viết trong **file riêng** (ví dụ `Core/Src/motor.c`) thay vì nhồi vào `main.c`.

### 14.2. Thêm file .c tự viết

Sửa `CMakeLists.txt` ở thư mục gốc. CubeMX không ghi đè file này, chỉ ghi đè `cmake/stm32cubemx/CMakeLists.txt`:
```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/motor.c
)
```
File `.h` đặt trong `Core/Inc` thì không cần khai báo thêm.

### 14.3. Build bằng hai IDE

| | VS Code | STM32CubeIDE 2.2 |
|---|---|---|
| Cách mở | Open Folder | File → Import → STM32 CMake Project |
| Preset | `Debug` (CMakePresets.json) | `Debug` (đọc cùng preset) |
| Thư mục build | `build/Debug` | `build/Debug` (dùng chung) |
| CMake / Ninja | bundle của extension (cmake 4.4.0, ninja 1.13.2), gọi qua `cube-cmake` | bản đi kèm CubeIDE (cmake 4.3.1) |
| Compiler | **GNU Tools for STM32 14.3.1** (bundle) | **GNU Tools for STM32 14.3** (đi kèm CubeIDE) |

- Hai IDE build **chung thư mục** `build/Debug`. Khi đổi qua lại, CMake có thể configure lại từ đầu, mất khoảng 10–20 giây, không sao.
- Nếu báo lỗi lạ về cache hoặc generator: xoá thư mục `build/Debug` rồi build lại. Trong VS Code: *CMake: Delete Cache and Reconfigure*.
- Không dùng `arm-none-eabi-gcc` của Ubuntu (`/usr/bin`, bản 14.2), để cả nhóm dùng cùng một compiler.
- Bảng mã của project trong CubeIDE: **UTF-8** (Project → Properties → Resource).

### 14.4. Những gì có và không có trong git

| Đưa lên git | Không đưa lên (`.gitignore`) |
|---|---|
| `.ioc`, `Core/`, `Drivers/`, `Middlewares/`, `cmake/`, `CMakeLists.txt`, `CMakePresets.json`, linker script, startup | `build/`, `Debug/`, `Release/` |
| `.project`, `.cproject`, `.settings/` (cấu hình CubeIDE và bundle) | `mx.scratch`, `*.bak` |
| `.vscode/`, `.clangd` (cấu hình VS Code) | |

Đưa cả `Drivers/` và `Middlewares/` lên để người khác clone về là **build được ngay**, không cần cài CubeMX.

---

## 15. Các sự cố đã gặp và cách sửa

| # | Hiện tượng | Nguyên nhân | Cách sửa |
|---|---|---|---|
| 1 | Encoder chỉ đếm một kênh, số đếm nhảy lung tung | Encoder Mode = TI1, Input Filter = 0 | Đổi thành **TI1 and TI2**, filter **8**, bật pull-up |
| 2 | CubeIDE báo *"A project with that name already exists in the workspace"* | Lần import trước bị lỗi nhưng vẫn còn trong workspace | Chuột phải project cũ → Delete (**không** tích *Delete project contents*), xoá file `.project` hỏng, import lại bằng *STM32 CMake Project* |
| 3 | Import xong nhưng không build được, icon project trơn | Chọn nhầm thư mục `Core` làm thư mục nguồn | Thư mục nguồn phải là thư mục gốc chứa `CMakeLists.txt` |
| 4 | CubeIDE cảnh báo *"no explicit encoding set"* | Project CMake import vào chưa gán bảng mã | Properties → Resource → Text file encoding = UTF-8 |
| 5 | VS Code build bằng gcc 14.2 thay vì 14.3.1 | Configure chạy trước khi bundle `gnu-tools-for-stm32` tải xong, CMake lấy gcc của hệ thống | Đợi bundle cài xong → *CMake: Delete Cache and Reconfigure* |
| 6 | Build Analyzer của CubeIDE trống | Chưa chọn file `.elf` | Bấm vào `.elf` trong Binaries → Refresh |
| 7 | (Dự kiến) Chương trình treo ở `vApplicationStackOverflowHook` | Stack của `defaultTask` 128 words không đủ cho `snprintf` | Tăng lên 512 words |

---

## 16. Lịch sử thay đổi

| Ngày | Người | Thay đổi |
|---|---|---|
| 27/10/2026 | Khoi281248 | Tạo project CubeMX 6.18.1, FW_F1 1.8.7, toolchain CMake |
| 27/10/2026 | Khoi281248 | Clock 72 MHz từ HSE 16 MHz; TIM3 PWM 20 kHz; TIM2/TIM4 encoder; I2C2 400 kHz; USART1 115200; ADC1 IN3; EXTI PA2, PA8; LED PC13 |
| 27/10/2026 | Khoi281248 | Sửa encoder: TI1 → TI1 and TI2, filter 0 → 8 |
| 27/10/2026 | Khoi281248 | Bật FreeRTOS (CMSIS_V2, heap 10 KB, kiểm tra tràn stack, newlib reentrant); EXTI2, EXTI9_5, USART1 priority 5 |
| 27/10/2026 | Khoi281248 | Build được trên VS Code và CubeIDE 2.2; đưa lên GitHub; thêm README, LICENSE, báo cáo CubeMX |

---

## 17. Việc còn lại

- [ ] Tăng stack `defaultTask` lên 512 words
- [ ] Viết code test phần cứng: nháy LED, đọc `WHO_AM_I`, đo VBAT, đọc encoder, chạy thử động cơ
- [ ] Nạp và debug lần đầu bằng ST-Link qua J3
- [ ] Tách code thành các file `motor.c`, `encoder.c`, `imu.c`, `comm.c`
- [ ] `controlTask`: PID tốc độ hai bánh, chu kỳ 10 ms
- [ ] `imuTask`: đọc IMU theo ngắt, lọc Mahony
- [ ] `commTask`: định nghĩa giao thức gói tin UART với ESP32-C3 (bật DMA cho USART1_RX)
- [ ] Bảo vệ pin: dừng động cơ khi VBAT < 3,4 V
- [ ] Xử lý nFAULT: dừng động cơ, báo lỗi về ESP32
- [ ] Nạp firmware STM32 qua ESP32-C3 (bootloader UART 8E1)
