# Swarm Robot – Firmware

Firmware cho robot bầy đàn (swarm robot) hai vi điều khiển trên cùng một mạch:

| Thư mục | Chip | Vai trò | Công cụ |
|---|---|---|---|
| [`stm32/`](stm32/README.md) | **STM32F103C8T6** | Điều khiển thời gian thực: 2 động cơ (PWM + encoder), IMU ICM-42670-P, đo pin | STM32CubeMX 6.18.1, CMake, FreeRTOS |
| [`esp32/`](esp32/README.md) | **ESP32-C3-WROOM-02** | Truyền thông: Wi-Fi / ESP-NOW giữa các robot, nạp firmware cho STM32 | ESP-IDF v6.1 |
| [`shared/`](shared/swarm_protocol.h) | – | Giao thức UART giữa hai chip, **cả hai firmware cùng include** | – |

> 🔌 Thiết kế mạch (KiCad 10: sơ đồ nguyên lý, PCB, BOM, datasheet): [Swarm_Robot_Hardware](https://github.com/Khoi281248/Swarm_Robot_Hardware)

```
   Robot khác / máy chủ
          │  Wi-Fi, ESP-NOW
   ┌──────┴───────┐   UART 115200   ┌──────────────────┐   PWM   ┌──────────┐
   │  ESP32-C3    │ ◄─────────────► │  STM32F103C8T6   │ ──────► │ DRV8833  │ ──► 2 động cơ
   │(truyền thông)│ NRST, BOOT0 ──► │ (điều khiển)     │ ◄────── │ encoder  │
   └──────────────┘                 └────────┬─────────┘         └──────────┘
          │ USB-C                            │ I2C
         PC                            ICM-42670-P (IMU)
```

## Cấu trúc repo

```
Swarm_Robot_Firmware/
├── README.md              Trang này
├── LICENSE                MIT
├── shared/
│   └── swarm_protocol.h   Giao thức UART STM32 <-> ESP32 (dùng chung)
├── stm32/                 Project STM32 (CubeMX + CMake)
│   ├── README.md          Build, nạp, sơ đồ chân STM32
│   └── docs/CAU_HINH.md   Nhật ký cấu hình STM32 và lý do
└── esp32/                 Project ESP-IDF
    ├── README.md          Build, nạp, sơ đồ chân ESP32-C3
    └── docs/CAU_HINH.md   Nhật ký cấu hình ESP32 và lý do
```

## Bắt đầu nhanh

```bash
gh repo clone Khoi281248/Swarm_Robot_Firmware
```

| Muốn làm | Mở thư mục | Đọc |
|---|---|---|
| Firmware STM32 | `stm32/` bằng STM32CubeIDE 2.2 (*Import → Existing Projects into Workspace*) hoặc VS Code | [stm32/README.md](stm32/README.md) |
| Firmware ESP32 | `esp32/` bằng VS Code + extension ESP-IDF | [esp32/README.md](esp32/README.md) |

> Mở **thư mục con** (`stm32/` hoặc `esp32/`) trong IDE, không mở thư mục gốc. Mỗi firmware có hệ build riêng.

## Phiên bản công cụ (cả nhóm dùng giống nhau)

| Công cụ | Phiên bản |
|---|---|
| STM32CubeMX | 6.18.1, gói FW_F1 V1.8.7 |
| GNU Tools for STM32 | 14.3.1 |
| ESP-IDF | v6.1 |
| riscv32-esp-elf-gcc | 15.2.0 |

## Làm việc nhóm

1. Mỗi việc làm trên một nhánh riêng, gộp vào `main` qua Pull Request:
   ```bash
   git switch main && git pull
   git switch -c ten-nhanh
   # ... sua code, build thu ...
   git add . && git commit -m "stm32: mo ta thay doi"
   git push -u origin ten-nhanh
   gh pr create --fill
   ```
2. Commit message bắt đầu bằng phần bị sửa: `stm32: ...`, `esp32: ...`, `shared: ...`, `docs: ...`.
3. **Sửa giao thức (`shared/`) thì sửa code cả hai firmware trong cùng một Pull Request**, để hai chip luôn hiểu nhau.
4. Chỉ một người phụ trách file `stm32/STM32_Swarm_Robot.ioc` (CubeMX). Cấu hình ESP32 đổi qua `esp32/sdkconfig.defaults`.
5. Thay đổi cấu hình thì cập nhật `docs/CAU_HINH.md` của phần tương ứng trong cùng commit.
6. Gắn tag phiên bản cho cả robot khi chạy ổn định: `git tag v0.1 && git push --tags`.

## Tiến độ

| Phần | Xong | Đang làm / tiếp theo |
|---|---|---|
| STM32 | Cấu hình CubeMX, FreeRTOS, build trên CubeIDE và VS Code | Test phần cứng, PID, đọc IMU |
| ESP32 | Cài ESP-IDF v6.1, tạo project, cấu hình chip | Cầu nối UART, điều khiển NRST/BOOT0 |
| Giao thức | Khung file `shared/swarm_protocol.h` | Định nghĩa gói tin |

## Giấy phép

Code của nhóm phát hành theo giấy phép [MIT](LICENSE). Thư viện bên thứ ba (STM32 HAL/CMSIS, FreeRTOS, ESP-IDF) giữ giấy phép gốc.
