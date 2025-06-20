#pragma once

#ifndef F455A1B7_80E2_4672_959C_840DEE6F1F26
#define F455A1B7_80E2_4672_959C_840DEE6F1F26
#define LGFX_USE_V1

#include <LovyanGFX.hpp>

namespace BTLogger {
namespace Hardware {

#define LCD_MOSI 13
#define LCD_MISO 12
#define LCD_SCK 14
#define LCD_CS 15
#define LCD_RST -1
#define LCD_DC 2
#define LCD_BL 21

#define TOUCH_MOSI 32
#define TOUCH_MISO 39
#define TOUCH_SCK 25
#define TOUCH_CS 33
#define TOUCH_IRQ 36

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Touch_XPT2046 _touch_instance;
    lgfx::Light_PWM _light_instance;

   public:
    LGFX(void) {
        {                                       // バス制御の設定を行います。
            auto cfg = _bus_instance.config();  // バス設定用の構造体を取得します。

            // SPIバスの設定
            cfg.spi_host = HSPI_HOST;   // 使用するSPIを選択  (VSPI_HOST or HSPI_HOST)
            cfg.spi_mode = 0;           // SPI通信モードを設定 (0 ~ 3)
            cfg.freq_write = 40000000;  // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
            cfg.freq_read = 16000000;   // 受信時のSPIクロック
            cfg.spi_3wire = true;       // 受信をMOSIピンで行う場合はtrueを設定
            cfg.use_lock = true;        // トランザクションロックを使用する場合はtrueを設定
            cfg.dma_channel = 1;        // Set the DMA channel (1 or 2. 0=disable)   使用するDMAチャンネルを設定 (0=DMA不使用)
            cfg.pin_sclk = LCD_SCK;     // SPIのSCLKピン番号を設定
            cfg.pin_mosi = LCD_MOSI;    // SPIのMOSIピン番号を設定
            cfg.pin_miso = LCD_MISO;    // SPIのMISOピン番号を設定 (-1 = disable)
            cfg.pin_dc = LCD_DC;        // SPIのD/Cピン番号を設定  (-1 = disable)
                                        // SDカードと共通のSPIバスを使う場合、MISOは省略せず必ず設定してください。

            _bus_instance.config(cfg);               // 設定値をバスに反映します。
            _panel_instance.setBus(&_bus_instance);  // バスをパネルにセットします。
        }

        {                                         // 表示パネル制御の設定を行います。
            auto cfg = _panel_instance.config();  // 表示パネル設定用の構造体を取得します。

            cfg.pin_cs = LCD_CS;    // CSが接続されているピン番号   (-1 = disable)
            cfg.pin_rst = LCD_RST;  // RSTが接続されているピン番号  (-1 = disable)
            cfg.pin_busy = -1;      // BUSYが接続されているピン番号 (-1 = disable)

            // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。

            cfg.memory_width = 240;    // ドライバICがサポートしている最大の幅
            cfg.memory_height = 320;   // ドライバICがサポートしている最大の高さ
            cfg.panel_width = 240;     // 実際に表示可能な幅
            cfg.panel_height = 320;    // 実際に表示可能な高さ
            cfg.offset_x = 0;          // パネルのX方向オフセット量
            cfg.offset_y = 0;          // パネルのY方向オフセット量
            cfg.offset_rotation = 0;   // 回転方向の値のオフセット 0~7 (4~7は上下反転)
            cfg.dummy_read_pixel = 8;  // ピクセル読出し前のダミーリードのビット数
            cfg.dummy_read_bits = 1;   // ピクセル以外のデータ読出し前のダミーリードのビット数
            cfg.readable = true;       // データ読出しが可能な場合 trueに設定
            cfg.invert = false;        // パネルの明暗が反転してしまう場合 trueに設定
            cfg.rgb_order = false;     // パネルの赤と青が入れ替わってしまう場合 trueに設定
            cfg.dlen_16bit = false;    // データ長を16bit単位で送信するパネルの場合 trueに設定
            cfg.bus_shared = true;     // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)

            _panel_instance.config(cfg);
        }

        {                                         // バックライト制御の設定を行います。（必要なければ削除）
            auto cfg = _light_instance.config();  // バックライト設定用の構造体を取得します。

            cfg.pin_bl = LCD_BL;  // バックライトが接続されているピン番号
            cfg.invert = false;   // バックライトの輝度を反転させる場合 true
            cfg.freq = 44100;     // バックライトのPWM周波数
            cfg.pwm_channel = 7;  // 使用するPWMのチャンネル番号

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
        }

#ifndef USE_BITBANG_TOUCH
        {  // タッチスクリーン制御の設定を行います。（必要なければ削除）
            auto cfg = _touch_instance.config();

            cfg.x_min = 0;            // タッチスクリーンから得られる最小のX値(生の値)
            cfg.x_max = 239;          // タッチスクリーンから得られる最大のX値(生の値)
            cfg.y_min = 0;            // タッチスクリーンから得られる最小のY値(生の値)
            cfg.y_max = 319;          // タッチスクリーンから得られる最大のY値(生の値)
            cfg.pin_int = TOUCH_IRQ;  // INTが接続されているピン番号
            cfg.bus_shared = true;    // 画面と共通のバスを使用している場合 trueを設定
            cfg.offset_rotation = 0;  // 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定

            // SPI接続の場合
            cfg.spi_host = VSPI_HOST;   // 使用するSPIを選択 (HSPI_HOST or VSPI_HOST)
            cfg.freq = 1000000;         // SPIクロックを設定
            cfg.pin_sclk = TOUCH_SCK;   // SCLKが接続されているピン番号
            cfg.pin_mosi = TOUCH_MOSI;  // MOSIが接続されているピン番号
            cfg.pin_miso = TOUCH_MISO;  // MISOが接続されているピン番号
            cfg.pin_cs = TOUCH_CS;      //   CSが接続されているピン番号

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);  // タッチスクリーンをパネルにセットします。
        }
#endif

        setPanel(&_panel_instance);  // 使用するパネルをセットします。
    }
};

}  // namespace Hardware
}  // namespace BTLogger

#endif /* F455A1B7_80E2_4672_959C_840DEE6F1F26 */