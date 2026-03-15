#pragma once

#include <vector>
#include <queue>
#include <iostream>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <unordered_set>

// JM 側の前方宣言
extern "C" {
    typedef struct decodedpic_t DecodedPicList;  
    typedef struct storable_picture StorablePicture;  
}

// 基本データ型
typedef unsigned char   Byte;
typedef unsigned char   PIXEL;
typedef unsigned char   Bool;
typedef unsigned char   UInt_8;
typedef unsigned short  UInt16;
typedef unsigned long   UInt32;
typedef char            SInt_8;
typedef short           SInt16;
typedef long            SInt32;

// デコーダ設定パラメータ
typedef struct {
    UInt32  bs_buf_size;
    UInt16  disp_buf_num;
    UInt16  disp_format;
    UInt16  disp_max_width;
    UInt16  disp_max_height;
    UInt16  target_profile;
    UInt16  target_level;
} DECPARAM_AVC;

// タイミング情報
typedef struct {
    UInt32  input_pts;
    int     m_ts_success;
    double  time;
    int     m_timinginfo_success;
    UInt32  time_scale;
    UInt32  num_units_in_tick;
    UInt32  low_delay_hrd_flag;
    UInt32  BitRate;
    UInt32  cbr_flag;
    UInt32  cpb_removal_delay;
    UInt32  dpb_output_delay;
    int     is_first_au_of_buff_period;
    UInt32  initial_cpb_removal_delay;
    UInt32  initial_cpb_removal_delay_offset;
} TIMEINFO_AVC;

// 表示画像メタ情報
typedef struct {
    TIMEINFO_AVC  pts;
    UInt16        pic_width;
    UInt16        pic_height;
    UInt16        pic_type;
    UInt16        bit_depth;
} PICMETAINFO_AVC;

/**
 * @brief H.264 デコーダクラス
 */
class Avcdec
{
    public:

        /**
         * @brief デコーダインスタンスを生成する
         * @param INPUT_PARAM デコーダ設定パラメータ
         */
        Avcdec(DECPARAM_AVC *INPUT_PARAM);
        
        /**
         * @brief デコーダインスタンスを破棄する
         */
        ~Avcdec();

        /**
         * @brief ビデオデコードエンジンを開始する
         * @param PLAY_MODE 再生モード（0: 通常再生  , 0以外: Reserved ）
         * @param POST_PROCESS ポスト処理指定 （0: ポスト処理なし  , 0以外: Reserved ）
         */
        void vdec_start(UInt16 PLAY_MODE, UInt16 POST_PROCESS);
        
        /**
         * @brief ビデオデコードエンジンを停止する
         * @return 常に 0 を返す
         */
        int vdec_stop();
        
        /**
         * @brief ポスト処理を指定する
         * @param TYPE ポスト処理指定 （0: ポスト処理なし , 0以外: Reserved ）
         */
        void vdec_postprocess(UInt16 TYPE);
        
        /**
         * @brief H.264 ビットストリームを入力する
         * @param PAYLOAD 入力ビットストリームデータ 
         * @param LENGTH PAYLOADデータ長(byte) 
         * @param END_OF_AU アクセスユニット末尾通知 （0: 継続 , 1: PAYLOADがAU末尾である）
         * @param PTS プレゼンテーションタイムスタンプ
         * @param ERR_FLAG PAYLOADのエラー有無フラグ
         * @param ERR_SN_SKIP シーケンス番号のスキップ数
         * @return ストリームバッファに書き込まれたサイズ 、空きがない場合は unsigned int の -1
         */
        unsigned int vdec_put_bs(
            Byte* PAYLOAD,
            UInt32 LENGTH,
            UInt16 END_OF_AU,
            UInt32 PTS,
            UInt16 ERR_FLAG,
            UInt32 ERR_SN_SKIP
        );
        
        /**
         * @brief 表示画像を取得する
         * @param PIC_METAINFO 画像表示情報 
         * @return 表示画像出力画面先頭アドレス 
         */
        Byte* vdec_get_picture(PICMETAINFO_AVC* PIC_METAINFO);
        
        /**
         * @brief ステータス情報を取得する
         * @param DEC_STATUS デコーダステータス値
         * @param DISP_STATUS 表示画像レディステータス値 
         * @param ERR_STATUS エラーステータス値 
         * @return 0正常 
         */
        unsigned int vdec_get_status(
            UInt16* DEC_STATUS,
            UInt16* DISP_STATUS,
            UInt16* ERR_STATUS
        );
        
