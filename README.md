# STM32_Health_Monitor
Health monitoring system using STM32F103C8T6 and FreeRTOS

## MicroSD

Driver SPI1 (`spi1.c`), MicroSD (`sd_spi.c`) và RTC (`sd_rtc.c`) truy cập
thanh ghi STM32F103C8T6 qua CMSIS. Dùng thẻ có phân vùng FAT16/FAT32
và module có tín hiệu logic 3,3 V. Nối nguồn và GND theo thông số của module.

| MicroSD | STM32F103C8T6 |
| --- | --- |
| CS | PA4 |
| SCK | PA5 |
| MISO | PA6 |
| MOSI | PA7 |

Build bằng `pio run` và nạp qua ST-Link bằng `pio run -t upload`. `main.c` tạo `Logging_Task` trước khi khởi động scheduler.
LED trên chân `PC13` (tích cực mức thấp trên Blue Pill) sẽ sáng khi STM32 nhận diện và mount thành công thẻ MicroSD, và tắt khi chưa có thẻ hoặc xảy ra lỗi thẻ.
Từ task cảm biến, gửi mẫu bằng `SD_Task_Submit(&sample, wait_ticks)`. Ví dụ,
SpO₂ 98,50% được gửi dưới dạng `9850` và nhiệt độ 36,75 °C dưới dạng `3675`.
`pdPASS` xác nhận đã đưa mẫu vào queue. Nếu queue đầy quá thời gian chờ,
hàm trả về lỗi. Gán `signal_status` theo `sd_task.h` (0 hợp lệ, 1 mất tiếp xúc,
2 không ổn định, 3 lỗi cảm biến). `SD_Task_LastResult()` trả về kết quả FatFs
gần nhất. `SD_Task_VerifiedRows()` đếm các dòng đã ghi và đọc lại thành công.

Để đặt giờ, gọi `SD_RTC_SetUnixTime(seconds)` từ một task sau khi scheduler chạy.
Hàm nhận Unix time UTC và dùng thạch anh LSE 32,768 kHz. Nếu bộ đếm RTC chưa có
giờ hợp lệ, cột `timestamp_utc` sẽ trống.

Task ghi nối tiếp vào `VITALS.CSV`, gom lô tối đa 8 mẫu hoặc tự ghi sau mỗi 2 giây.
Sau khi đóng tệp, task mở lại bằng FatFs và so sánh nội dung vừa ghi.
Nếu lỗi thẻ, task thử mount lại sau một giây. Các mẫu chưa bắt đầu ghi được giữ
để thử lại trong giới hạn queue. Nếu thẻ lỗi giữa lúc ghi, lô đang ghi có thể
thiếu dữ liệu. Chỉ rút thẻ sau khi ngừng gửi mẫu và task đã ghi xong.
