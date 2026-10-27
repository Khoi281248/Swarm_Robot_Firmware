# Nhật ký cấu hình firmware ESP32-C3 – Swarm Robot

Tài liệu này ghi lại **mọi thiết lập** của project `ESP32_Swarm_Robot`: đã đặt **cái gì**, **tại sao** đặt như vậy, và **nếu đổi thì hỏng gì**.
Đọc file này trước khi sửa `sdkconfig.defaults`, đổi chân GPIO hay cài lại môi trường.

- Hướng dẫn build, nạp nhanh: [`README.md`](../README.md)
- Firmware STM32 và tài liệu cấu hình của nó: [../../stm32/docs/CAU_HINH.md](../../stm32/docs/CAU_HINH.md)

> **Quy tắc:** ai thay đổi cấu hình (`sdkconfig.defaults`, chân GPIO, phân vùng flash, phiên bản ESP-IDF…) thì **cập nhật file này trong cùng commit** và thêm một dòng vào mục [Lịch sử thay đổi](#11-lịch-sử-thay-đổi).

---

## Mục lục
1. [Trạng thái hiện tại](#1-trạng-thái-hiện-tại)
2. [Phần cứng liên quan đến firmware](#2-phần-cứng-liên-quan-đến-firmware)
3. [Cấu hình project (`sdkconfig.defaults`)](#3-cấu-hình-project-sdkconfigdefaults)
4. [Điều khiển STM32: NRST và BOOT0](#4-điều-khiển-stm32-nrst-và-boot0)
5. [UART1 – giao tiếp với STM32](#5-uart1--giao-tiếp-với-stm32)
6. [Flash và phân vùng](#6-flash-và-phân-vùng)
7. [FreeRTOS trên ESP-IDF](#7-freertos-trên-esp-idf)
8. [Cài đặt môi trường](#8-cài-đặt-môi-trường)
9. [Build, nạp, debug](#9-build-nạp-debug)
10. [Các sự cố đã gặp và cách sửa](#10-các-sự-cố-đã-gặp-và-cách-sửa)
11. [Lịch sử thay đổi](#11-lịch-sử-thay-đổi)
12. [Việc còn lại](#12-việc-còn-lại)

---

## 1. Trạng thái hiện tại

*Cập nhật: 27/10/2026*

| Hạng mục | Trạng thái |
|---|---|
| Môi trường ESP-IDF v6.1 (EIM, VS Code extension, udev, dialout) | ✅ Xong |
| Project tạo từ mẫu `sample_project`, target esp32c3 | ✅ Xong |
| `sdkconfig.defaults`: chip, console USB, flash 4 MB, CPU 160 MHz | ✅ Xong, đã build thử thành công |
| Code ứng dụng | ⏳ Chưa có, `app_main()` đang trống |
| Nạp và chạy trên mạch thật | ⏳ Chưa |

Phiên bản công cụ: **ESP-IDF v6.1**, **EIM v0.20.1**, extension VS Code **ESP-IDF 2.3.0**, compiler **riscv32-esp-elf-gcc 15.2.0**, **OpenOCD v0.12.0-esp32-20260703**.

---

## 2. Phần cứng liên quan đến firmware

Thông tin lấy từ netlist của mạch KiCad. Ký hiệu của module trên mạch là **U6**.

### 2.1. Chân đang dùng

| Chân | Net | Nối tới | Dùng để |
|---|---|---|---|
| IO4 | `TX_ESP` | STM32 PA10 (USART1_RX) | UART1 TX |
| IO5 | `RX_ESP` | STM32 PA9 (USART1_TX) | UART1 RX |
| IO6 | `/NRST` | STM32 NRST, R14 kéo lên 3,3 V, C22, nút SW1 | Reset STM32 |
| IO7 | `/BOOT0` | STM32 BOOT0, R21 → công tắc SW3 | Chọn chế độ khởi động STM32 |
| IO18 | `ESP_D-` | USB-C qua U7 (USBLC6-2SC6, chống tĩnh điện) | USB Serial/JTAG |
| IO19 | `ESP_D+` | USB-C qua U7 | USB Serial/JTAG |

### 2.2. Chân strapping và nút nhấn

| Chân | Mạch | Ý nghĩa |
|---|---|---|
| **IO9** | R22 = 10 kΩ kéo lên, C28 = 100 nF, nút **SW4** xuống GND | IO9 = 0 lúc khởi động → **chế độ nạp** (download mode) |
| **IO8** | R19 = 10 kΩ kéo lên | Phải = 1 khi vào chế độ nạp qua UART. Không dùng làm GPIO |
| **IO2** | R20 = 10 kΩ kéo lên | Strapping, phải = 1. Không dùng làm GPIO |
| **EN** | R15 = 10 kΩ kéo lên, R16 = 100 Ω, C25 = 100 nF, nút **SW2** | Reset ESP32-C3 |

### 2.3. Chân còn trống
IO0, IO1, IO3, IO10, IO20 (U0RXD), IO21 (U0TXD) không nối. **UART0 không ra ngoài**, nên log phải đi qua USB (xem [mục 3](#3-cấu-hình-project-sdkconfigdefaults)).

Board **không có LED** riêng cho ESP32-C3. Muốn báo trạng thái thì in log hoặc gửi lệnh sang STM32 để nháy LED PC13.

---

## 3. Cấu hình project (`sdkconfig.defaults`)

ESP-IDF có hai file cấu hình:

| File | Nội dung | Commit? |
|---|---|---|
| `sdkconfig.defaults` | **Chỉ các mục project đổi khác mặc định** (vài dòng) | ✅ Có |
| `sdkconfig` | **Toàn bộ** cấu hình (vài nghìn dòng), sinh ra từ `sdkconfig.defaults` + mặc định của ESP-IDF | ❌ Không |

Không commit `sdkconfig` vì mỗi lần nâng cấp ESP-IDF hoặc chạy menuconfig, file này thay đổi hàng trăm dòng, gây conflict liên tục khi làm nhóm. `sdkconfig.defaults` ngắn, đọc là hiểu project đã đổi gì.

**Các mục đang đặt:**

| Mục | Giá trị | Tại sao |
|---|---|---|
| `CONFIG_IDF_TARGET` | `"esp32c3"` | Chip trên mạch. Không đặt thì ESP-IDF mặc định build cho ESP32 đời đầu (kiến trúc Xtensa, khác hoàn toàn) |
| `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` | `y` | Log (`printf`, `ESP_LOGI`) và nhập liệu đi qua **cổng USB-C**. Mặc định là UART0, nhưng UART0 (IO20, IO21) không nối ra ngoài, nên sẽ không thấy log |
| `CONFIG_ESPTOOLPY_FLASHSIZE_4MB` | `y` | Module **WROOM-02 có 4 MB flash**. Mặc định ESP-IDF chỉ dùng 2 MB, phí một nửa |
| `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_160` | `y` | Tốc độ tối đa của ESP32-C3. Nếu sau này cần tiết kiệm pin thì hạ xuống 80 MHz |

**Cách đổi cấu hình:**
1. `idf.py menuconfig` (hoặc biểu tượng ⚙️ trong VS Code), đổi và lưu.
2. Chạy `idf.py save-defconfig`. Lệnh này ghi lại **tất cả** mục khác mặc định vào `sdkconfig.defaults`. Mở file ra, giữ các dòng có ý nghĩa và thêm comment giải thích.
3. Cập nhật bảng ở trên.

> **Muốn áp dụng lại từ đầu** (ví dụ vừa sửa `sdkconfig.defaults` bằng tay): xoá file `sdkconfig` rồi build lại. `sdkconfig.defaults` chỉ được đọc khi tạo `sdkconfig` mới hoặc khi có mục mới chưa có giá trị.

---

## 4. Điều khiển STM32: NRST và BOOT0

### 4.1. NRST (IO6)
- Cấu hình **open-drain** (`GPIO_MODE_OUTPUT_OD`):
  - Ghi **0** → kéo NRST xuống → STM32 reset.
  - Ghi **1** → thả ra. R14 kéo NRST lên 3,3 V, STM32 chạy.
- **Tại sao open-drain, không dùng push-pull:** nút SW1 cũng nối NRST xuống GND, và STM32 tự kéo NRST xuống khi bị reset bên trong (watchdog, brown-out). Nếu ESP đẩy cứng NRST lên 3,3 V (push-pull), lúc SW1 được nhấn sẽ chập 3,3 V xuống GND qua chân ESP.

### 4.2. BOOT0 (IO7)
- **Bình thường để input, không pull** (thả nổi). Công tắc **SW3** qua **R21** quyết định mức của BOOT0.
- **Tại sao không giữ output 0:** nếu ESP luôn kéo BOOT0 xuống 0 thì gạt SW3 = 1 để cứu STM32 (xem tài liệu STM32) sẽ không còn tác dụng, vì chân ESP mạnh hơn điện trở R21.
- Chỉ chuyển IO7 sang **output 1** khi ESP cần đưa STM32 vào bootloader để nạp firmware.

### 4.3. Trình tự điều khiển

**Reset STM32 bình thường:**
```
NRST = 0  →  chờ 10 ms  →  NRST = 1
```

**Đưa STM32 vào bootloader ROM (để nạp firmware qua UART):**
```
IO7 = output, mức 1 (BOOT0 = 1)
NRST = 0  →  chờ 10 ms  →  NRST = 1
chờ khoảng 50 ms cho bootloader khởi động
đổi UART1 sang 8E1, gửi 0x7F để đồng bộ, nhận 0x79 (ACK)
... ghi flash theo giao thức AN3155 ...
IO7 = input (thả nổi) → reset STM32 lần nữa → STM32 chạy firmware mới
đổi UART1 về 8N1
```

---

## 5. UART1 – giao tiếp với STM32

| Thông số | Giá trị | Ghi chú |
|---|---|---|
| Bộ UART | **UART1** | UART0 dành cho console mặc định của ESP-IDF |
| Chân | TX = IO4, RX = IO5 | ESP32-C3 cho phép gán UART vào gần như chân nào cũng được (qua GPIO matrix) |
| Baud | **115200** | Phải khớp với USART1 của STM32 |
| Khung (chạy bình thường) | **8N1** | |
| Khung (nạp STM32) | **8E1** | Bootloader ROM của STM32 chỉ nhận parity chẵn |
| Bộ đệm nhận | 512 byte | `uart_driver_install(..., 256 * 2, ...)` |

**Hàm chính của ESP-IDF:**
```c
uart_driver_install(UART_NUM_1, rx_buf_size, tx_buf_size, queue_size, &queue, 0);
uart_param_config(UART_NUM_1, &cfg);
uart_set_pin(UART_NUM_1, GPIO_NUM_4, GPIO_NUM_5, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
uart_read_bytes(UART_NUM_1, buf, len, timeout_ticks);
uart_write_bytes(UART_NUM_1, data, len);
```

**Dự kiến:** định nghĩa khung gói tin trong `swarm_protocol.h` (byte đầu, loại gói, độ dài, dữ liệu, CRC). File nằm ở [`shared/swarm_protocol.h`](../../shared/swarm_protocol.h), STM32 và ESP32 cùng include một file.

---

## 6. Flash và phân vùng

| Thông số | Giá trị |
|---|---|
| Dung lượng flash | 4 MB |
| Chế độ flash | DIO, 80 MHz |
| Bảng phân vùng | Mặc định: *Single factory app, no OTA* |
| Phân vùng ứng dụng | 1 MB (0x100000) |
| Kích thước app hiện tại | 0x22E40 = **143 KB** (còn trống 86 %) |

Địa chỉ nạp: bootloader `0x0`, bảng phân vùng `0x8000`, ứng dụng `0x10000`.

> **Khi làm cập nhật qua Wi-Fi (OTA):** phải đổi sang bảng phân vùng có 2 vùng app (`Factory app, two OTA definitions` hoặc file `partitions.csv` tự viết). Với 4 MB flash, chia được 2 vùng app khoảng 1,5 MB mỗi vùng. Firmware STM32 (chưa tới 64 KB) cũng có thể lưu trong một vùng dữ liệu riêng, để ESP nạp lại cho STM32.

---

## 7. FreeRTOS trên ESP-IDF

ESP-IDF **luôn chạy FreeRTOS**, không có lựa chọn bare-metal như STM32.

| Điểm | ESP-IDF | STM32 (CubeMX, CMSIS-RTOS v2) |
|---|---|---|
| Hàm bắt đầu | `app_main()`, chạy **trong một task** sẵn có | `main()`, rồi `osKernelStart()` |
| **Đơn vị stack** | **byte** (`xTaskCreate(..., 4096, ...)` = 4 KB) | word (128 words = 512 byte) |
| Tick | 100 Hz mặc định (`CONFIG_FREERTOS_HZ`) | 1000 Hz |
| Đổi ms sang tick | `pdMS_TO_TICKS(ms)` | `osDelay(ms)` (tick = 1 ms) |
| Số nhân | 1 (ESP32-C3 là RISC-V đơn nhân) | 1 |

**Lưu ý:**
- `app_main` **được phép return**. Các task đã tạo vẫn tiếp tục chạy.
- Với tick 100 Hz, `vTaskDelay(pdMS_TO_TICKS(5))` làm tròn thành 0 tick. Nếu cần chu kỳ ngắn hơn 10 ms thì tăng `CONFIG_FREERTOS_HZ` lên 1000.
- Wi-Fi và ESP-NOW chạy trong các task riêng của ESP-IDF, có priority cao. Task của mình nên để priority **5 hoặc thấp hơn**, và không chiếm CPU liên tục (luôn có `vTaskDelay` hoặc lệnh chờ queue).

---

## 8. Cài đặt môi trường

Làm lần lượt trên **Ubuntu** (máy mới hoặc thành viên mới):

### 8.1. Gói hệ thống
```bash
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

### 8.2. Quyền dùng cổng USB
```bash
sudo usermod -aG dialout $USER
```
Sau đó **khởi động lại máy**. Kiểm tra bằng lệnh `groups`, kết quả phải có `dialout`.

> Chỉ đăng xuất thì **có thể không đủ**: nếu `systemd --user` của phiên cũ còn chạy, các ứng dụng mở lại vẫn mang danh sách nhóm cũ. Khởi động lại là chắc chắn nhất.

### 8.3. ESP-IDF v6.1 qua EIM
1. VS Code → cài extension **ESP-IDF** (Espressif Systems).
2. `Ctrl+Shift+P` → **ESP-IDF: Open ESP-IDF Installation Manager**.
3. Trả lời các câu hỏi của trình cài:

| Câu hỏi | Chọn | Lý do |
|---|---|---|
| Target platforms | Chỉ **esp32c3** | Chọn `all` thì phải tải thêm bộ compiler Xtensa, nặng gấp đôi |
| ESP-IDF version | **v6.1** | Cả nhóm phải cùng phiên bản |
| Nguồn tải ESP-IDF | **github.com** | Mirror Trung Quốc (`git.espressif.com.cn`) bị timeout khi truy cập từ Việt Nam |
| Nguồn tải công cụ | **github.com** | Như trên. Dự phòng: `dl.espressif.com/github_assets` |
| PyPI mirror | **pypi.org/simple** | Nhanh nhất (khoảng 140 ms) |
| Thư mục cài | Mặc định `~/.espressif` | Extension và tài liệu đều tìm ở đây |
| Công cụ thêm | Tích **cmake**, không tích qemu | CMake bản đi kèm (4.0.3) đã được Espressif kiểm tra với v6.1. QEMU chỉ dùng để giả lập, không cần |

4. `Ctrl+Shift+P` → **ESP-IDF: Select Current ESP-IDF Version** → **v6.1**.

### 8.4. Quyền udev cho OpenOCD (để debug)
```bash
sudo cp ~/.espressif/tools/openocd-esp32/v0.12.0-esp32-20260703/openocd-esp32/share/openocd/contrib/60-openocd.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger
```
Tên thư mục phiên bản OpenOCD có thể khác. Xem bằng lệnh `ls ~/.espressif/tools/openocd-esp32`.

### 8.5. Dùng ESP-IDF trong terminal
```bash
source ~/.espressif/tools/activate_idf_v6.1.fish    # shell fish
source ~/.espressif/tools/activate_idf_v6.1.sh      # shell bash/zsh
```
Chỉ chạy lệnh này trong terminal dùng cho ESP. Nó thêm cmake và compiler riêng của ESP-IDF vào PATH, có thể lẫn với công cụ STM32.

---

## 9. Build, nạp, debug

| Việc | VS Code (thanh trạng thái) | Dòng lệnh |
|---|---|---|
| Chọn cổng | 🔌 → `/dev/ttyACM0` | `-p /dev/ttyACM0` |
| Cấu hình | ⚙️ SDK Configuration Editor | `idf.py menuconfig` |
| Build | 🔧 | `idf.py build` |
| Nạp | ⚡ (Flash method: **UART**) | `idf.py -p /dev/ttyACM0 flash` |
| Xem log | 🖥️ | `idf.py -p /dev/ttyACM0 monitor` (thoát: `Ctrl+]`) |
| Cả ba | 🔥 | `idf.py -p /dev/ttyACM0 flash monitor` |
| Debug | `F5` | OpenOCD config: `board/esp32c3-builtin.cfg` |

**Nạp không được:** giữ **SW4 (BOOT/IO9)** → nhấn và thả **SW2 (RESET/EN)** → thả SW4 → nạp lại. Chip vào chế độ nạp và chờ ở đó.

**Debug:** ESP32-C3 có sẵn bộ USB-JTAG bên trong, nên chỉ cần cáp USB-C, không cần mạch nạp ngoài. Đặt breakpoint, nhấn F5, dùng F10 / F11 để chạy từng dòng.

---

## 10. Các sự cố đã gặp và cách sửa

| # | Hiện tượng | Nguyên nhân | Cách sửa |
|---|---|---|---|
| 1 | `groups` không có `dialout` dù đã chạy `usermod` và đăng xuất | `systemd --user` của phiên cũ (từ sáng) vẫn chạy, nên app mở lại kế thừa danh sách nhóm cũ | Khởi động lại máy |
| 2 | EIM báo WARN `Mirror ping failed … git.espressif.com.cn` / `dl.espressif.cn` / `aliyun 403` | Mirror ở Trung Quốc không truy cập được từ Việt Nam | Bỏ qua, chọn github.com và pypi.org |
| 3 | `.gitignore` của mẫu project có comment nằm cùng dòng (`*.exe # ...`) | Git **không** hỗ trợ comment cuối dòng, nên cả dòng bị coi là một mẫu tên file | Viết lại `.gitignore`, comment đặt trên dòng riêng |
| 4 | `.vscode/settings.json` chứa đường dẫn `/home/<user>/.espressif/...` | Extension ghi đường dẫn ESP-IDF của máy hiện tại | Không commit file này; mỗi người tự chọn phiên bản bằng *Select Current ESP-IDF Version* |

---

## 11. Lịch sử thay đổi

| Ngày | Người | Thay đổi |
|---|---|---|
| 27/10/2026 | Khoi281248 | Cài ESP-IDF v6.1 bằng EIM 0.20.1 (chỉ esp32c3), thêm udev OpenOCD, thêm user vào nhóm dialout |
| 27/10/2026 | Khoi281248 | Tạo project `ESP32_Swarm_Robot` từ mẫu `sample_project`, board *ESP32-C3 chip (via builtin USB-JTAG)* |
| 27/10/2026 | Khoi281248 | Thêm `sdkconfig.defaults` (esp32c3, console USB Serial/JTAG, flash 4 MB, CPU 160 MHz); viết lại `.gitignore`; thêm README và tài liệu này |

---

## 12. Việc còn lại

- [ ] Cầu nối UART: đọc dữ liệu STM32 gửi sang, in lên cổng USB
- [ ] Hàm `stm32_reset()`, `stm32_enter_bootloader()` (NRST, BOOT0)
- [ ] `swarm_protocol.h`: khung gói tin dùng chung với STM32
- [ ] ESP-NOW: gửi và nhận trạng thái giữa các robot
- [ ] Nạp firmware STM32 qua bootloader UART (AN3155, 8E1)
- [ ] Bảng phân vùng có OTA, cập nhật firmware qua Wi-Fi
- [ ] Đo dòng tiêu thụ, cân nhắc hạ CPU xuống 80 MHz và bật chế độ tiết kiệm năng lượng của Wi-Fi