        /**
         * @brief 表示画像完了通知ハンドルを取得する 
         * @return DecodedHandle へのポインタ（void*）
         */
        void* vdec_get_DecodedHandle();
        
        /**
         * @brief 指定する表示画像バッファ面に対する書き込み可を指示する 
         * @param PIC_ADDR vdec_get_pictureで取得した表示画像出力画面先頭アドレス 
         */
        void vdec_release_pic_buffer(Byte* PIC_ADDR);
        
        /**
         * @brief YUV420画像からRGB画像へ変換する
         * @param Mode RGB画像モード(0: TOPDOWN, 1: BOTTOMUP) 
         * @param iBGR RGB画像格納先アドレス 
         * @param iYUV YUV画像格納先アドレス 
         * @param width 画像サイズ(幅) 
         * @param height 画像サイズ(高さ) 
         * @return 変換後の画像サイズ(異常終了の場合は-1) 
         */
        int vdec_YUV420toRGB24(
            int Mode,
            unsigned char* iBGR,
            unsigned char* iYUV,
            int width,
            int height
        );
        
        /**
         * @brief YUV420画像からRGB画像への変換する(高速化) 
         * @param y YUV輝度情報 
         * @param u YUV色差情報 Cb 
         * @param v YUV色差情報 Cr 
         * @param rgb RGB画像格納先アドレス 
         * @param width 画像サイズ(幅) 
         * @param height 画像サイズ(高さ) 
         */
        void vdec_YUV420toRGB24_2(
            unsigned char* y,
            unsigned char* u,
            unsigned char* v,
            unsigned char* rgb,
            int width,
            int height
        );
        
        /**
         * @brief YUV420→RGB24変換(BGR順番でデータ取得DirectDraw用) 
         * @param Mode RGB画像モード(0: TOPDOWN, 1: BOTTOMUP) 
         * @param iBGR RGB画像格納先アドレス 
         * @param iYUV YUV画像格納先アドレス 
         * @param width 画像サイズ(幅) 
         * @param height 画像サイズ(高さ) 
         * @return 出力バイト数
         */
        int YUV420toRGB24_DX(
            int Mode,
            unsigned char* iBGR,
            unsigned char* iYUV,
            int width,
            int height
        );

        /**
         * @brief デコード完了通知用の同期ハンドル
         */
        struct DecodedHandle {
            std::mutex mutex;
            std::condition_variable cv;
            bool signaled = false;
        };

    private:
        // 設定情報
        DECPARAM_AVC m_config;
        
        // ストリームバッファ（メモリ入力）
        Byte* m_streamBuffer;  // 確保済みメモリ先頭
        UInt32 m_streamCapacity; // 格納可能な最大バイト数
        UInt32 m_streamSize; // 現在保持しているバイト数

        // 表示画像バッファ構造体
        struct PictureBuffer {
            Byte* data;
            int width;
            int height;
            bool locked;
            int poc;
        };
        
        PictureBuffer* m_pictureBuffers;  // 表示画像バッファ配列
        UInt16 m_bufferCount;             // 表示画像バッファ数（disp_buf_num）
        
        // デコーダ状態
        bool m_started;
        bool m_postprocess_enabled;
        
        // 出力フレーム保持キュー要素
        struct QueuedFrame {
            Byte* data;
            int width;
            int height;
            int poc;
            PICMETAINFO_AVC metadata;
        };
        
        std::queue<QueuedFrame> m_frameQueue;
        std::mutex m_frameQueueMutex;
        std::mutex m_allocatedFrameMutex;
        std::unordered_set<Byte*> m_allocatedQueuedFrames;
        int m_frameCount;
        DecodedHandle m_decodedHandle;
        
        // 既定パラメータで JM デコーダを初期化する
        void InitJMDecoder();

        // 現在のバッファ済みビットストリームをデコードする
        void DecodeBuffer();

        // 新規入力分の空き容量があるか確認する
        bool CheckBufferSpace(UInt32 needed_bytes);

        // Access Unit (AU) 終端をデコーダへ通知する
        void HandleEndOfAU();

        // デコード済み画像を表示キューへ追加する
        void QueueFrameForDisplay(PictureBuffer* buffer);

        // 利用可能な表示バッファを取得する
        PictureBuffer* GetAvailableBuffer();    

        // JM の内部画像リストから1枚分を取り出して処理する
        void ProcessDecodedPicture(DecodedPicList *pPic);
};