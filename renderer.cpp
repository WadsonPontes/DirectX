// g++ -o renderer.exe renderer.cpp -lgdi32 -ld2d1 -lole32 -luuid -lwindowscodecs -DUNICODE

#include <windows.h>
#include <d2d1.h>
#include <wincodec.h>

// Ponteiros globais para interfaces Direct2D
ID2D1Factory* pFactory = NULL;
ID2D1HwndRenderTarget* pRenderTarget = NULL;
ID2D1Bitmap* pBitmap = NULL;
IWICImagingFactory* pWICFactory = NULL;

// Função para carregar imagem PNG usando WIC
HRESULT LoadBitmapFromFile(LPCWSTR uri, ID2D1RenderTarget* pRenderTarget, ID2D1Bitmap** ppBitmap) {
    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICFormatConverter* pConverter = NULL;

    HRESULT hr = pWICFactory->CreateDecoderFromFilename(uri, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    if (SUCCEEDED(hr)) {
        hr = pWICFactory->CreateFormatConverter(&pConverter);
    }
    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
    }
    if (SUCCEEDED(hr)) {
        hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, ppBitmap);
    }

    if (pDecoder) pDecoder->Release();
    if (pSource) pSource->Release();
    if (pConverter) pConverter->Release();

    return hr;
}

// Inicializa Direct2D
HRESULT InitD2D(HWND hwnd) {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
    if (SUCCEEDED(hr)) {
        RECT rc;
        GetClientRect(hwnd, &rc);

        // Cria o alvo de renderização
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)),
            &pRenderTarget
        );
    }
    if (SUCCEEDED(hr)) {
        hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));
    }
    if (SUCCEEDED(hr)) {
        // Carrega a imagem .png
        hr = LoadBitmapFromFile(L"image.png", pRenderTarget, &pBitmap);
    }

    return hr;
}

// Desenha o quadrado com a textura carregada
void OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);

    pRenderTarget->BeginDraw();
    pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // Desenha o bitmap (a imagem PNG) em um quadrado de 256x256
    D2D1_RECT_F rectangle = D2D1::RectF(50, 50, 306, 306);
    pRenderTarget->DrawBitmap(pBitmap, rectangle);

    pRenderTarget->EndDraw();
    EndPaint(hwnd, &ps);
}

// Limpa recursos
void Cleanup() {
    if (pBitmap) pBitmap->Release();
    if (pRenderTarget) pRenderTarget->Release();
    if (pFactory) pFactory->Release();
    if (pWICFactory) pWICFactory->Release();
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
        if (pRenderTarget) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            pRenderTarget->Resize(D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top));
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Função principal
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Registro da classe de janela
    const wchar_t CLASS_NAME[] = L"SampleWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Criação da janela
    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Direct2D PNG Rendering",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Inicialização Direct2D
    if (FAILED(InitD2D(hwnd))) {
        MessageBox(NULL, L"Failed to initialize Direct2D", L"Error", MB_OK);
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
