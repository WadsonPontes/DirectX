// gcc -o renderer.exe renderer.c -lgdi32 -ld2d1 -lole32 -luuid -lwindowscodecs

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <d2d1.h>
#include <wincodec.h>

// Ponteiros globais para interfaces Direct2D
ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pHwndRenderTarget = NULL;
ID2D1Bitmap* pBitmap = NULL;
IWICImagingFactory* pWICFactory = NULL;

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

    // Chame a função usando a função apontada
    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->DrawBitmap((ID2D1RenderTarget*)pHwndRenderTarget, pBitmap, &rectangle, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);

    ((ID2D1RenderTarget*)pHwndRenderTarget)->lpVtbl->EndDraw((ID2D1RenderTarget*)pHwndRenderTarget, NULL, NULL);
    EndPaint(hwnd, &ps);
}

// Limpa recursos
void Cleanup() {
    if (pWICFactory) pWICFactory->lpVtbl->Release(pWICFactory);
}

// Função da janela
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
