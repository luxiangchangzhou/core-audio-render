#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <Devicetopology.h>
#include <Endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>

#include <iostream>
#include <stdio.h>

using namespace std;


//-----------------------------------------------------------
// Play an audio stream on the default audio rendering
// device. The PlayAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data to the
// rendering device. The inner loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);


FILE* fp;
char* file_data;
int file_size = 0;
int write_count = 0;


long get_file_size(FILE * stream)
{
    long file_size = -1;
    long cur_offset = ftell(stream);	// 获取当前偏移位置
    if (cur_offset == -1) {
        printf("ftell failed :%s\n", strerror(errno));
        return -1;
    }
    if (fseek(stream, 0, SEEK_END) != 0) {	// 移动文件指针到文件末尾
        printf("fseek failed: %s\n", strerror(errno));
        return -1;
    }
    file_size = ftell(stream);	// 获取此时偏移值，即文件大小
    if (file_size == -1) {
        printf("ftell failed :%s\n", strerror(errno));
    }
    if (fseek(stream, cur_offset, SEEK_SET) != 0) {	// 将文件指针恢复初始位置
        printf("fseek failed: %s\n", strerror(errno));
        return -1;
    }
    return file_size;
}



//双声道 float
void lx_loadData(UINT32 bufferFrameCount, BYTE * pData, DWORD *flags)
{
    
    if (write_count < file_size)
    {
        memcpy(pData, file_data + write_count, bufferFrameCount*8);
        write_count += bufferFrameCount*8;
        *flags = 0;
    }
    else
    {
        *flags = AUDCLNT_BUFFERFLAGS_SILENT;
    }
    
}


int main()
{

    fp = fopen("D:\\lx.pcm", "rb");
	if (fp == 0)
	{
		cout << "open failed................................" << endl;
	}
    file_size = get_file_size(fp);
	
    file_data = new char[file_size];
    fread(file_data, 1, file_size, fp);


    HRESULT hr;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioRenderClient* pRenderClient = NULL;
    WAVEFORMATEX* pwfx = NULL;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    UINT32 numFramesPadding;
    BYTE* pData;
    DWORD flags = 0;

    CoInitialize(0);

    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
        (void**)&pEnumerator);
    EXIT_ON_ERROR(hr)

        hr = pEnumerator->GetDefaultAudioEndpoint(
            eRender, eConsole, &pDevice);
    EXIT_ON_ERROR(hr)

        hr = pDevice->Activate(
            IID_IAudioClient, CLSCTX_ALL,
            NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr)

        hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr)

        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,
            hnsRequestedDuration,
            0,
            pwfx,
            NULL);
    EXIT_ON_ERROR(hr)

        // Tell the audio source which format to use.
        ///////////////////////////////////////////////////luxiang//////////
        //hr = pMySource->SetFormat(pwfx);
    EXIT_ON_ERROR(hr)

        // Get the actual size of the allocated buffer.
        hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr)

        hr = pAudioClient->GetService(
            IID_IAudioRenderClient,
            (void**)&pRenderClient);
    EXIT_ON_ERROR(hr)

        // Grab the entire buffer for the initial fill operation.
        hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
    EXIT_ON_ERROR(hr)

        // Load the initial data into the shared buffer.
        //hr = pMySource->LoadData(bufferFrameCount, pData, &flags);
        lx_loadData(bufferFrameCount, pData, &flags);
    EXIT_ON_ERROR(hr)

        hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
    EXIT_ON_ERROR(hr)

        // Calculate the actual duration of the allocated buffer.
        hnsActualDuration = (double)REFTIMES_PER_SEC *
        bufferFrameCount / pwfx->nSamplesPerSec;

    hr = pAudioClient->Start();  // Start playing.
    EXIT_ON_ERROR(hr)

        // Each loop fills about half of the shared buffer.
        while (flags != AUDCLNT_BUFFERFLAGS_SILENT)
        {
            // Sleep for half the buffer duration.
            Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

            // See how much buffer space is available.
            hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
            EXIT_ON_ERROR(hr)

                numFramesAvailable = bufferFrameCount - numFramesPadding;

            // Grab all the available space in the shared buffer.
            hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
            EXIT_ON_ERROR(hr)

                // Get next 1/2-second of data from the audio source.
                //hr = pMySource->LoadData(numFramesAvailable, pData, &flags);
                lx_loadData(numFramesAvailable, pData, &flags);
            EXIT_ON_ERROR(hr)

                hr = pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
            EXIT_ON_ERROR(hr)
        }

    // Wait for last data in buffer to play before stopping.
    Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

    hr = pAudioClient->Stop();  // Stop playing.
    EXIT_ON_ERROR(hr)

        Exit:
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator)
        SAFE_RELEASE(pDevice)
        SAFE_RELEASE(pAudioClient)
        SAFE_RELEASE(pRenderClient)

        CoUninitialize();

        return hr;
}