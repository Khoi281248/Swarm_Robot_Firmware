# ESP32 Swarm Robot – Firmware

Firmware cho module **ESP32-C3-WROOM-02** trên mạch robot bầy đàn (swarm robot), viết bằng **ESP-IDF v6.1**.

ESP32-C3 là **khối truyền thông** của robot:
- Liên lạc không dây với các robot khác và máy chủ (Wi-Fi / ESP-NOW).
- Chuyển lệnh điều hướng xuống STM32 và gửi trạng thái robot (vận tốc, hướng, vị trí) ngược lên, qua UART.
- Điều khiển chân **NRST** và **BOOT0** của STM32, để nạp firmware cho STM32 không cần dây.

> 📖 Chi tiết cấu hình và lý do: [docs/CAU_HINH.md](docs/CAU_HINH.md)
> 🔗 Firmware STM32 (điều khiển động cơ, cảm biến): [../stm32](../stm32/README.md)

```
  Robot khác / máy chủ
         │  Wi-Fi, ESP-NOW
  ┌──────┴───────┐   UART1 115200   ┌──────────────────┐
  │  ESP32-C3    │ IO4 ───────────► │ PA10             │
  │(truyền thông)│ IO5 ◄─────────── │ PA9   STM32F103  │
  │              │ IO6 ───────────► │ NRST (điều khiển)│
  │              │ IO7 ───────────► │ BOOT0            │
  └──────┬───────┘                  └──────────────────┘
         │ USB-C (IO18, IO19): nạp code, log, debug JTAG
        PC
```

## Sơ đồ chân

| Chân ESP32-C3 | Chức năng | Nối tới | Ghi chú |
|---|---|---|---|
| IO4 | UART1 TX | STM32 PA10 (RX) | 115200 8N1 |
| IO5 | UART1 RX | STM32 PA9 (TX) | |
| IO6 | Reset STM32 | STM32 NRST | Open-drain; mạch có R14 kéo lên |
| IO7 | Chọn chế độ khởi động STM32 | STM32 BOOT0 | Bình thường để input (thả nổi) để công tắc SW3 quyết định |
| IO18, IO19 | USB D−, D+ | Cổng USB-C | Nạp code, console, debug JTAG |
| IO9 | Nút BOOT (SW4) | – | Giữ khi reset để vào chế độ nạp |
| EN | Nút RESET (SW2) | – | |
| IO2, IO8 | Strapping | Kéo lên 3,3 V | Không dùng làm GPIO |
| IO0, IO1, IO3, IO10, IO20, IO21 | – | Không nối | Còn trống |

Board không có LED riêng cho ESP32-C3.

## Công cụ cần cài

| Công cụ | Phiên bản |
|---|---|
| ESP-IDF | **v6.1** (cài bằng ESP-IDF Installation Manager – EIM) |
| VS Code + extension *ESP-IDF* (Espressif) | 2.3 trở lên |
| Trình biên dịch | `riscv32-esp-elf-gcc` 15.2.0 (EIM tự cài) |

Trên Linux cần thêm:
```bash
sudo usermod -aG dialout $USER     # quyen mo /dev/ttyACM0, phai dang nhap lai (hoac khoi dong lai)
```
Và luật udev cho OpenOCD để debug (xem [docs/CAU_HINH.md](docs/CAU_HINH.md#8-cài-đặt-môi-trường)).

## Build, nạp, xem log

### VS Code
1. Mở thư mục này. Lần đầu: `Ctrl+Shift+P` → **ESP-IDF: Select Current ESP-IDF Version** → v6.1.
2. Cắm board qua USB-C, chọn cổng **/dev/ttyACM0** ở thanh trạng thái.
3. Bấm 🔥 **Build, Flash and Monitor**. Thoát monitor: `Ctrl+]`.

### Dòng lệnh
```bash
source ~/.espressif/tools/activate_idf_v6.1.sh   # fish: activate_idf_v6.1.fish
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Lần build đầu, ESP-IDF tạo file `sdkconfig` từ [`sdkconfig.defaults`](sdkconfig.defaults) (chip esp32c3, console USB, flash 4 MB, CPU 160 MHz).

**Không nạp được** (`Failed to connect`): giữ **SW4 (BOOT)**, nhấn và thả **SW2 (RESET)**, thả SW4, rồi nạp lại.

## Cấu trúc thư mục

```
ESP32_Swarm_Robot/
├── CMakeLists.txt        Khai báo project ESP-IDF
├── sdkconfig.defaults    Cấu hình riêng của project (có commit)
├── main/
│   ├── CMakeLists.txt    Danh sách file .c của thành phần main
│   └── main.c            app_main()
├── docs/
│   └── CAU_HINH.md       Nhật ký cấu hình và lý do
├── .vscode/              launch.json, c_cpp_properties.json (settings.json không commit)
└── .devcontainer/        Môi trường Docker (tùy chọn)
```

## Làm việc nhóm

1. **Đổi cấu hình bằng `idf.py menuconfig`**, rồi chép các mục đã đổi vào `sdkconfig.defaults`. File `sdkconfig` không được commit.
2. Mỗi việc làm trên một nhánh riêng, gộp vào `main` qua Pull Request:
   ```bash
   git switch main && git pull
   git switch -c ten-nhanh
   # ... sua code, build thu ...
   git add . && git commit -m "Mo ta thay doi"
   git push -u origin ten-nhanh
   gh pr create --fill
   ```
3. Giao thức UART với STM32 nằm trong [`../shared/swarm_protocol.h`](../shared/swarm_protocol.h), dùng chung cho cả hai firmware. Sửa giao thức thì sửa code cả hai bên trong **cùng một Pull Request**.

## Tiến độ

- [x] Cài ESP-IDF v6.1, tạo project, cấu hình chip, console, flash
- [ ] Cầu nối UART: in dữ liệu STM32 gửi sang lên cổng USB
- [ ] Điều khiển NRST, BOOT0 của STM32
- [ ] Giao thức gói tin UART với STM32 (`swarm_protocol.h`)
- [ ] Truyền thông giữa các robot qua ESP-NOW
- [ ] Nạp firmware STM32 qua ESP32-C3 (bootloader UART 8E1)
- [ ] Cập nhật firmware ESP32 qua Wi-Fi (OTA)

## Giấy phép

[MIT](../LICENSE)
