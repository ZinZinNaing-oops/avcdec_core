#include "AvcDecoder.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <mutex>

// JM 側の外部関数・外部変数
extern "C" {
    #include "../../JM/ldecod/inc/global.h"
    #include "../../JM/ldecod/inc/annexb.h"
     #include "../../JM/ldecod/inc/image.h" 
    
    // メモリ入力モード用グローバル
    extern unsigned char* g_memory_buffer;
    extern int g_memory_size;
    extern int g_memory_pos;
    extern int g_memory_mode;
    
    // メモリ入力モード用関数
    extern void AnnexBMemoryModeInit(unsigned char *buffer, int size);
    extern void AnnexBMemoryModeReset(void);
    extern void AnnexBMemoryModeExit(void);
    
    // JM 構造体
    extern DecoderParams *p_Dec;
    
    // JM 関数
    extern int OpenDecoder(InputParameters *p_Inp);
    extern int DecodeOneFrame(DecodedPicList **ppDecPicList);
    extern int FinitDecoder(DecodedPicList **ppDecPicList);
    extern int CloseDecoder(void);
}

#define DEC_OPEN_NOERR  0
#define DEC_SUCCEED 0
#define DEC_EOS 1

Avcdec::Avcdec(DECPARAM_AVC *INPUT_PARAM)
    : m_streamBuffer(nullptr),
      m_streamCapacity(0),
      m_streamSize(0),
      m_started(false),
      m_postprocess_enabled(false)
{   
    // 設定をコピー
    m_config = *INPUT_PARAM;
    
    // ストリームバッファを確保
    m_streamCapacity = m_config.bs_buf_size;
    m_streamBuffer = new Byte[m_streamCapacity];
    memset(m_streamBuffer, 0, m_streamCapacity);
    m_streamSize = 0;  

    // 表示用ピクチャバッファを確保
    m_bufferCount = m_config.disp_buf_num;
    m_pictureBuffers = new PictureBuffer[m_bufferCount];
    
    // 1フレーム当たりのサイズを計算（YUV420）
    int pic_width = m_config.disp_max_width;
    int pic_height = m_config.disp_max_height;
    int ySize = pic_width * pic_height;
    int uvSize = ySize / 4;
    int pic_size = ySize + 2 * uvSize;  // YUV420 size
    
    // 各表示バッファを初期化
    for (int i = 0; i < m_bufferCount; i++)
    {
        m_pictureBuffers[i].data = new Byte[pic_size];
        m_pictureBuffers[i].width = pic_width;
        m_pictureBuffers[i].height = pic_height;
        m_pictureBuffers[i].locked = false; 
        memset(m_pictureBuffers[i].data, 0, pic_size);
    }
}

Avcdec::~Avcdec()
{  
    // ストリームバッファを解放
    if (m_streamBuffer)
    {
        delete[] m_streamBuffer;
        m_streamBuffer = nullptr;
    }
    
    // 表示バッファを解放（確保済み時のみ）
    if (m_pictureBuffers && m_bufferCount > 0)
    {
        for (int i = 0; i < m_bufferCount; i++)
        {
            if (m_pictureBuffers[i].data != nullptr)
            {
                delete[] m_pictureBuffers[i].data;
                m_pictureBuffers[i].data = nullptr;
            }
        }
        delete[] m_pictureBuffers;
        m_pictureBuffers = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(m_allocatedFrameMutex);
        for (Byte* frameData : m_allocatedQueuedFrames)
        {
            delete[] frameData;
        }
        m_allocatedQueuedFrames.clear();
    }
    
    // キューを空にする
    while (!m_frameQueue.empty())
        m_frameQueue.pop();
}

void Avcdec::vdec_start(UInt16 PLAY_MODE, UInt16 POST_PROCESS)
{  
    if(m_started)
        return;

    m_started = true;
    m_postprocess_enabled = (POST_PROCESS != 0);
    
    // JM をメモリ入力モードで初期化
    AnnexBMemoryModeInit(m_streamBuffer, 0);

    // JM デコーダを初期化
    InitJMDecoder();  
}

int Avcdec::vdec_stop()
{
    m_started = false;
    return 0;
}

