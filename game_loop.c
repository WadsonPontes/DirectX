// gcc game_loop.c -o game_loop.exe -ld3d11 -ldxgi -ldxguid -ld2d1 -luuid -lole32 -lwindowscodecs -ldwrite -mwindows
// -mwindows Remove o Console
// Exemplos da Micorsoft: https://github.com/microsoft/Windows-classic-samples

#include <stdio.h>
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <d2d1.h>
#include <wincodec.h>
#include <dwrite.h>
#include <initguid.h>

#ifdef DEFINE_GUID
DEFINE_GUID(IID_IDWriteFactory, 0xb859ee5a, 0xd838, 0x4b5b, 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb,0x48);
#endif

// Variáveis globais DirectX
IDXGISwapChain *swapChain;
ID3D11Device *device;
ID3D11DeviceContext *deviceContext;
ID3D11RenderTargetView *backBuffer;

// Variáveis Direct2D
ID2D1Factory *pD2DFactory;
ID2D1RenderTarget *pRenderTarget;
ID2D1Bitmap *pBitmap;
IWICImagingFactory *pWICFactory;
ID2D1SolidColorBrush *brush;
IDWriteFactory *writeFactory;
IDWriteTextFormat *textFormat;
wchar_t fpsText[16];

// Variáveis para FPS
LARGE_INTEGER frequency, startCounter, endCounter;
double frameTime = 0.0;
double fps = 0.0;
int frameCount = 0;
double elapsedTime = 0.0;

// Cor de limpeza do background (RGBA)
float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // Limpa com transparência
D2D1_RECT_F rectangle = {0, 0, 400, 600};
D2D1_RECT_F layoutRect = {650.0f, 10.0f, 800.0f, 50.0f};

// Prototipação
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void InitD3D(HWND hWnd);
void InitD2D(HWND hWnd);
void CleanD3D(void);
void RenderFrame(void);
void InitFPSCounter(void);
void UpdateFPS(void);
void LoadBitmapFromFile(LPCWSTR uri, ID2D1Bitmap **ppBitmap);

// Função principal
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Criação da janela
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "janela_game_loop";

    RegisterClassEx(&wc);

    RECT wr = {0, 0, 800, 600};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    //SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

    HWND hWnd = CreateWindowEx(0,
        "janela_game_loop",
        "Janela DirectX COM C",
        WS_OVERLAPPEDWINDOW,
        0, 0,
        wr.right - wr.left, wr.bottom - wr.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    // Tornar a janela transparente
    SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
    //SetLayeredWindowAttributes(hWnd, 0, 255, LWA_COLORKEY); 

    ShowWindow(hWnd, nCmdShow);
    //ShowWindow(hWnd, SW_HIDE);
    //ShowWindow(hWnd, SW_SHOWNORMAL);
    //ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    UpdateWindow(hWnd);

    // Inicialização do DirectX e Direct2D
    InitD3D(hWnd);
    InitD2D(hWnd);

    // Inicializando QueryPerformanceCounter
    InitFPSCounter();

    // Carregar a imagem
    LoadBitmapFromFile(L"image.png", &pBitmap);

    // Loop principal
    MSG msg = {0};
    while (TRUE) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Renderização de cada frame
        RenderFrame();

        // Calcular e exibir FPS
        UpdateFPS();
    }

    // Limpeza do DirectX e Direct2D
    CleanD3D();

    return msg.wParam;
}

// Processamento de mensagens da janela
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

