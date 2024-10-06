// gcc -o renderer.exe renderer.c -lgdi32 -ld2d1 -lole32 -luuid -lwindowscodecs

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <d2d1.h>
#include <wincodec.h>
#include <dwrite.h>  // Para uso do DirectWrite
#include <time.h>
#include <initguid.h> // Inclua esta linha se necessário
#include <guiddef.h>

#ifdef DEFINE_GUID

DEFINE_GUID(IID_IDWriteFactory, 
0xb859ee5a, 0xd838, 0x4b5b, 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb,0x48);

#endif  // DEFINE_GUID

// Variáveis globais para calcular FPS
time_t startTime, endTime;
long framesTime = 0;
double fpsTime = 0.0f;

clock_t startClock;
int framesClock = 0;
float fpsClock = 0.0f;

// Ponteiros globais para interfaces Direct2D
ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pHwndRenderTarget = NULL;
ID2D1Bitmap* pBitmap = NULL;
IWICImagingFactory* pWICFactory = NULL;

IDWriteFactory* pDWriteFactory = NULL;  // Para texto
IDWriteTextFormat* pTextFormat = NULL;  // Para formatar o texto

void CalculateFPSClock() {
    ++framesClock;

    if (!startClock) {
        startClock = clock();
        return;
    }

    clock_t currentTime = clock();
    float elapsedTime = (float)(currentTime - startClock) / CLOCKS_PER_SEC;

    if (elapsedTime > 1.0f) { // Atualiza o FPS CLock a cada segundo
        fpsClock = framesClock / elapsedTime;
        framesClock = 0;
        startClock = currentTime;
    }
}

// Função para carregar imagem PNG usando WIC
HRESULT LoadBitmapFromFile(LPCWSTR uri, ID2D1HwndRenderTarget* pHwndRenderTarget, ID2D1Bitmap** ppBitmap) {
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICFormatConverter* pConverter = NULL;

    HRESULT hr = pWICFactory->lpVtbl->CreateDecoderFromFilename(pWICFactory, uri, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (SUCCEEDED(hr)) {
        hr = pDecoder->lpVtbl->GetFrame(pDecoder, 0, &pSource);
    }
    if (SUCCEEDED(hr)) {
        hr = pWICFactory->lpVtbl->CreateFormatConverter(pWICFactory, &pConverter);
    }
    if (SUCCEEDED(hr)) {
        hr = pConverter->lpVtbl->Initialize(pConverter, (IWICBitmapSource*)pSource, &GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
    }
    if (SUCCEEDED(hr)) {
        hr = ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->CreateBitmapFromWicBitmap((ID2D1RenderTarget*)pHwndRenderTarget, (IWICBitmapSource*)pConverter, NULL, ppBitmap);
    }

    if (pDecoder) pDecoder->lpVtbl->Release(pDecoder);
    if (pSource) pSource->lpVtbl->Release(pSource);
    if (pConverter) pConverter->lpVtbl->Release(pConverter);

    return hr;
}

// Inicializa Direct2D
HRESULT InitD2D(HWND hwnd) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, NULL, (void**)&pFactory);
    
    if (SUCCEEDED(hr)) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        D2D1_PIXEL_FORMAT pixelFormat = {
            DXGI_FORMAT_R8G8B8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE
        };
        D2D1_RENDER_TARGET_PROPERTIES rtProps = {
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            pixelFormat,
            0,
            0,
            D2D1_RENDER_TARGET_USAGE_NONE,
            D2D1_FEATURE_LEVEL_DEFAULT
        };
        D2D1_SIZE_U size = {
            rc.right - rc.left, // Largura
            rc.bottom - rc.top   // Altura
        };
        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = {
            hwnd,
            size,
            D2D1_PRESENT_OPTIONS_NONE
        };
        
        // Cria o alvo de renderização
        hr = pFactory->lpVtbl->CreateHwndRenderTarget(
            pFactory,
            &rtProps,
            &hwndProps,
            &pHwndRenderTarget
        );

    }
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void**)&pWICFactory);
    }
    if (SUCCEEDED(hr)) {
        // Carrega a imagem .png
        hr = LoadBitmapFromFile(L"image.png", pHwndRenderTarget, &pBitmap);
    }
    if (SUCCEEDED(hr)) {
        // Inicializa DirectWrite para renderizar o texto
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&pDWriteFactory);
    }
    if (FAILED(hr)) {
        char* errorMsg = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL, hr, 0, (LPSTR)&errorMsg, 0, NULL);

        printf("HRESULT error: 0x%lx - %s\n", hr, errorMsg);
        exit(-1);
    }
    if (SUCCEEDED(hr)) {
        // Cria o formato de texto
        hr = pDWriteFactory->lpVtbl->CreateTextFormat(
            pDWriteFactory,
            L"Arial",         // Fonte
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            24.0f,            // Tamanho da fonte
            L"en-us",         // Localidade
            &pTextFormat      // Ponteiro para armazenar o formato
        );
    }

    return hr;
}