void Avcdec::vdec_postprocess(UInt16 TYPE)
{
    m_postprocess_enabled = (TYPE != 0);
}

unsigned int Avcdec::vdec_put_bs(
    Byte* PAYLOAD,
    UInt32 LENGTH,
    UInt16 END_OF_AU,
    UInt32 PTS,
    UInt16 ERR_FLAG,
    UInt32 ERR_SN_SKIP)
{   
    // デコーダ起動状態を確認
    if (!m_started)
    {
        std::cout << "  ERROR: Decoder not started" << std::endl;
        return (unsigned int)-1;
    }
    
    // 入力データをストリームバッファへ蓄積
    if (PAYLOAD && LENGTH > 0)
    {
        // バッファ空き容量を確認
        if (!CheckBufferSpace(LENGTH))
        {
            std::cout << "  ERROR: Buffer overflow" << std::endl;
            return (unsigned int)-1;
        }
        
        // 入力データをコピー
        memcpy(m_streamBuffer + m_streamSize, PAYLOAD, LENGTH);
        m_streamSize += LENGTH;
    }
    
    // JM 側が参照するサイズ情報を更新
    g_memory_size = m_streamSize;
    
    // AU 終端フラグを処理
    if (END_OF_AU == 1)
    {
        HandleEndOfAU();
        
        // AU 終端受信時にデコードを実行
        DecodeBuffer();
    }
    
    return LENGTH;
}

Byte* Avcdec::vdec_get_picture(PICMETAINFO_AVC* PIC_METAINFO)
{
    {
        std::lock_guard<std::mutex> lock(m_frameQueueMutex);
        
        if (m_frameQueue.empty())
        {
            return NULL;
        }
        
        QueuedFrame frame = m_frameQueue.front();
        m_frameQueue.pop();
        
        if (PIC_METAINFO)
        {
            PIC_METAINFO->pic_width = frame.width;
            PIC_METAINFO->pic_height = frame.height;
            PIC_METAINFO->pic_type = frame.metadata.pic_type;
            PIC_METAINFO->bit_depth = frame.metadata.bit_depth;
        }
        
        m_frameCount++;
        return frame.data;
    }
}

unsigned int Avcdec::vdec_get_status(
    UInt16* DEC_STATUS,
    UInt16* DISP_STATUS,
    UInt16* ERR_STATUS)
{
    UInt16 dec_status = 0;
    
    // ステータスフラグ設定
    if (m_started)
        dec_status |= (1 << 6);  // Decoding in progress
    
    *DEC_STATUS = dec_status;
    *DISP_STATUS = m_frameQueue.empty() ? 0 : 1;
    *ERR_STATUS = 0;
    
    return 0;
}

void* Avcdec::vdec_get_DecodedHandle()
{
    return &m_decodedHandle;
}

void Avcdec::vdec_release_pic_buffer(Byte* PIC_ADDR)
{
    if (!PIC_ADDR)
        return;
    
    // 固定表示バッファなら未使用状態へ戻す
    for (int i = 0; i < m_bufferCount; i++)
    {
        if (m_pictureBuffers[i].data == PIC_ADDR)
        {
            m_pictureBuffers[i].locked = false;
            return;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_allocatedFrameMutex);
        auto it = m_allocatedQueuedFrames.find(PIC_ADDR);
        if (it != m_allocatedQueuedFrames.end())
        {
            delete[] PIC_ADDR;
            m_allocatedQueuedFrames.erase(it);
            return;
        }
    }
    
    std::cout << "    WARNING: Buffer address not found" << std::endl;
}