// Inicializa o Direct3D
void InitD3D(HWND hWnd) {
    // Configuração da swap chain
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    scd.BufferCount = 1;                                    // Um back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;    // Formato 32-bit cor
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferDesc.Width = 1920;
    scd.BufferDesc.Height = 1080;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // Alvo de renderização
    scd.OutputWindow = hWnd;                                // Janela de saída
    scd.SampleDesc.Count = 1;                               // Anti-aliasing 4x
    scd.SampleDesc.Quality = 0;
    scd.Windowed = FALSE;                                    // Modo janela
    //scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    //scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    const D3D_FEATURE_LEVEL level[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    IDXGIFactory6 *pFactory;
    CreateDXGIFactory(&IID_IDXGIFactory6, (void **)&pFactory);

    IDXGIAdapter *pAdapter = NULL;
    IDXGIAdapter *bestAdapter = NULL;

    //for (UINT i = 0; pFactory->lpVtbl->EnumAdapterByGpuPreference(pFactory, i, DXGI_GPU_PREFERENCE_MINIMUM_POWER, &IID_IDXGIAdapter, (void **)&pAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
    for (UINT i = 0; pFactory->lpVtbl->EnumAdapterByGpuPreference(pFactory, i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, &IID_IDXGIAdapter, (void **)&pAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
        bestAdapter = pAdapter;
        break;
    }

    // Use o melhor adaptador encontrado (GPU dedicada)
    if (bestAdapter) {
        D3D11CreateDeviceAndSwapChain(bestAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, level, 2,
            D3D11_SDK_VERSION, &scd, &swapChain, &device, NULL, &deviceContext);
        bestAdapter->lpVtbl->Release(bestAdapter);
    } else {
        // Caso não encontre um adaptador melhor, use o padrão
        D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, level, 2,
            D3D11_SDK_VERSION, &scd, &swapChain, &device, NULL, &deviceContext);
    }

    pFactory->lpVtbl->Release(pFactory);

    swapChain->lpVtbl->SetFullscreenState(swapChain, TRUE, NULL);

    // Criação do back buffer
    ID3D11Texture2D *pBackBuffer;
    swapChain->lpVtbl->GetBuffer(swapChain, 0, &IID_ID3D11Texture2D, (void**)&pBackBuffer);
    device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)pBackBuffer, NULL, &backBuffer);
    pBackBuffer->lpVtbl->Release(pBackBuffer);

    // Configurando o alvo de renderização
    deviceContext->lpVtbl->OMSetRenderTargets(deviceContext, 1, &backBuffer, NULL);
}

// Inicializa o Direct2D e WIC (para carregar imagens)
void InitD2D(HWND hWnd) {
    D2D1_FACTORY_OPTIONS options = {D2D1_DEBUG_LEVEL_NONE};
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &IID_ID2D1Factory, &options, (void **)&pD2DFactory);

    // Criação de render target associado ao Direct3D
    IDXGISurface *dxgiBackBuffer;
    swapChain->lpVtbl->GetBuffer(swapChain, 0, &IID_IDXGISurface, (void **)&dxgiBackBuffer);

    D2D1_PIXEL_FORMAT pixelFormat = {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
        D2D1_ALPHA_MODE_PREMULTIPLIED
    };
    D2D1_RENDER_TARGET_PROPERTIES props = {
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        pixelFormat,
        0,
        0,
        D2D1_RENDER_TARGET_USAGE_NONE,
        D2D1_FEATURE_LEVEL_DEFAULT
    };
    D2D1_BITMAP_PROPERTIES bitmapProps = {
        pixelFormat,
        0,
        0
    };
    pD2DFactory->lpVtbl->CreateDxgiSurfaceRenderTarget(pD2DFactory, dxgiBackBuffer, &props, &pRenderTarget);

    D2D1_COLOR_F colorBack = {0.0f, 0.0f, 0.0f, 1.0f};
    D2D1_COLOR_F colorWhite = {1.0f, 1.0f, 1.0f, 1.0f};

    // Cria o brush
    ((ID2D1RenderTarget*)pRenderTarget)->lpVtbl->CreateSolidColorBrush((ID2D1RenderTarget*)pRenderTarget, &colorWhite, NULL, &brush);

    // Cria e carrega a fonte
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown**)&writeFactory);
    writeFactory->lpVtbl->CreateTextFormat(writeFactory, L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL,
                                           DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 24.0f, L"en-us", &textFormat);

    // Inicializar a fábrica WIC para carregar imagens
    //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    //CoInitializeEx(NULL, COINIT_MULTITHREADED);
    CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void **)&pWICFactory);
    dxgiBackBuffer->lpVtbl->Release(dxgiBackBuffer);
}