// Desenha o quadrado com a textura carregada
void OnPaint(HWND hwnd) {
    D2D1_COLOR_F color = {1.0f, 1.0f, 1.0f, 1.0f};
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);

    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->BeginDraw((ID2D1RenderTarget*)pHwndRenderTarget);

    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->Clear((ID2D1RenderTarget*)pHwndRenderTarget, &color);

    // Desenha o bitmap (a imagem PNG) em um quadrado de 256x256
    D2D1_RECT_F rectangle = {50, 50, 306, 306};

    // Desenha a imagem na tela
    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->DrawBitmap((ID2D1RenderTarget*)pHwndRenderTarget, pBitmap, &rectangle, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);

    ID2D1SolidColorBrush* pBrushFillRect;
    ID2D1SolidColorBrush* pBrushRect;
    D2D1_COLOR_F colorfillRect = {0.0f, 0.0f, 0.0f, 1.0f};
    D2D1_COLOR_F colorRect = {0.0f, 0.8f, 0.0f, 1.0f};
    D2D1_RECT_F fillRect = {500, 300, 800, 600};
    D2D1_RECT_F rectRect = {500, 300, 800, 600};

    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->CreateSolidColorBrush((ID2D1RenderTarget*)pHwndRenderTarget, &colorfillRect, NULL, &pBrushFillRect);
    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->CreateSolidColorBrush((ID2D1RenderTarget*)pHwndRenderTarget, &colorRect, NULL, &pBrushRect);


    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->FillRectangle((ID2D1RenderTarget*)pHwndRenderTarget, &rectRect, (ID2D1Brush*)pBrushRect);
    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->DrawRectangle((ID2D1RenderTarget*)pHwndRenderTarget, &fillRect, (ID2D1Brush*)pBrushFillRect, 1, NULL);

    // Converte o valor de FPS Clock para string
    wchar_t fpsClockText[50];
    swprintf(fpsClockText, 50, L"FPS Clock: %f", fpsClock);

    // Cria uma cor para o texto
    D2D1_COLOR_F colorTextFpsClock = {0.0f, 0.0f, 0.0f, 1.0f};
    ID2D1SolidColorBrush* pBrushText = NULL;
    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->CreateSolidColorBrush((ID2D1RenderTarget*)pHwndRenderTarget, &colorTextFpsClock, NULL, &pBrushText);

    // Define a área onde o texto será desenhado
    D2D1_RECT_F layoutRect = {400, 10, 800, 30};

    // Desenha o FPS Clock no canto superior direito
    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->DrawText(
        (ID2D1RenderTarget*)pHwndRenderTarget,
        fpsClockText,
        wcslen(fpsClockText),
        pTextFormat,
        &layoutRect,
        (ID2D1Brush*)pBrushText,
        D2D1_DRAW_TEXT_OPTIONS_NONE,
        DWRITE_MEASURING_MODE_NATURAL
    );

    // Finaliza a renderização
    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->EndDraw((ID2D1RenderTarget*)pHwndRenderTarget, NULL, NULL);
    EndPaint(hwnd, &ps);
}

// Limpa recursos
void Cleanup() {
    if (pWICFactory) pWICFactory->lpVtbl->Release(pWICFactory);
}

// Função da janela
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Calcula o FPS Clock
    CalculateFPSClock();

    switch (uMsg) {
        case WM_PAINT:
            OnPaint(hwnd);
            return 0;

        case WM_DESTROY:
            Cleanup();
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
            if (pHwndRenderTarget) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                D2D1_SIZE_U size = {
                    rc.right - rc.left, // Largura
                    rc.bottom - rc.top   // Altura
                };

                pHwndRenderTarget->lpVtbl->Resize(pHwndRenderTarget, &size);
            }
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Função principal
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SampleWindowClass";

    RegisterClass(&wc);

    // Criação da janela
    HWND hwnd = CreateWindowEx(
        0, "SampleWindowClass", "Direct2D PNG Rendering",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Inicialização Direct2D
    if (FAILED(InitD2D(hwnd))) {
        printf("Error: Failed to initialize Direct2D\n");
        MessageBox(NULL, "Failed to initialize Direct2D", "Error", MB_OK);
        return 0;
    }

    // Loop de mensagens
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Cleanup();
    return 0;
}