int Avcdec::vdec_YUV420toRGB24(
    int Mode,
    unsigned char* iBGR,
    unsigned char* iYUV,
    int width,
    int height)
{
    int ySize = width * height;
    int uvSize = ySize / 4;
    
    unsigned char* y_ptr = iYUV;
    unsigned char* u_ptr = iYUV + ySize;
    unsigned char* v_ptr = iYUV + ySize + uvSize;
    
    // 各画素を YUV から RGB へ変換
    for (int i = 0; i < ySize; i++)
    {
        int x = i % width;
        int y = i / width;
        int u_idx = (y / 2) * (width / 2) + (x / 2);
        
        int Y = y_ptr[i];
        int U = u_ptr[u_idx];
        int V = v_ptr[u_idx];
        
        // YUV -> RGB 変換式
        int R = Y + (1.402f * (V - 128));
        int G = Y - (0.344f * (U - 128)) - (0.714f * (V - 128));
        int B = Y + (1.772f * (U - 128));
        
        // 画素値を有効範囲に丸める
        R = std::max(0, std::min(255, R));
        G = std::max(0, std::min(255, G));
        B = std::max(0, std::min(255, B));
        
        int rgb_idx = i * 3;
        iBGR[rgb_idx] = R;
        iBGR[rgb_idx + 1] = G;
        iBGR[rgb_idx + 2] = B;
    }
    
    return width * height * 3;
}

void Avcdec::vdec_YUV420toRGB24_2(
    unsigned char* y,
    unsigned char* u,
    unsigned char* v,
    unsigned char* rgb,
    int width,
    int height)
{
    int ySize = width * height;
    
    for (int i = 0; i < ySize; i++)
    {
        int x = i % width;
        int y_idx = i / width;
        int u_idx = (y_idx / 2) * (width / 2) + (x / 2);
        
        int Y = y[i];
        int U = u[u_idx];
        int V = v[u_idx];
        
        int R = Y + (1.402f * (V - 128));
        int G = Y - (0.344f * (U - 128)) - (0.714f * (V - 128));
        int B = Y + (1.772f * (U - 128));
        
        R = std::max(0, std::min(255, R));
        G = std::max(0, std::min(255, G));
        B = std::max(0, std::min(255, B));
        
        int rgb_idx = i * 3;
        rgb[rgb_idx] = R;
        rgb[rgb_idx + 1] = G;
        rgb[rgb_idx + 2] = B;
    }
}

int Avcdec::YUV420toRGB24_DX(
    int Mode,
    unsigned char* iBGR,
    unsigned char* iYUV,
    int width,
    int height)
{
    return vdec_YUV420toRGB24(Mode, iBGR, iYUV, width, height);
}

void Avcdec::InitJMDecoder()
{   
    // JM 初期化パラメータを作成
    InputParameters inputParams = {};
    
    // メモリ入力モードのためファイルパスは空
    strcpy(inputParams.infile, "");
    strcpy(inputParams.outfile, "");
    strcpy(inputParams.reffile, "");
    
    inputParams.FileFormat = 0;
    inputParams.write_uv = 0;
    inputParams.bDisplayDecParams = 0;
    
    // デコーダをオープン
    if (OpenDecoder(&inputParams) != DEC_OPEN_NOERR)
    {
        std::cout << "  ERROR: OpenDecoder failed" << std::endl;
        return;
    }
}

void Avcdec::DecodeBuffer()
{
    if (!p_Dec)
    {
        std::cout << "  ERROR: p_Dec is NULL" << std::endl;
        return;
    }
    
    int frame_count = 0;
    int picture_count = 0;
    g_memory_pos = 0;
    
    DecodedPicList *pDecPicList = nullptr;
    int ret;
    
    // デコードループ（EOS まで1フレームずつ処理）
    do
    {
        ret = DecodeOneFrame(&pDecPicList);
        
        if (ret == DEC_EOS || ret == DEC_SUCCEED)
        {
            frame_count++;
            
            if (pDecPicList != nullptr)
            {
                for (DecodedPicList *pPic = pDecPicList; pPic != nullptr; pPic = pPic->pNext)
                {
                    if (pPic->bValid == 1 &&
                        pPic->iWidth > 0 &&
                        pPic->iHeight > 0 &&
                        pPic->pY != nullptr)
                    {           
                        ProcessDecodedPicture(pPic);
                        picture_count++;
                        
                        // 処理済みとして無効化
                        pPic->bValid = 0;
                    }
                }
            }
        }
        else
        {
            std::cout << "  Decode error. ret=" << ret << std::endl;
            break;
        }
    } while (ret == DEC_SUCCEED);
    
    // DPB をフラッシュして残フレームを取り出す
    FinitDecoder(&pDecPicList);
    
    // フラッシュ後に残っている有効画像を処理
    if (pDecPicList != nullptr)
    {
        for (DecodedPicList *pPic = pDecPicList; pPic != nullptr; pPic = pPic->pNext)
        {
            if (pPic->bValid == 1 &&
                pPic->iWidth > 0 &&
                pPic->iHeight > 0 &&
                pPic->pY != nullptr)
            {   
                ProcessDecodedPicture(pPic);
                picture_count++;              
                pPic->bValid = 0;
            }
        }
    }
}