// Carrega um bitmap (imagem) de arquivo
void LoadBitmapFromFile(LPCWSTR uri, ID2D1Bitmap **ppBitmap) {
    IWICBitmapDecoder *pDecoder = NULL;
    IWICBitmapFrameDecode *pFrame = NULL;
    IWICFormatConverter *pConverter = NULL;

    // Criar o decoder de imagem
    pWICFactory->lpVtbl->CreateDecoderFromFilename(pWICFactory, uri, NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    
    // Pegar o primeiro frame
    pDecoder->lpVtbl->GetFrame(pDecoder, 0, &pFrame);
    
    // Converter o formato para um que o Direct2D entenda
    pWICFactory->lpVtbl->CreateFormatConverter(pWICFactory, &pConverter);
    pConverter->lpVtbl->Initialize(pConverter, (IWICBitmapSource*)pFrame, &GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut);

    // Criar o bitmap no Direct2D a partir da imagem convertida
    pRenderTarget->lpVtbl->CreateBitmapFromWicBitmap(pRenderTarget, (IWICBitmapSource*)pConverter, NULL, ppBitmap);

    // Limpeza
    pConverter->lpVtbl->Release(pConverter);
    pFrame->lpVtbl->Release(pFrame);
    pDecoder->lpVtbl->Release(pDecoder);
}

// Renderiza um frame
void RenderFrame(void) {
    // Limpeza do back buffer
    deviceContext->lpVtbl->ClearRenderTargetView(deviceContext, backBuffer, clearColor);

    // Iniciar a renderização com Direct2D
    pRenderTarget->lpVtbl->BeginDraw(pRenderTarget);

    // Desenhar a imagem carregada
    pRenderTarget->lpVtbl->DrawBitmap(pRenderTarget, pBitmap, &rectangle, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, NULL);

    // Desenha o FPS na canto superior direito
    ((ID2D1RenderTarget*)pRenderTarget)->lpVtbl->DrawText(
        (ID2D1RenderTarget*)pRenderTarget,
        fpsText,
        wcslen(fpsText),
        textFormat,
        &layoutRect,
        (ID2D1Brush*)brush,
        D2D1_DRAW_TEXT_OPTIONS_NONE,
        DWRITE_MEASURING_MODE_NATURAL
    );

    pRenderTarget->lpVtbl->EndDraw(pRenderTarget, NULL, NULL);

    // Apresentar o buffer trocado
    //swapChain->lpVtbl->Present(swapChain, 0, DXGI_PRESENT_ALLOW_TEARING);
    swapChain->lpVtbl->Present(swapChain, 0, 0);
}

// Inicializa o contador de FPS
void InitFPSCounter(void) {
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startCounter);
}

// Atualiza o FPS
void UpdateFPS(void) {
    QueryPerformanceCounter(&endCounter);
    frameTime = (double)(endCounter.QuadPart - startCounter.QuadPart) / frequency.QuadPart;
    startCounter = endCounter;
    elapsedTime += frameTime;

    frameCount++;
    if (elapsedTime >= 1.0) {
        fps = (double)frameCount / elapsedTime;
        swprintf(fpsText, 16, L"FPS: %.2f", fps);
        frameCount = 0;
        elapsedTime = 0.0;
    }
}

// Limpeza do DirectX e Direct2D
void CleanD3D(void) {
    swapChain->lpVtbl->Release(swapChain);
    backBuffer->lpVtbl->Release(backBuffer);
    device->lpVtbl->Release(device);
    deviceContext->lpVtbl->Release(deviceContext);
    pWICFactory->lpVtbl->Release(pWICFactory);
    CoUninitialize();
}