void Avcdec::ProcessDecodedPicture(DecodedPicList *pPic)
{
    if (!pPic || !pPic->pY)
    {
        std::cout << "ERROR: Invalid picture pointer" << std::endl;
        return;
    }
    
    int width = pPic->iWidth;
    int height = pPic->iHeight;
    int ySize = width * height;
    int uvSize = ySize / 4;
    int totalSize = ySize + 2 * uvSize;
    
    // 利用可能な表示バッファを取得
    PictureBuffer* buffer = GetAvailableBuffer();
    if (!buffer)
    {
        std::cout << "ERROR: No available display buffer" << std::endl;
        return;
    }
    
    // YUV データを表示バッファへコピー
    if (pPic->pY)
    {
        memcpy(buffer->data, pPic->pY, ySize);
    }
    
    if (pPic->pU)
    {
        memcpy(buffer->data + ySize, pPic->pU, uvSize);
    }
    else
    {
        memset(buffer->data + ySize, 128, uvSize);
    }
    
    if (pPic->pV)
    {
        memcpy(buffer->data + ySize + uvSize, pPic->pV, uvSize);
    }
    else
    {
        memset(buffer->data + ySize + uvSize, 128, uvSize);
    }
    
    // バッファのメタ情報を更新
    buffer->width = width;
    buffer->height = height;
    buffer->poc = pPic->iPOC;
    buffer->locked = true;
    
    // アプリ側取り出し用キューへ追加
    QueueFrameForDisplay(buffer);
    buffer->locked = false;
}

bool Avcdec::CheckBufferSpace(UInt32 needed_bytes)
{
    return (m_streamSize + needed_bytes <= m_streamCapacity);
}

void Avcdec::HandleEndOfAU()
{
    // AU 終端通知用のダミースタートコードを追加
    if (CheckBufferSpace(3))
    {
        m_streamBuffer[m_streamSize++] = 0x00;
        m_streamBuffer[m_streamSize++] = 0x00;
        m_streamBuffer[m_streamSize++] = 0x01;
        g_memory_size = m_streamSize;
    }
}

Avcdec::PictureBuffer* Avcdec::GetAvailableBuffer()
{
    if (!m_pictureBuffers || m_bufferCount == 0)
    {
        std::cout << "ERROR: Picture buffers not allocated" << std::endl;
        return NULL;
    }
    
    // 最初に見つかった未ロックバッファを返す
    for (int i = 0; i < m_bufferCount; i++)
    {
        if (!m_pictureBuffers[i].locked)
        {
            return &m_pictureBuffers[i];
        }
    }

    return NULL;
}

void Avcdec::QueueFrameForDisplay(PictureBuffer* buffer)
{
    if (!buffer)
        return;

    int ySize = buffer->width * buffer->height;
    int uvSize = ySize / 4;
    int totalSize = ySize + (2 * uvSize);

    Byte* queuedData = new Byte[totalSize];
    memcpy(queuedData, buffer->data, totalSize);

    {
        std::lock_guard<std::mutex> lock(m_allocatedFrameMutex);
        m_allocatedQueuedFrames.insert(queuedData);
    }
    
    QueuedFrame frame;
    frame.data = queuedData;
    frame.width = buffer->width;
    frame.height = buffer->height;
    frame.poc = buffer->poc;
    frame.metadata.pic_width = buffer->width;
    frame.metadata.pic_height = buffer->height;
    frame.metadata.pic_type = 0;
    frame.metadata.bit_depth = 8;
    
    {
        std::lock_guard<std::mutex> lock(m_frameQueueMutex);
        m_frameQueue.push(frame);
    }

    {
        std::lock_guard<std::mutex> lock(m_decodedHandle.mutex);
        m_decodedHandle.signaled = true;
    }
    m_decodedHandle.cv.notify_all();
}